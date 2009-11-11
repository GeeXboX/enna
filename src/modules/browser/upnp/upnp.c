/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
upnp_media_server_free (upnp_media_server_t *srv)
{
    if (!srv)
        return;

    g_object_unref (srv->info);
    g_object_unref (srv->content_dir);

    ENNA_FREE (srv->type);
    ENNA_FREE (srv->location);
    ENNA_FREE (srv->udn);
    ENNA_FREE (srv->name);
    ENNA_FREE (srv->model);
    free (srv);
}

static xmlNode *
xml_util_get_element (xmlNode *node, ...)
{
    va_list var_args;

    va_start (var_args, node);

    while (1)
    {
        const char *arg;

        arg = va_arg (var_args, const char *);
        if (!arg)
            break;

        for (node = node->children; node; node = node->next)
        {
            if (!node->name)
                continue;

            if (!strcmp (arg, (char *) node->name))
                break;
        }

        if (!node)
            break;
    }

    va_end (var_args);

    return node;
}

static Enna_Vfs_File *
didl_process_object (xmlNode *e, char *udn)
{
    char *id, *title;
    gboolean is_container;
    Enna_Vfs_File *f = NULL;

    id = gupnp_didl_lite_object_get_id (e);
    if (!id)
        goto err_id;

    title = gupnp_didl_lite_object_get_title (e);
    if (!title)
        goto err_title;

    is_container = gupnp_didl_lite_object_is_container (e);


    if (is_container)
    {
        char uri[1024];

        memset (uri, '\0', sizeof (uri));
        snprintf (uri, sizeof (uri), "%s/%s", mod->current_id, id);

        f = enna_vfs_create_directory (uri, title, "icon/directory", NULL);

        enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                  "DIDL container '%s' (id: %s, uri: %s)", title, id, uri);
    }
    else
    {
        char *class_name, *uri;
        char *icon = NULL;
        GList *resources;
        xmlNode *n;

        class_name = gupnp_didl_lite_object_get_upnp_class (e);
        if (!class_name)
            goto err_no_class;

        if (!strncmp (class_name,
                      ITEM_CLASS_IMAGE, strlen (ITEM_CLASS_IMAGE)))
            icon = "icon/photo";
        else if (!strncmp (class_name,
                           ITEM_CLASS_AUDIO, strlen (ITEM_CLASS_AUDIO)))
            icon = "icon/music";
        else if (!strncmp (class_name,
                           ITEM_CLASS_VIDEO, strlen (ITEM_CLASS_VIDEO)))
            icon = "icon/video";

        g_free (class_name);

    err_no_class:
        if (!icon)
            icon = "icon/hd";

        resources = gupnp_didl_lite_object_get_property (e, "res");
        if (!resources)
            goto err_resources;

        n = (xmlNode *) resources->data;
        if (!n)
            goto err_res_node;

        uri = gupnp_didl_lite_property_get_value (n);
        if (!uri)
            goto err_no_uri;

        f = enna_vfs_create_file (uri, title, icon, NULL);

        enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                  "DIDL item '%s' (id: %s, uri: %s)", title, id, uri);

    err_no_uri:
    err_res_node:
        g_list_free (resources);
    }

 err_resources:
    g_free (title);
 err_title:
    g_free (id);
 err_id:
    return f;
}

static Eina_List *
didl_process (char *didl, char *udn)
{
    xmlNode *element;
    xmlDoc  *doc;
    Eina_List *list = NULL;

    doc = xmlParseMemory (didl, strlen (didl));
    if (!doc)
    {
        enna_log (ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                  "Unable to parse DIDL-Lite XML:\n%s", didl);
        return NULL;
    }

    /* Get a pointer to root element */
    element = xml_util_get_element ((xmlNode *) doc, "DIDL-Lite", NULL);
    if (!element)
    {
        enna_log (ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                  "No 'DIDL-Lite' node found.");
        goto err_element;
    }

    for (element = element->children; element; element = element->next)
    {
        const char *name = (const char *) element->name;

        if (!g_ascii_strcasecmp (name, "container") ||
            !g_ascii_strcasecmp (name, "item"))
        {
            Enna_Vfs_File *f;

            f = didl_process_object (element, udn);
            if (f)
                list = eina_list_append (list, f);
        }
    }

 err_element:
    xmlFreeDoc (doc);
    return list;
}

