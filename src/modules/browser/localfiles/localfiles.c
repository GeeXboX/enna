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

typedef struct localfiles_path_s {
    char *uri;
    char *label;
    char *icon;
} localfiles_path_t;

typedef struct localfiles_cfg_s {
    Eina_List *path_music;
    Eina_List *path_video;
    Eina_List *path_photo;
    Eina_Bool home;
} localfiles_cfg_t;

static localfiles_cfg_t localfiles_cfg;
static Enna_Module_LocalFiles *mod;

static localfiles_path_t *
localfiles_path_new (const char *uri, const char *label, const char *icon)
{
    localfiles_path_t *p;

    if (!uri || !label || !icon)
        return NULL;

    p        = calloc(1, sizeof(localfiles_path_t));
    p->uri   = strdup(uri);
    p->label = strdup(label);
    p->icon  = strdup(icon);

    return p;
}

static void
localfiles_path_free (localfiles_path_t *p)
{
    if (!p)
        return;

    ENNA_FREE(p->uri);
    ENNA_FREE(p->label);
    ENNA_FREE(p->icon);
    ENNA_FREE(p);
}

static unsigned char
_uri_is_root(Class_Private_Data *data, const char *uri)
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

static Eina_List *
_class_browse_up(const char *path, ENNA_VFS_CAPS caps,
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
                                        root->icon ? root->icon : "icon/hd",
                                        NULL);
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

        files = eina_list_sort(files, eina_list_count(files),
                               EINA_COMPARE_CB(strcasecmp));
        EINA_LIST_FOREACH(files, l, filename)
        {
            sprintf(dir, "%s/%s", path, filename);
            if (filename[0] == '.')
                continue;
            else if (ecore_file_is_dir(dir+7))
            {
                Enna_Vfs_File *f;

                f = enna_vfs_create_directory(dir, filename,
                                              "icon/directory", NULL);
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
static Eina_List *
_class_browse_up_music(const char *path, void *cookie)
{
    return _class_browse_up(path, ENNA_CAPS_MUSIC,
                            mod->music, "icon/file/music");
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Eina_List *
_class_browse_up_video(const char *path, void *cookie)
{
    return _class_browse_up(path, ENNA_CAPS_VIDEO,
                            mod->video, "icon/file/video");
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Eina_List *
_class_browse_up_photo(const char *path, void *cookie)
{
    return _class_browse_up(path, ENNA_CAPS_PHOTO,
                            mod->photo, "icon/file/photo");
}
#endif

static Eina_List *
_class_browse_down(Class_Private_Data *data, ENNA_VFS_CAPS caps)
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
static Eina_List *
_class_browse_down_music(void *cookie)
{
    return _class_browse_down(mod->music, ENNA_CAPS_MUSIC);
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Eina_List *
_class_browse_down_video(void *cookie)
{
    return _class_browse_down(mod->video, ENNA_CAPS_VIDEO);
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Eina_List *
_class_browse_down_photo(void *cookie)
{
    return _class_browse_down(mod->photo, ENNA_CAPS_PHOTO);
}
#endif

static Enna_Vfs_File *
_class_vfs_get(int type)
{
    switch (type)
    {
#ifdef BUILD_ACTIVITY_MUSIC
    case ENNA_CAPS_MUSIC:
        return enna_vfs_create_directory(mod->music->uri,
                                         ecore_file_file_get(mod->music->uri),
                                         eina_stringshare_add("icon/music"),
                                         NULL);
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    case ENNA_CAPS_VIDEO:
        return enna_vfs_create_directory(mod->video->uri,
                                         ecore_file_file_get(mod->video->uri),
                                         eina_stringshare_add("icon/video"),
                                         NULL);
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    case ENNA_CAPS_PHOTO:
        return enna_vfs_create_directory(mod->photo->uri,
                                         ecore_file_file_get(mod->photo->uri),
                                         eina_stringshare_add("icon/photo"),
                                         NULL);
#endif
    default:
        break;
    }

    return NULL;
}

#ifdef BUILD_ACTIVITY_MUSIC
static Enna_Vfs_File *
_class_vfs_get_music(void *cookie)
{
    return _class_vfs_get(ENNA_CAPS_MUSIC);
}
#endif

#ifdef BUILD_ACTIVITY_VIDEO
static Enna_Vfs_File *
_class_vfs_get_video(void *cookie)
{
    return _class_vfs_get(ENNA_CAPS_VIDEO);
}
#endif

#ifdef BUILD_ACTIVITY_PHOTO
static Enna_Vfs_File *
_class_vfs_get_photo(void *cookie)
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
    priv->config->root_directories =
        eina_list_append(priv->config->root_directories, root);
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
            priv->config->root_directories =
                eina_list_remove(priv->config->root_directories, root);
            ENNA_FREE(root->uri);
            ENNA_FREE(root->label);
            ENNA_FREE(root->icon);
            ENNA_FREE(root);
        }
    }
}

static void
__class_init(const char *name, Class_Private_Data **priv,
             ENNA_VFS_CAPS caps, Enna_Class_Vfs *class, char *key)
{
    Class_Private_Data *data;
    Root_Directories *root;
    char buf[PATH_MAX + 7];
    Eina_List *path_list;
    Eina_List *l;
    Enna_Volume *v;

    data = calloc(1, sizeof(Class_Private_Data));
    *priv = data;

    enna_vfs_append(name, caps, class);
    data->prev_uri = NULL;

    data->config = calloc(1, sizeof(Module_Config));
    data->config->root_directories = NULL;

    switch (caps)
    {
    case ENNA_CAPS_MUSIC:
        path_list = localfiles_cfg.path_music;
        break;
    case ENNA_CAPS_VIDEO:
        path_list = localfiles_cfg.path_video;
        break;
    case ENNA_CAPS_PHOTO:
        path_list = localfiles_cfg.path_photo;
        break;
    default:
        return;
    }

    if (!path_list)
        return;

    for (l = path_list; l; l = l->next)
    {
        localfiles_path_t *p;

        p = l->data;

        root        = calloc(1, sizeof(Root_Directories));
        root->uri   = strdup(p->uri);
        root->label = strdup(p->label);
        root->icon  = strdup(p->icon);

        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "Root Data: %s", root->uri);
        data->config->root_directories =
            eina_list_append(data->config->root_directories, root);
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
        data->config->root_directories =
            eina_list_append(data->config->root_directories, root);
    }

    if (localfiles_cfg.home)
    {
        // add home directory entry
        root = ENNA_NEW(Root_Directories, 1);
        snprintf(buf, sizeof(buf), "file://%s", enna_util_user_home_get());
        root->uri = strdup(buf);
        root->label = strdup("Home");
        root->icon = strdup("icon/favorite");

        data->config->root_directories =
            eina_list_append(data->config->root_directories, root);
    }

    /* add localfiles to the list of volumes listener */
    data->vl = enna_volumes_listener_add("localfiles", _add_volumes_cb,
                                         _remove_volumes_cb, data);
}

#ifdef BUILD_ACTIVITY_MUSIC
static Enna_Class_Vfs class_music = {
    "localfiles_music",
    1,
    N_("Browse local devices"),
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
    N_("Browse local devices"),
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
    N_("Browse local devices"),
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

static Eina_List *
cfg_localfiles_section_list_get(const char *section, const char *key)
{
    Eina_List *vl;
    Eina_List *list = NULL;

    if (!section || !key)
        return NULL;

    vl = enna_config_string_list_get(section, key);
    if (vl)
    {
        Eina_List *l;
        char *c;

        EINA_LIST_FOREACH(vl, l, c)
        {
            Eina_List *tuple;

            tuple = enna_util_tuple_get(c, ",");

            /* ensure that name and icon were specified */
            if (tuple && eina_list_count(tuple) == 1)
            {
                const char *l = ecore_file_file_get(eina_list_nth(tuple, 0));
                const char *i = "icon/favorite";
                tuple = eina_list_append(tuple, strdup(l));
                tuple = eina_list_append(tuple, strdup(i));
            }

            if (tuple && eina_list_count(tuple) == 3)
            {
                localfiles_path_t *p;
                const char *uri, *label, *icon;

                uri   = eina_list_nth(tuple, 0);
                label = eina_list_nth(tuple, 1);
                icon  = eina_list_nth(tuple, 2);

                p = localfiles_path_new (uri, label, icon);
                list = eina_list_append(list, p);
            }
        }
    }

    return list;
}

static void
cfg_localfiles_section_list_set(const char *section,
                                const char *key, Eina_List *list)
{
    Eina_List *vl = NULL, *l;
    localfiles_path_t *p;

    if (!section || !key || !list)
        return;

    EINA_LIST_FOREACH(list, l, p)
    {
        char v[1024];

        snprintf (v, sizeof(v), "%s,%s,%s", p->uri, p->label, p->icon);
        vl = eina_list_append(vl, strdup(v));
    }

    enna_config_string_list_set(section, key, vl);
}

static void
cfg_localfiles_free(void)
{
    localfiles_path_t *p;

    EINA_LIST_FREE(localfiles_cfg.path_music, p)
        localfiles_path_free(p);

    EINA_LIST_FREE(localfiles_cfg.path_video, p)
        localfiles_path_free(p);

    EINA_LIST_FREE(localfiles_cfg.path_photo, p)
        localfiles_path_free(p);
}

static void
cfg_localfiles_section_load(const char *section)
{
    cfg_localfiles_free();

    localfiles_cfg.path_music =
        cfg_localfiles_section_list_get(section, "path_music");
    localfiles_cfg.path_video =
        cfg_localfiles_section_list_get(section, "path_video");
    localfiles_cfg.path_photo =
        cfg_localfiles_section_list_get(section, "path_photo");
    localfiles_cfg.home = enna_config_bool_get(section, "display_home");
}

static void
cfg_localfiles_section_save(const char *section)
{
    cfg_localfiles_section_list_set(section,
                                    "path_music", localfiles_cfg.path_music);
    cfg_localfiles_section_list_set(section,
                                    "path_video", localfiles_cfg.path_video);
    cfg_localfiles_section_list_set(section,
                                    "path_photo", localfiles_cfg.path_photo);
    enna_config_bool_set(section, "display_home", localfiles_cfg.home);
}

static void
cfg_localfiles_section_set_default(void)
{
    cfg_localfiles_free();

    localfiles_cfg.path_music = NULL;
    localfiles_cfg.path_video = NULL;
    localfiles_cfg.path_photo = NULL;
    localfiles_cfg.home = EINA_TRUE;
}

static Enna_Config_Section_Parser cfg_localfiles = {
    "localfiles",
    cfg_localfiles_section_load,
    cfg_localfiles_section_save,
    cfg_localfiles_section_set_default,
    cfg_localfiles_free,
};

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
    N_("Browse files in your local file systems"),
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

    enna_config_section_parser_register(&cfg_localfiles);
    cfg_localfiles_section_set_default();
    cfg_localfiles_section_load(cfg_localfiles.section);

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
