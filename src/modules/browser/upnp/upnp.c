/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>
#include <string.h>
#include <pthread.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "vfs.h"
#include "volumes.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME  "upnp"

#define UPNP_MEDIA_SERVER   "urn:schemas-upnp-org:device:MediaServer:*"
#define UPNP_CONTENT_DIR    "urn:schemas-upnp-org:service:ContentDirectory"
#define UPNP_MAX_BROWSE     1024
#define UPNP_DEFAULT_ROOT   "0"
#define ITEM_CLASS_IMAGE    "object.item.imageItem"
#define ITEM_CLASS_AUDIO    "object.item.audioItem"
#define ITEM_CLASS_VIDEO    "object.item.videoItem"
#define ITEM_CLASS_TEXT     "object.item.textItem"

typedef struct Enna_Module_UPnP_s
{
    Evas *e;
    Enna_Module *em;
    pthread_mutex_t mutex;
    Eina_List *devices;
    GMainContext *mctx;
    GUPnPContext *ctx;
    GUPnPControlPoint *cp;
    pthread_mutex_t mutex_id;
    char *current_id;
} Enna_Module_UPnP;

typedef struct upnp_media_server_s {
    GUPnPDeviceInfo *info;
    GUPnPServiceProxy *content_dir;
    char *type;
    char *location;
    char *udn;
    char *name;
    char *model;
} upnp_media_server_t;

static Enna_Module_UPnP *mod;

/* UPnP Internals */

static void
upnp_current_id_set(const char *id)
{
    pthread_mutex_lock(&mod->mutex_id);
    ENNA_FREE(mod->current_id);
    mod->current_id = id ? strdup(id) : NULL;
    pthread_mutex_unlock(&mod->mutex_id);
}

static void
upnp_media_server_free(upnp_media_server_t *srv)
{
    if (!srv)
        return;

    g_object_unref(srv->info);
    g_object_unref(srv->content_dir);

    ENNA_FREE(srv->type);
    ENNA_FREE(srv->location);
    ENNA_FREE(srv->udn);
    ENNA_FREE(srv->name);
    ENNA_FREE(srv->model);
    ENNA_FREE(srv);
}

typedef struct upnp_filter_arg_s {
    Eina_List **list;
    ENNA_VFS_CAPS cap;
} upnp_filter_arg_t;

static void
didl_parse_item(GUPnPDIDLLiteParser *parser,
                GUPnPDIDLLiteItem *item, gpointer user_data)
{
    GUPnPDIDLLiteObject *obj;
    Enna_Vfs_File *f = NULL;
    const char *id, *title;
    const char *class_name, *uri;
    Eina_List **list = NULL;
    GUPnPDIDLLiteResource *res;
    char *icon = NULL;
    GList *resources;
    upnp_filter_arg_t *arg;
    ENNA_VFS_CAPS cap;

    arg  = user_data;
    list = arg->list;
    cap  = arg->cap;
    obj  = (GUPnPDIDLLiteObject *) item;

    id = gupnp_didl_lite_object_get_id(obj);
    if (!id)
        return;

    title = gupnp_didl_lite_object_get_title(obj);
    if (!title)
        return;

    class_name = gupnp_didl_lite_object_get_upnp_class(obj);
    if (!class_name)
        return;

    switch (cap)
    {
    case ENNA_CAPS_PHOTO:
        if (!strncmp(class_name,
                     ITEM_CLASS_IMAGE, strlen(ITEM_CLASS_IMAGE)))
            icon = "icon/photo";
        else
            return;
        break;
    case ENNA_CAPS_MUSIC:
        if (!strncmp(class_name,
                     ITEM_CLASS_AUDIO, strlen(ITEM_CLASS_AUDIO)))
            icon = "icon/music";
        else
            return;
        break;
    case ENNA_CAPS_VIDEO:
        if (!strncmp(class_name,
                     ITEM_CLASS_VIDEO, strlen(ITEM_CLASS_VIDEO)))
            icon = "icon/video";
        else
            return;
        break;
    default:
        return;
    }

    if (!icon)
        icon = "icon/hd";

    resources = gupnp_didl_lite_object_get_resources(obj);
    if (!resources)
        return;

    res = resources->data;
    if (!res)
        goto err_res;

    uri = gupnp_didl_lite_resource_get_uri(res);
    if (!uri)
        goto err_res;

    f = enna_vfs_create_file(uri, title, icon, NULL);
    *list = eina_list_append(*list, f);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "DIDL item '%s' (id: %s, uri: %s)", title, id, uri);