static Eina_List *
upnp_browse (upnp_media_server_t *srv, const char *container_id,
             guint32 starting_index, guint32 requested_count)
{
    guint32 number_returned, total_matches, si;
    Eina_List *results = NULL;
    char *didl_xml;
    GError *error;

    g_object_ref (srv->content_dir);
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

        info = GUPNP_SERVICE_INFO (srv->content_dir);
        enna_log (ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                  "Failed to browse '%s': %s",
                  gupnp_service_info_get_location (info), error->message);
        g_error_free (error);
        goto err;
    }

    if (didl_xml)
    {
        guint32 remaining;
        Eina_List *list;

        list = didl_process (didl_xml, srv->udn);
        results = eina_list_merge (results, list);
        g_free (didl_xml);

        /* See if we have more objects to get */
        si += number_returned;
        remaining = total_matches - si;

        /* Keep browsing till we get each and every object */
        if (remaining)
        {
            list = upnp_browse (srv, container_id, si,
                                MIN (remaining, UPNP_MAX_BROWSE));
            results = eina_list_merge (results, list);
        }
    }

 err:
    g_object_unref (srv->content_dir);
    return results;
}

static Eina_List *
browse_server_list (const char *uri)
{
    upnp_media_server_t *srv = NULL;
    char udn[512], id[512];
    char *container_id;
    int res;
    Eina_List *l;

    memset (udn, '\0', sizeof (udn));
    memset (id, '\0', sizeof (id));

    if (!uri)
        return NULL;

    res = sscanf (uri, "udn:%[^,],id:%[^,]", udn, id);
    if (res != 2)
        return NULL;

    container_id = strrchr (id, '/');
    if (!container_id)
        container_id = UPNP_DEFAULT_ROOT;
    else
        container_id++;

    EINA_LIST_FOREACH(mod->devices, l, srv)
        if (!strcmp (srv->udn, udn))
            break;

    /* no server to browse */
    if (!srv)
        return NULL;

    /* memorize our position */
    ENNA_FREE (mod->current_id);
    mod->current_id = strdup (uri);

    return upnp_browse (srv, container_id, 0, UPNP_MAX_BROWSE);
}

static Eina_List *
upnp_list_mediaservers (void)
{
    Eina_List *servers = NULL;
    Eina_List *l;
    upnp_media_server_t *srv;

    EINA_LIST_FOREACH(mod->devices, l, srv)
    {

        char name[256], uri[1024];
        Enna_Vfs_File *f;

        memset (name, '\0', sizeof (name));
        snprintf (name, sizeof (name), "%s (%s)", srv->name, srv->model);

        memset (uri, '\0', sizeof (uri));
        snprintf (uri, sizeof (uri), "udn:%s,id:%s",
                  srv->udn, UPNP_DEFAULT_ROOT);

        f = enna_vfs_create_directory (uri, name, "icon/dev/nfs", NULL);
        servers = eina_list_append (servers, f);
    }

    return servers;
}

static Eina_List *
_class_browse_up (const char *id, void *cookie)
{
    if (!id)
    {
        /* list available UPnP media servers */
        ENNA_FREE (mod->current_id);
        return upnp_list_mediaservers ();
    }

    /* browse content from a given media server */
    return browse_server_list (id);
}

static Eina_List *
_class_browse_down (void *cookie)
{
    char *prev_id;
    char new_id[512] = { 0 };
    int i, len;

    /* no recorded position, try root */
    if (!mod->current_id)
        return upnp_list_mediaservers ();

    /* try to guess where we come from */
    prev_id = strrchr (mod->current_id, '/');
    if (!prev_id)
        return upnp_list_mediaservers ();

    len = strlen (mod->current_id) - strlen (prev_id);
    for (i = 0; i < len; i++)
        new_id[i] = mod->current_id[i];

    return browse_server_list (new_id);
}

static Enna_Vfs_File *
_class_vfs_get (void *cookie)
{
    return enna_vfs_create_directory (NULL, NULL,
            eina_stringshare_add ("icon/upnp"), NULL);
}

