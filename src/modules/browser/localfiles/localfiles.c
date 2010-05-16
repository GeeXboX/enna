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

static void
_add_volumes_cb(void *data, Enna_Volume *v)
{
    Class_Private_Data *priv = data;
    Root_Directories *root;

    if (!strstr(v->mount_point, "file://"))
        return;

    root = calloc(1, sizeof(Root_Directories));
    root->name = eina_stringshare_add(v->device_name);
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
        root->icon = strdup("icon/favorite");

        data->config->root_directories =
            eina_list_append(data->config->root_directories, root);
    }

    /* add localfiles to the list of volumes listener */
    data->vl = enna_volumes_listener_add("localfiles", _add_volumes_cb,
                                         _remove_volumes_cb, data);
}



typedef struct _Enna_Localfiles_Priv
{
    Eina_List *tokens;
    void *(*add_file)(void *data, Enna_Vfs_File *file);
    void *data;
    ENNA_VFS_CAPS caps;
}Enna_Localfiles_Priv;

static void *
_add(Eina_List *tokens, ENNA_VFS_CAPS caps,void *(*add_file)(void *data, Enna_Vfs_File *file), void *data)
{
    Enna_Localfiles_Priv *p = calloc(1, sizeof(Enna_Localfiles_Priv));

    p->tokens = tokens;
    p->add_file = add_file;
    p->data = data;
    p->caps = caps;
    return p;
}

static void
_get_children(void *priv)
{
    Eina_List *l;
    buffer_t *buf;
    Enna_Localfiles_Priv *p = priv;
    Class_Private_Data *pmod;
    if (!p)
        return;

    if (p->caps == ENNA_CAPS_MUSIC)
        pmod = mod->music;
    else if (p->caps == ENNA_CAPS_VIDEO)
        pmod = mod->video;
    else if (p->caps == ENNA_CAPS_PHOTO)
        pmod = mod->photo;
    else
        return;

    if (eina_list_count(p->tokens) == 2 )
    {
        DBG("Browse Root\n");
        for (l = pmod->config->root_directories; l; l = l->next)
        {
            Enna_Vfs_File *f;
            Root_Directories *root;
            
            root = l->data;
            
            f = calloc(1, sizeof(Enna_Vfs_File));
            
            buf = buffer_new();
            buffer_appendf(buf, "/music/%s/%s", pmod->name, root->name);
            f->name = eina_stringshare_add(root->name);
            f->uri = eina_stringshare_add(buf->buf);
            buffer_free(buf);
            f->label = eina_stringshare_add(root->label);
            f->icon = eina_stringshare_add("icon/hd");
            f->is_menu = 1;

            p->add_file(p->data, f);
        }

    }
    else
    {
        const char *root_name = eina_list_nth(p->tokens, 2);
        Root_Directories *root = NULL;
        Enna_Vfs_File *f;
        
        EINA_LIST_FOREACH(pmod->config->root_directories, l, root)
        {
            if (!strcmp(root->name, root_name))
            {
                Eina_List *files = NULL;
                Eina_List *l;
                char *filename = NULL;
                Eina_List *files_list = NULL;
                Eina_List *dirs_list = NULL;
                buffer_t *path;
                buffer_t *relative_path;
                char *tmp;
                char dir[PATH_MAX];
                Eina_List *l_tmp;

                
                path = buffer_new();
                relative_path = buffer_new();
                buffer_appendf(path, "%s", root->uri + 7);
                /* Remove the Root Name (1st Item) from the list received */
                DBG("Tokens : %d\n", eina_list_count(p->tokens));
                EINA_LIST_FOREACH(p->tokens, l, tmp)
                    DBG(tmp);
                
                l_tmp = eina_list_nth_list(p->tokens, 3);
                EINA_LIST_FOREACH(l_tmp, l, tmp)
                {
                    DBG("Append : /%s to %s\n", tmp, path->buf);
                    buffer_appendf(path, "/%s", tmp);
                    buffer_appendf(relative_path, "%s/", tmp);
                }
                files = ecore_file_ls(path->buf);
                
                /* If no file found return immediatly*/
                if (!files)
                {
                    buffer_free(path);
                    return NULL;
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
                        Enna_Vfs_File *f;
                        buffer_t *buf;
                        f = calloc(1, sizeof(Enna_Vfs_File));

                        buf = buffer_new();
                        relative_path->buf ?
                            buffer_appendf(buf, "/music/%s/%s/%s/%s", pmod->name, root->name, relative_path->buf, filename) :
                            buffer_appendf(buf, "/music/%s/%s/%s", pmod->name, root->name, filename);
                            
                        f->name = eina_stringshare_add(filename);
                        f->uri = eina_stringshare_add(buf->buf);
                        buffer_free(buf);
                        f->label = eina_stringshare_add(filename);
                        f->icon = eina_stringshare_add("icon/directory");
                        f->is_directory = 1;

                        dirs_list = eina_list_append(dirs_list, f);
                    }
                    else if (enna_util_uri_has_extension(dir, p->caps))
                    {
                        buffer_t *mrl;
                        Enna_Vfs_File *f;
                        f = calloc(1, sizeof(Enna_Vfs_File));
                       
                        buf = buffer_new();
                        buffer_appendf(buf, "/music/%s/%s/%s/%s", pmod->name, root->name, relative_path->buf, filename);

                        mrl = buffer_new();
                        /* TODO : remove file:// on top of root->uri */
                        buffer_appendf(mrl, "%s/%s/%s", root->uri, relative_path->buf, filename);
                        f->mrl = eina_stringshare_add(mrl->buf);
                        buffer_free(mrl);

                        f->name = eina_stringshare_add(filename);
                        f->uri = eina_stringshare_add(buf->buf);
                        buffer_free(buf);
                        f->label = eina_stringshare_add(filename);
                        f->icon = eina_stringshare_add( "icon/music");

                        files_list = eina_list_append(files_list, f);
                    }
                }
                /* File after dir */
                for (l = files_list; l; l = l->next)
                {
                    dirs_list = eina_list_append(dirs_list, l->data);
                }

                EINA_LIST_FOREACH(dirs_list, l, f)
                    p->add_file(p->data, f);
                buffer_free(path);
                buffer_free(relative_path);
                return;
            }
        }
    }
    return;
}

static void
_del(void *priv)
{
    if (!priv)
        return;
}

static Enna_Class2_Vfs class2 = {
    "localfiles_music",
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
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_LocalFiles));
    mod->em = em;
    em->mod = mod;

    enna_config_section_parser_register(&cfg_localfiles);
    cfg_localfiles_section_set_default();
    cfg_localfiles_section_load(cfg_localfiles.section);

#ifdef BUILD_ACTIVITY_MUSIC
    __class_init("localfiles_music", &mod->music, ENNA_CAPS_MUSIC, "path_music");
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    __class_init("localfiles_video", &mod->video, ENNA_CAPS_VIDEO, "path_video");
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    __class_init("localfiles_photo", &mod->photo, ENNA_CAPS_PHOTO, "path_photo");
#endif
    enna_vfs_register(&class2);
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