err_res:
    g_list_free(resources);
}

static void
didl_parse_container(GUPnPDIDLLiteParser *parser,
                     GUPnPDIDLLiteContainer *container,
                     gpointer user_data)
{
    GUPnPDIDLLiteObject *obj;
    Enna_Vfs_File *f = NULL;
    const char *id, *title;
    Eina_List **list = NULL;
    char uri[512];

    obj = (GUPnPDIDLLiteObject *) container;
    list = user_data;

    id = gupnp_didl_lite_object_get_id(obj);
    if (!id)
        return;

    title = gupnp_didl_lite_object_get_title(obj);
    if (!title)
        return;

    snprintf(uri, sizeof(uri), "%s/%s", mod->current_id, id);

    f = enna_vfs_create_directory(uri, title, "icon/directory", NULL);
    *list = eina_list_append(*list, f);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "DIDL container '%s' (id: %s, uri: %s)", title, id, uri);
}

static Eina_List *
didl_process(char *didl, char *udn, ENNA_VFS_CAPS cap)
{
    Eina_List *list = NULL;
    GUPnPDIDLLiteParser *parser;
    upnp_filter_arg_t arg;

    parser = gupnp_didl_lite_parser_new();

    arg.list = &list;
    arg.cap = cap;

    g_signal_connect(parser, "container-available",
                     G_CALLBACK(didl_parse_container), &list);
    g_signal_connect(parser, "item-available",
                     G_CALLBACK(didl_parse_item), &arg);

    gupnp_didl_lite_parser_parse_didl(parser, didl, NULL);

    g_object_unref(parser);

    return list;
}

static Eina_List *
upnp_browse(upnp_media_server_t *srv, const char *container_id,
            guint32 starting_index, guint32 requested_count,
            ENNA_VFS_CAPS cap)
{
    guint32 number_returned, total_matches, si;
    Eina_List *results = NULL;
    char *didl_xml;
    GError *error;

    g_object_ref(srv->content_dir);
    didl_xml = NULL;
    error = NULL;

    si = starting_index;

    if (!srv)
        return NULL;

    /* UPnP Browse request */
    gupnp_service_proxy_send_action
        (srv->content_dir, "Browse", &error,
         /* IN args */
         "ObjectID",       G_TYPE_STRING, container_id,
         "BrowseFlag",     G_TYPE_STRING, "BrowseDirectChildren",
         "Filter",         G_TYPE_STRING, "*",
         "StartingIndex",  G_TYPE_UINT,   starting_index,
         "RequestedCount", G_TYPE_UINT,   requested_count,
         "SortCriteria",   G_TYPE_STRING, "",
         NULL,
         /* OUT args */
         "Result",         G_TYPE_STRING, &didl_xml,
         "NumberReturned", G_TYPE_UINT,   &number_returned,
         "TotalMatches",   G_TYPE_UINT,   &total_matches,
         NULL);

    /* check action result */
    if (error)
    {
        GUPnPServiceInfo *info;

        info = GUPNP_SERVICE_INFO(srv->content_dir);
        enna_log(ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                 "Failed to browse '%s': %s",
                 gupnp_service_info_get_location(info), error->message);
        g_error_free(error);
        goto err;
    }

    if (didl_xml)
    {
        guint32 remaining;
        Eina_List *list;

        list = didl_process(didl_xml, srv->udn, cap);
        results = eina_list_merge(results, list);
        g_free(didl_xml);

        /* See if we have more objects to get */
        si += number_returned;
        remaining = total_matches - si;

        /* Keep browsing till we get each and every object */
        if (remaining)
        {
            list = upnp_browse(srv, container_id, si,
                               MIN(remaining, UPNP_MAX_BROWSE), cap);
            results = eina_list_merge(results, list);
        }
    }

 err:
    g_object_unref(srv->content_dir);
    return results;
}