static Enna_Class_Vfs class_upnp = {
    ENNA_MODULE_NAME,
    10,
    N_("UPnP/DLNA Media Servers"),
    NULL,
    "icon/upnp",
    {
        NULL,
        NULL,
        _class_browse_up,
        _class_browse_down,
        _class_vfs_get,
    },
    NULL
};

/* Device Callbacks */

static void
upnp_add_device (GUPnPControlPoint *cp, GUPnPDeviceProxy  *proxy)
{
    const char *type, *location, *udn;
    char *name, *model;
    upnp_media_server_t *srv;
    GUPnPServiceInfo *si;
    Eina_List *l;

    type = gupnp_device_info_get_device_type (GUPNP_DEVICE_INFO (proxy));
    if (!g_pattern_match_simple (UPNP_MEDIA_SERVER, type))
        return;

    location = gupnp_device_info_get_location (GUPNP_DEVICE_INFO (proxy));
    udn      = gupnp_device_info_get_udn (GUPNP_DEVICE_INFO (proxy));
    name     = gupnp_device_info_get_friendly_name (GUPNP_DEVICE_INFO (proxy));
    model    = gupnp_device_info_get_model_name (GUPNP_DEVICE_INFO (proxy));

    /* check if device is already known */
    EINA_LIST_FOREACH(mod->devices, l, srv)
        if (location && !strcmp (srv->location, location))
            return;

    srv              = calloc (1, sizeof (upnp_media_server_t));
    srv->info        = GUPNP_DEVICE_INFO (proxy);
    si               = gupnp_device_info_get_service (srv->info,
                                                      UPNP_CONTENT_DIR);
    srv->content_dir = GUPNP_SERVICE_PROXY (si);
    srv->type        = type     ? strdup (type)     : NULL;
    srv->location    = location ? strdup (location) : NULL;
    srv->udn         = udn      ? strdup (udn)      : NULL;
    srv->name        = name     ? strdup (name)     : NULL;
    srv->model       = model    ? strdup (model)    : NULL;

    pthread_mutex_lock (&mod->mutex);
    mod->devices = eina_list_append (mod->devices, srv);
    pthread_mutex_unlock (&mod->mutex);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              _("Found new UPnP device '%s (%s)'"), name, model);
}

/* Module interface */

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "browser_upnp",
    N_("UPnP / DLNA module"),
    "icon/module",
    N_("This module allows Enna to browse contents from UPnP/DLNA Media Servers"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void module_init (Enna_Module *em)
{
    int flags = ENNA_CAPS_MUSIC | ENNA_CAPS_VIDEO | ENNA_CAPS_PHOTO;
    GError *error;

    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_UPnP));
    mod->em = em;
    em->mod = mod;

    g_thread_init (NULL);
    g_type_init ();

    pthread_mutex_init (&mod->mutex, NULL);

    /* bind upnp context to ecore */
    mod->mctx = g_main_context_default ();
    ecore_main_loop_glib_integrate ();

    error = NULL;
    mod->ctx = gupnp_context_new (mod->mctx, NULL, 0, &error);
    if (error)
    {
        g_error_free (error);
        return;
    }

    mod->cp = gupnp_control_point_new (mod->ctx, GSSDP_ALL_RESOURCES);
    if (!mod->cp)
    {
        g_object_unref (mod->ctx);
        return;
    }

    g_signal_connect (mod->cp, "device-proxy-available",
                      G_CALLBACK (upnp_add_device), NULL);

    gssdp_resource_browser_set_active
        (GSSDP_RESOURCE_BROWSER (mod->cp), TRUE);

    enna_vfs_append (ENNA_MODULE_NAME, flags, &class_upnp);
}

void module_shutdown (Enna_Module *em)
{
    Enna_Module_UPnP *mod;
    upnp_media_server_t *srv;
    Eina_List *l;

    mod = em->mod;

    gssdp_resource_browser_set_active
        (GSSDP_RESOURCE_BROWSER (mod->cp), FALSE);

    EINA_LIST_FOREACH(mod->devices, l, srv)
        upnp_media_server_free(srv);
    g_object_unref (mod->cp);
    g_object_unref (mod->ctx);

    ENNA_FREE (mod->current_id);
    pthread_mutex_destroy (&mod->mutex);
}
