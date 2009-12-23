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

#include <string.h>

#include <Ecore.h>
#include <Ecore_File.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "vfs.h"
#include "volumes.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME "localfiles"

typedef struct _Root_Directories
{
    char *uri;
    char *label;
    char *icon;
} Root_Directories;

typedef struct _Module_Config
{
    Eina_List *root_directories;
} Module_Config;

typedef struct _Class_Private_Data
{
    const char *uri;
    const char *prev_uri;
    Module_Config *config;
    Enna_Volumes_Listener *vl;
} Class_Private_Data;

typedef struct _Enna_Module_LocalFiles
{
    Evas *e;
    Enna_Module *em;
#ifdef BUILD_ACTIVITY_MUSIC
    Class_Private_Data *music;
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    Class_Private_Data *video;
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    Class_Private_Data *photo;
#endif
} Enna_Module_LocalFiles;

static Enna_Module_LocalFiles *mod;

static unsigned char _uri_is_root(Class_Private_Data *data, const char *uri)
{
    Eina_List *l;

    for (l = data->config->root_directories; l; l = l->next)
    {
        Root_Directories *root = l->data;
        if (!strcmp(root->uri, uri))
            return 1;
    }

    return 0;
}

static Eina_List *_class_browse_up(const char *path, ENNA_VFS_CAPS caps,
        Class_Private_Data *data, char *icon)
{
    /* Browse Root */
    if (!path)
    {
        Eina_List *files = NULL;
        Eina_List *l;
        /* FIXME: this list should come from config ! */
        for (l = data->config->root_directories; l; l = l->next)
        {
            Enna_Vfs_File *file;
            Root_Directories *root;

            root = l->data;
            file = enna_vfs_create_menu(root->uri, root->label,
                                             root->icon ?
                                             root->icon : "icon/hd", NULL);
            files = eina_list_append(files, file);
        }
        //eina_stringshare_del(data->prev_uri);
        //eina_stringshare_del(data->uri);
        data->prev_uri = NULL;
        data->uri = NULL;
        return files;
    }
    else if (strstr(path, "file://"))
    {
        Eina_List *files = NULL;
        Eina_List *l;
        char *filename = NULL;
        Eina_List *files_list = NULL;
        Eina_List *dirs_list = NULL;

        char dir[PATH_MAX];

        files = ecore_file_ls(path+7);

        /* If no file found return immediatly*/
        if (!files)
            return NULL;

        files = eina_list_sort(files, eina_list_count(files), EINA_COMPARE_CB(strcasecmp));
        EINA_LIST_FOREACH(files, l, filename)
        {
            sprintf(dir, "%s/%s", path, filename);
            if (filename[0] == '.')
                continue;
            else if (ecore_file_is_dir(dir+7))
            {
                Enna_Vfs_File *f;

                f = enna_vfs_create_directory(dir, filename, "icon/directory", NULL);
                dirs_list = eina_list_append(dirs_list, f);
            }
            else if (enna_util_uri_has_extension(dir, caps))
            {
                Enna_Vfs_File *f;
                f = enna_vfs_create_file(dir, filename, icon, NULL);
                files_list = eina_list_append(files_list, f);
            }
        }
        /* File after dir */
        for (l = files_list; l; l = l->next)
        {
            dirs_list = eina_list_append(dirs_list, l->data);
        }
        //eina_stringshare_del(data->prev_uri);
        data->prev_uri = data->uri;
        data->uri = eina_stringshare_add(path);
        return dirs_list;
    }

    return NULL;
}