static Eina_List *
browse_server_list(const char *uri, ENNA_VFS_CAPS cap)
{
    upnp_media_server_t *srv = NULL;
    char udn[512], id[512];
    char *container_id;
    int res;
    Eina_List *l;

    if (!uri)
        return NULL;

    res = sscanf(uri, "udn:%[^,],id:%[^,]", udn, id);
    if (res != 2)
        return NULL;

    container_id = strrchr(id, '/');
    if (!container_id)
        container_id = UPNP_DEFAULT_ROOT;
    else
        container_id++;

    EINA_LIST_FOREACH(mod->devices, l, srv)
        if (!strcmp(srv->udn, udn))
            break;

    /* no server to browse */
    if (!srv)
        return NULL;

    /* memorize our position */
    upnp_current_id_set(uri);

    return upnp_browse(srv, container_id, 0, UPNP_MAX_BROWSE, cap);
}

static Eina_List *
upnp_list_mediaservers(void)
{
    Eina_List *servers = NULL;
    Eina_List *l;
    upnp_media_server_t *srv;

    EINA_LIST_FOREACH(mod->devices, l, srv)
    {

        char name[256], uri[1024];
        Enna_Vfs_File *f;

        snprintf(name, sizeof(name), "%s (%s)", srv->name, srv->model);
        snprintf(uri, sizeof(uri), "udn:%s,id:%s",
                 srv->udn, UPNP_DEFAULT_ROOT);

        f = enna_vfs_create_directory(uri, name, "icon/dev/nfs", NULL);
        servers = eina_list_append(servers, f);
    }

    return servers;
}

static Eina_List *
_class_browse_up(const char *id, void *cookie, ENNA_VFS_CAPS cap)
{
    /* clean up our current position */
    upnp_current_id_set(NULL);

    if (!id)
    {
        /* list available UPnP media servers */
        return upnp_list_mediaservers();
    }

    /* browse content from a given media server */
    return browse_server_list(id, cap);
}

