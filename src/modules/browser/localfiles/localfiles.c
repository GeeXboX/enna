/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2010 The Enna Project
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
#include "buffer.h"

#define ENNA_MODULE_NAME "localfiles"

typedef struct _Root_Directories
{
    const char *name;
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
    const char *name;
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

static void
_add_volumes_cb(void *data, Enna_Volume *v)
{
    Class_Private_Data *priv = data;
    Root_Directories *root;

    if (!strstr(v->mount_point, "file://"))
        return;

    root = calloc(1, sizeof(Root_Directories));
    root->name = eina_stringshare_add(v->label);

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
             ENNA_VFS_CAPS caps, char *key)
{
    Class_Private_Data *data;
    Root_Directories *root;
    char buf[PATH_MAX + 7];
    Eina_List *path_list;
    Eina_List *l;
    Enna_Volume *v;

    data = calloc(1, sizeof(Class_Private_Data));
    *priv = data;

    data->prev_uri = NULL;
    data->name = eina_stringshare_add(name);
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

    for (l = path_list; l; l = l->next)
    {
        localfiles_path_t *p;

        p = l->data;

        root        = calloc(1, sizeof(Root_Directories));
        root->name  = eina_stringshare_add(p->label);
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
        root->name = eina_stringshare_add(v->device_name);
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
        root->name = eina_stringshare_add("Home");
        root->uri = strdup(buf);
        root->label = strdup("Home");
        root->icon = strdup("icon/home");

        data->config->root_directories =
            eina_list_append(data->config->root_directories, root);
    }

    /* add localfiles to the list of volumes listener */
    data->vl = enna_volumes_listener_add("localfiles", _add_volumes_cb,
                                         _remove_volumes_cb, data);
}

static void
_add_child_volume_cb(void *data, Enna_Volume *v)
{
    Enna_Browser *b = data;
    Enna_File *f;

    Enna_Buffer *buf;

    buf = enna_buffer_new();
    enna_buffer_appendf(buf, "/%s/localfiles/%s", "music", v->label);
    f = enna_browser_create_menu(v->label, buf->buf,
                                 v->label, "icon/hd");
    enna_buffer_free(buf);
    enna_browser_file_add(b, f);
}

static void
_remove_child_volume_cb(void *data, Enna_Volume *v)
{
    Enna_Browser *b = data;
    Eina_List *files, *l;
    Enna_File *file;

    files = enna_browser_files_get(b);
    EINA_LIST_FOREACH(files, l, file)
    {
        if (file->name == v->label)
        {
            enna_browser_file_del(b, file);
        }
    }
}

static void *
_add(Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    Enna_Volumes_Listener *vl = NULL;
    if (eina_list_count(tokens) == 2 )
    {
        vl = enna_volumes_listener_add("localfiles_refresh", _add_child_volume_cb,
                                       _remove_child_volume_cb, browser);
    }
    return vl;
}

static void
_get_children(void *priv, Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    Eina_List *l;
    Class_Private_Data *pmod = NULL;

    switch(caps)
    {
        case  ENNA_CAPS_MUSIC:
        #ifdef BUILD_ACTIVITY_MUSIC
            pmod = mod->music;
        #endif
            break;
        case ENNA_CAPS_VIDEO:
        #ifdef BUILD_ACTIVITY_VIDEO
            pmod = mod->video;
        #endif
            break;
        case ENNA_CAPS_PHOTO:
        #ifdef BUILD_ACTIVITY_PHOTO
            pmod = mod->photo;
        #endif
            break;
        default:
            break;
    }

    if (!pmod)
        return;

    if (eina_list_count(tokens) == 2 )
    {
        //DBG("Browse Root\n");
        for (l = pmod->config->root_directories; l; l = l->next)
        {
            Enna_File *f;
            Root_Directories *root;
            Enna_Buffer *buf;

            root = l->data;

            buf = enna_buffer_new();
            EVT("Root name : %s\n", root->name);
            enna_buffer_appendf(buf, "/%s/localfiles/%s", pmod->name, root->name);
            f = enna_browser_create_menu(root->name, buf->buf,
                                              root->label, root->icon);
            enna_buffer_free(buf);
            enna_browser_file_add(browser, f);
            /* add localfiles to the list of volumes listener */
        }


    }
    else
    {
        const char *root_name = eina_list_nth(tokens, 2);
        Root_Directories *root = NULL;
        Enna_File *f;

        EINA_LIST_FOREACH(pmod->config->root_directories, l, root)
        {
            if (!strcmp(root->name, root_name))
            {
                Eina_List *files = NULL;
                Eina_List *l;
                char *filename = NULL;
                Eina_List *files_list = NULL;
                Eina_List *dirs_list = NULL;
                Enna_Buffer *path;
                Enna_Buffer *relative_path;
                char *tmp;
                char dir[PATH_MAX];
                Eina_List *l_tmp;


                path = enna_buffer_new();
                relative_path = enna_buffer_new();
                enna_buffer_appendf(path, "%s", root->uri + 7);
                /* Remove the Root Name (1st Item) from the list received */
               // DBG("Tokens : %d\n", eina_list_count(p->tokens));
               // EINA_LIST_FOREACH(p->tokens, l, tmp)
               //     DBG(tmp);

                l_tmp = eina_list_nth_list(tokens, 3);
                EINA_LIST_FOREACH(l_tmp, l, tmp)
                {
                    //DBG("Append : /%s to %s\n", tmp, path->buf);
                    enna_buffer_appendf(path, "/%s", tmp);
                    enna_buffer_appendf(relative_path, "%s/", tmp);
                }
                files = ecore_file_ls(path->buf);

                /* If no file found return immediatly*/
                if (!files)
                {
                    enna_buffer_free(path);
                    return;
                }
                files = eina_list_sort(files, eina_list_count(files),
                                       EINA_COMPARE_CB(strcasecmp));
                EINA_LIST_FREE(files, filename)
                {
                    sprintf(dir, "%s/%s", path->buf, filename);
                    if (filename[0] == '.')
                        continue;
                    else if (ecore_file_is_dir(dir))
                    {
                        Enna_File *f;
                        Enna_Buffer *buf;

                        buf = enna_buffer_new();
                        relative_path->buf ?
                            enna_buffer_appendf(buf, "/%s/localfiles/%s/%s%s", pmod->name, root->name, relative_path->buf, filename) :
                            enna_buffer_appendf(buf, "/%s/localfiles/%s/%s", pmod->name, root->name, filename);

                        f = enna_browser_create_directory(filename, buf->buf, filename, "icon/directory");
                        enna_buffer_free(buf);
                        dirs_list = eina_list_append(dirs_list, f);
                    }
                    else if (enna_util_uri_has_extension(dir, caps))
                    {
                        Enna_Buffer *buf;
                        Enna_Buffer *mrl;
                        Enna_File *f;

                        buf = enna_buffer_new();
                        relative_path->buf ?
                            enna_buffer_appendf(buf, "/%s/localfiles/%s/%s%s", pmod->name, root->name, relative_path->buf, filename) :
                            enna_buffer_appendf(buf, "/%s/localfiles/%s/%s", pmod->name, root->name, filename);

                        mrl = enna_buffer_new();
                        /* TODO : remove file:// on top of root->uri */
                        relative_path->buf ?
                            enna_buffer_appendf(mrl, "%s/%s%s", root->uri, relative_path->buf, filename):
                            enna_buffer_appendf(mrl, "%s/%s", root->uri, filename);

                        f = enna_file_file_add(filename, buf->buf,
                                                     mrl->buf, filename,
                                                     "icon/music");
                        enna_buffer_free(mrl);
                        enna_buffer_free(buf);

                        files_list = eina_list_append(files_list, f);
                    }
                }
                /* File after dir */
                dirs_list = eina_list_merge(dirs_list, files_list);

                if (!eina_list_count(dirs_list))
                {
                    enna_browser_file_add(browser, NULL);
                }
                else
                {
                    EINA_LIST_FREE(dirs_list, f)
                        enna_browser_file_add(browser, f);
                }
                enna_buffer_free(path);
                enna_buffer_free(relative_path);
                return;
            }
        }
    }
    return;
}

static void
_del(void *priv)
{
    Enna_Volumes_Listener *vl = priv;
    if (vl)
        enna_volumes_listener_del(vl);
}

static Enna_Vfs_Class class = {
    "localfiles",
    1,
    N_("Browse local devices"),
    NULL,
    "icon/hd",
    {
        _add,
        _get_children,
        _del
    },
    NULL
};

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

static void
module_init(Enna_Module *em)
{
    int flags = 0;

    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_LocalFiles));
    mod->em = em;
    em->mod = mod;

    enna_config_section_parser_register(&cfg_localfiles);
    cfg_localfiles_section_set_default();
    cfg_localfiles_section_load(cfg_localfiles.section);

#ifdef BUILD_ACTIVITY_MUSIC
    flags |= ENNA_CAPS_MUSIC;
    __class_init("music", &mod->music, ENNA_CAPS_MUSIC, "path_music");
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    flags |= ENNA_CAPS_VIDEO;
    __class_init("video", &mod->video, ENNA_CAPS_VIDEO, "path_video");
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    flags |= ENNA_CAPS_PHOTO;
    __class_init("photo", &mod->photo, ENNA_CAPS_PHOTO, "path_photo");
#endif
    enna_vfs_register(&class, flags);
}

static void
module_shutdown(Enna_Module *em)
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
    ENNA_FREE(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_localfiles",
    N_("Browse local files"),
    "icon/hd",
    N_("Browse files in your local file systems"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