#ifdef BUILD_ACTIVITY_MUSIC
static Eina_List *_class_browse_up_music(const char *path, void *cookie)
{
    return _class_browse_up(path, ENNA_CAPS_MUSIC,
                            mod->music, "icon/file/music");
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Eina_List *_class_browse_up_video(const char *path, void *cookie)
{
    return _class_browse_up(path, ENNA_CAPS_VIDEO,
                            mod->video, "icon/file/video");
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Eina_List *_class_browse_up_photo(const char *path, void *cookie)
{
    return _class_browse_up(path, ENNA_CAPS_PHOTO,
                            mod->photo, "icon/file/photo");
}
#endif

static Eina_List * _class_browse_down(Class_Private_Data *data,
                                      ENNA_VFS_CAPS caps)
{
    /* Browse Root */
    if (data->uri && strstr(data->uri, "file://"))
    {
        char *path_tmp = NULL;
        char *p;
        Eina_List *files = NULL;

        if (_uri_is_root(data, data->uri))
        {
            Eina_List *files = NULL;
            Eina_List *l;
            for (l = data->config->root_directories; l; l = l->next)
            {
                Enna_Vfs_File *file;
                Root_Directories *root;

                root = l->data;
                file = enna_vfs_create_menu(root->uri, root->label,
                        root->icon ? root->icon : "icon/hd", NULL);
                files = eina_list_append(files, file);
            }
            data->prev_uri = NULL;
            data->uri = NULL;
            return files;
        }

        path_tmp = strdup(data->uri);
        if (path_tmp[strlen(data->uri) - 1] == '/')
            path_tmp[strlen(data->uri) - 1] = 0;
        p = strrchr(path_tmp, '/');
        if (p && *(p - 1) == '/')
            *(p) = 0;
        else if (p)
            *(p) = 0;

        files = _class_browse_up(path_tmp, caps, data, NULL);
        data->uri = eina_stringshare_add(path_tmp);
        return files;
    }

    return NULL;
}

#ifdef BUILD_ACTIVITY_MUSIC
static Eina_List * _class_browse_down_music(void *cookie)
{
    return _class_browse_down(mod->music, ENNA_CAPS_MUSIC);
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Eina_List * _class_browse_down_video(void *cookie)
{
    return _class_browse_down(mod->video, ENNA_CAPS_VIDEO);
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Eina_List * _class_browse_down_photo(void *cookie)
{
    return _class_browse_down(mod->photo, ENNA_CAPS_PHOTO);
}
#endif

static Enna_Vfs_File * _class_vfs_get(int type)
{
    switch (type)
    {
#ifdef BUILD_ACTIVITY_MUSIC
        case ENNA_CAPS_MUSIC:
            return enna_vfs_create_directory(mod->music->uri,
                    ecore_file_file_get(mod->music->uri),
                    eina_stringshare_add("icon/music"), NULL);
#endif
#ifdef BUILD_ACTIVITY_VIDEO
        case ENNA_CAPS_VIDEO:
            return enna_vfs_create_directory(mod->video->uri,
                    ecore_file_file_get(mod->video->uri),
                    eina_stringshare_add("icon/video"), NULL);
#endif
#ifdef BUILD_ACTIVITY_PHOTO
        case ENNA_CAPS_PHOTO:
            return enna_vfs_create_directory(mod->photo->uri,
                    ecore_file_file_get(mod->photo->uri),
                    eina_stringshare_add("icon/photo"), NULL);
#endif
        default:
            break;
    }

    return NULL;
}

#ifdef BUILD_ACTIVITY_MUSIC
static Enna_Vfs_File * _class_vfs_get_music(void *cookie)
{
    return _class_vfs_get(ENNA_CAPS_MUSIC);
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Enna_Vfs_File * _class_vfs_get_video(void *cookie)
{
    return _class_vfs_get(ENNA_CAPS_VIDEO);
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Enna_Vfs_File * _class_vfs_get_photo(void *cookie)
{
    return _class_vfs_get(ENNA_CAPS_PHOTO);
}
#endif

static void
_add_volumes_cb(void *data, Enna_Volume *v)
{
    Class_Private_Data *priv = data;
    Root_Directories *root;

    if (!strstr(v->mount_point, "file://"))
        return;

    root = calloc(1, sizeof(Root_Directories));
    root->uri = strdup( v->mount_point);
    root->label = strdup(v->label);
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
             "Root Data: %s", root->uri);
    root->icon = strdup(enna_volumes_icon_from_type(v));
    priv->config->root_directories = eina_list_append(
        priv->config->root_directories, root);
}

static void
_remove_volumes_cb(void *data, Enna_Volume *v)
{
    Class_Private_Data *priv = data;
    Root_Directories *root;
    Eina_List *l;

    EINA_LIST_FOREACH(priv->config->root_directories, l, root)
    {
        if (!strcmp(root->label, v->label))
        {
            priv->config->root_directories = eina_list_remove(
                priv->config->root_directories, root);
            ENNA_FREE(root->uri);
            ENNA_FREE(root->label);
            ENNA_FREE(root->icon);
            ENNA_FREE(root);
        }
    }
}

static void __class_init(const char *name, Class_Private_Data **priv,
                         ENNA_VFS_CAPS caps, Enna_Class_Vfs *class, char *key)
{
    Class_Private_Data *data;
    Enna_Config_Data *cfgdata;
    Root_Directories *root;
    char buf[PATH_MAX + 7];
    Eina_List *l;
    Enna_Volume *v;

    data = calloc(1, sizeof(Class_Private_Data));
    *priv = data;

    enna_vfs_append(name, caps, class);
    data->prev_uri = NULL;

    data->config = calloc(1, sizeof(Module_Config));
    data->config->root_directories = NULL;

    cfgdata = enna_config_module_pair_get("localfiles");
    if (!cfgdata)
        return;

    for (l = cfgdata->pair; l; l = l->next)
    {
        Config_Pair *pair = l->data;
        if (!strcmp(pair->key, key))
        {
            Eina_List *dir_data;
            enna_config_value_store(&dir_data, key, ENNA_CONFIG_STRING_LIST,
                                    pair);
            if (dir_data)
            {
                if (eina_list_count(dir_data) != 3)
                    continue;
                else
                {
                    root = calloc(1, sizeof(Root_Directories));
                    root->uri = eina_list_nth(dir_data, 0);
                    root->label = eina_list_nth(dir_data, 1);
                    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                             "Root Data: %s", root->uri);
                    root->icon = eina_list_nth(dir_data, 2);
                    data->config->root_directories = eina_list_append(
                        data->config->root_directories, root);
                }
            }
        }
    }
    /* Add All detected volumes */
    EINA_LIST_FOREACH(enna_volumes_get(), l, v)
    {
        root = calloc(1, sizeof(Root_Directories));
        snprintf(buf, sizeof(buf), "file://%s", v->mount_point);
        root->uri = strdup(buf);
        root->label = strdup(v->label);
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 "Root Data: %s", root->uri);
        root->icon = strdup(enna_volumes_icon_from_type(v));
        data->config->root_directories = eina_list_append(
            data->config->root_directories, root);
    }

    // add home directory entry
    root = ENNA_NEW(Root_Directories, 1);
    snprintf(buf, sizeof(buf), "file://%s", enna_util_user_home_get());
    root->uri = strdup(buf);
    root->label = strdup("Home");
    root->icon = strdup("icon/favorite");

    data->config->root_directories = eina_list_append(
        data->config->root_directories, root);

    /* add localfiles to the list of volumes listener */
    data->vl = enna_volumes_listener_add("localfiles", _add_volumes_cb, _remove_volumes_cb, data);
}

#ifdef BUILD_ACTIVITY_MUSIC
static Enna_Class_Vfs class_music = {
    "localfiles_music",
    1,
    N_("Browse Local Devices"),
    NULL,
    "icon/hd",
    {
        NULL,
        NULL,
        _class_browse_up_music,
        _class_browse_down_music,
        _class_vfs_get_music,
    },
    NULL
};
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Enna_Class_Vfs class_video = {
    "localfiles_video",
    1,
    N_("Browse Local Devices"),
    NULL,
    "icon/hd",
    {
        NULL,
        NULL,
        _class_browse_up_video,
        _class_browse_down_video,
        _class_vfs_get_video,
    },
    NULL
};
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Enna_Class_Vfs class_photo = {
    "localfiles_photo",
    1,
    N_("Browse Local Devices"),
    NULL,
    "icon/hd",
    {
        NULL,
        NULL,
        _class_browse_up_photo,
        _class_browse_down_photo,
        _class_vfs_get_photo,
    },
    NULL
};
#endif

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_localfiles
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_localfiles",
    N_("Browse local files"),
    "icon/hd",
    N_("Browse files in your local hard disk"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_LocalFiles));
    mod->em = em;
    em->mod = mod;

#ifdef BUILD_ACTIVITY_MUSIC
    __class_init("localfiles_music", &mod->music, ENNA_CAPS_MUSIC,
                 &class_music, "path_music");
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    __class_init("localfiles_video", &mod->video, ENNA_CAPS_VIDEO,
                 &class_video, "path_video");
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    __class_init("localfiles_photo", &mod->photo, ENNA_CAPS_PHOTO,
                 &class_photo, "path_photo");
#endif
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Module_LocalFiles *mod;

    mod = em->mod;
#ifdef BUILD_ACTIVITY_MUSIC
    enna_volumes_listener_del(mod->music->vl);
    free(mod->music);
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    enna_volumes_listener_del(mod->video->vl);
    free(mod->video);
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    enna_volumes_listener_del(mod->photo->vl);
    free(mod->photo);
#endif
}