#ifdef BUILD_ACTIVITY_MUSIC
static Eina_List *
music_class_browse_up(const char *id, void *cookie)
{
    return _class_browse_up(id, cookie, ENNA_CAPS_MUSIC);
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Eina_List *
video_class_browse_up(const char *id, void *cookie)
{
    return _class_browse_up(id, cookie, ENNA_CAPS_VIDEO);
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Eina_List *
photo_class_browse_up(const char *id, void *cookie)
{
    return _class_browse_up(id, cookie, ENNA_CAPS_PHOTO);
}
#endif

static Eina_List *
_class_browse_down(void *cookie, ENNA_VFS_CAPS cap)
{
    char *prev_id;
    char new_id[512] = { 0 };
    int i, len;

    /* no recorded position, return to main browsers list */
    if (!mod->current_id)
        return NULL;

    /* try to guess where we come from */
    prev_id = strrchr(mod->current_id, '/');
    if (!prev_id)
    {
        upnp_current_id_set(NULL);
        return upnp_list_mediaservers();
    }

    len = strlen(mod->current_id) - strlen(prev_id);
    for (i = 0; i < len; i++)
        new_id[i] = mod->current_id[i];

    return browse_server_list(new_id, cap);
}

#ifdef BUILD_ACTIVITY_MUSIC
static Eina_List *
music_class_browse_down(void *cookie)
{
    return _class_browse_down(cookie, ENNA_CAPS_MUSIC);
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Eina_List *
video_class_browse_down(void *cookie)
{
    return _class_browse_down(cookie, ENNA_CAPS_VIDEO);
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Eina_List *
photo_class_browse_down(void *cookie)
{
    return _class_browse_down(cookie, ENNA_CAPS_PHOTO);
}
#endif

static Enna_Vfs_File *
_class_vfs_get(void *cookie)
{
    return enna_vfs_create_directory(mod->current_id, NULL,
                                     eina_stringshare_add("icon/upnp"), NULL);
}

#ifdef BUILD_ACTIVITY_MUSIC
static Enna_Class_Vfs class_upnp_music = {
    ENNA_MODULE_NAME,
    10,
    N_("UPnP/DLNA media servers"),
    NULL,
    "icon/upnp",
    {
        NULL,
        NULL,
        music_class_browse_up,
        music_class_browse_down,
        _class_vfs_get,
    },
    NULL
};
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Enna_Class_Vfs class_upnp_video = {
    ENNA_MODULE_NAME,
    10,
    N_("UPnP/DLNA media servers"),
    NULL,
    "icon/upnp",
    {
        NULL,
        NULL,
        video_class_browse_up,
        video_class_browse_down,
        _class_vfs_get,
    },
    NULL
};
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Enna_Class_Vfs class_upnp_photo = {
    ENNA_MODULE_NAME,
    10,
    N_("UPnP/DLNA media servers"),
    NULL,
    "icon/upnp",
    {
        NULL,
        NULL,
        photo_class_browse_up,
        photo_class_browse_down,
        _class_vfs_get,
    },
    NULL
};
#endif

/* Device Callbacks */

static void
upnp_add_device(GUPnPControlPoint *cp, GUPnPDeviceProxy  *proxy)
{
    const char *type, *location, *udn;
    char *name, *model;
    upnp_media_server_t *srv;
    GUPnPServiceInfo *si;
    Eina_List *l;

    type = gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO(proxy));
    if (!g_pattern_match_simple(UPNP_MEDIA_SERVER, type))
        return;

    location = gupnp_device_info_get_location(GUPNP_DEVICE_INFO(proxy));
    udn      = gupnp_device_info_get_udn(GUPNP_DEVICE_INFO(proxy));
    name     = gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO(proxy));
    model    = gupnp_device_info_get_model_name(GUPNP_DEVICE_INFO(proxy));

    /* check if device is already known */
    EINA_LIST_FOREACH(mod->devices, l, srv)
        if (location && !strcmp(srv->location, location))
            return;

    srv              = calloc(1, sizeof(upnp_media_server_t));
    srv->info        = GUPNP_DEVICE_INFO(proxy);
    si               = gupnp_device_info_get_service(srv->info,
                                                     UPNP_CONTENT_DIR);
    srv->content_dir = GUPNP_SERVICE_PROXY(si);
    srv->type        = type     ? strdup(type)     : NULL;
    srv->location    = location ? strdup(location) : NULL;
    srv->udn         = udn      ? strdup(udn)      : NULL;
    srv->name        = name     ? strdup(name)     : NULL;
    srv->model       = model    ? strdup(model)    : NULL;

    pthread_mutex_lock(&mod->mutex);
    mod->devices = eina_list_append(mod->devices, srv);
    pthread_mutex_unlock(&mod->mutex);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             _("Found new UPnP device '%s (%s)'"), name, model);
}

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_upnp
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_upnp",
    N_("UPnP/DLNA module"),
    "icon/module",
    N_("This module allows Enna to browse contents from UPnP/DLNA media servers"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    GError *error;

    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_UPnP));
    mod->em = em;
    em->mod = mod;

    g_thread_init(NULL);
    g_type_init();

    pthread_mutex_init(&mod->mutex, NULL);
    pthread_mutex_init(&mod->mutex_id, NULL);

    /* bind upnp context to ecore */
    mod->mctx = g_main_context_default();
    ecore_main_loop_glib_integrate();

    error = NULL;
    mod->ctx = gupnp_context_new(mod->mctx, NULL, 0, &error);
    if (error)
    {
        g_error_free(error);
        return;
    }

    mod->cp = gupnp_control_point_new(mod->ctx, GSSDP_ALL_RESOURCES);
    if (!mod->cp)
    {
        g_object_unref(mod->ctx);
        return;
    }

    g_signal_connect(mod->cp, "device-proxy-available",
                     G_CALLBACK(upnp_add_device), NULL);

    gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(mod->cp), TRUE);

#ifdef BUILD_ACTIVITY_MUSIC
    enna_vfs_append(ENNA_MODULE_NAME, ENNA_CAPS_MUSIC, &class_upnp_music);
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    enna_vfs_append(ENNA_MODULE_NAME, ENNA_CAPS_VIDEO, &class_upnp_video);
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    enna_vfs_append(ENNA_MODULE_NAME, ENNA_CAPS_PHOTO, &class_upnp_photo);
#endif
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Module_UPnP *mod;
    upnp_media_server_t *srv;
    Eina_List *l;

    mod = em->mod;

    gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(mod->cp), FALSE);

    EINA_LIST_FOREACH(mod->devices, l, srv)
        upnp_media_server_free(srv);
    g_object_unref(mod->cp);
    g_object_unref(mod->ctx);

    upnp_current_id_set(NULL);
    pthread_mutex_destroy(&mod->mutex);
    pthread_mutex_destroy(&mod->mutex_id);
}
