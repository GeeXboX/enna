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
#include "module.h"
#include "vfs.h"
#include "volumes.h"

#define ENNA_MODULE_NAME "dvd"



typedef struct _Class_Private_Data
{
    const char *uri;
    const char *device;
    unsigned char state;
    Enna_Volumes_Listener *vl;
} Class_Private_Data;

typedef struct _Enna_Module_Dvd
{
    Evas *e;
    Enna_Module *em;
    Class_Private_Data *dvd;

} Enna_Module_Dvd;

static Enna_Module_Dvd *mod;

static Eina_List *
_class_browse_up(const char *path, void *cookie)
{
    Enna_Vfs_File *f;
    Eina_List *l = NULL;
    char uri[4096];

    snprintf(uri, sizeof(uri), "dvd://%s", mod->dvd->device);
    f = enna_vfs_create_file(uri, _("Play"), "icon/video", NULL);
    l = eina_list_append(l, f);
    snprintf(uri, sizeof(uri), "dvdnav://%s", mod->dvd->device);
    f = enna_vfs_create_file(uri, _("Play (With Menus)"), "icon/video", NULL);
    l = eina_list_append(l, f);
    return l;
}


static Eina_List *
_class_browse_down(void *cookie)
{
    return NULL;
}

static Enna_Vfs_File *
_class_vfs_get(void *cookie)
{

    return enna_vfs_create_directory(mod->dvd->uri,
                                     ecore_file_file_get(mod->dvd->uri),
                                     eina_stringshare_add("icon/dvd"), NULL);
}



static Enna_Class_Vfs class_dvd = {
    "dvd_dvd",
    0,
    N_("Watch DVD Video"),
    NULL,
    "icon/dev/dvd",
    {
        NULL,
        NULL,
        _class_browse_up,
        _class_browse_down,
        _class_vfs_get,
    },
    NULL
};

static void
_add_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_DVD_VIDEO)
    {
        mod->dvd->device = eina_stringshare_add(v->device_name);
        enna_vfs_append("dvd", ENNA_CAPS_VIDEO, &class_dvd);
    }
}

static void
_remove_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_DVD_VIDEO)
    {
        enna_vfs_class_remove("dvd", ENNA_CAPS_VIDEO);
    }
}



/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_dvd
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_dvd",
    N_("DVD module"),
    "icon/dev/dvd",
    N_("Watch movies from DVD"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Dvd));
    mod->em = em;
    em->mod = mod;

    mod->dvd = calloc(1, sizeof(Class_Private_Data));

    mod->dvd->vl =
        enna_volumes_listener_add ("dvd", _add_volumes_cb,
                                   _remove_volumes_cb, mod->dvd);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Module_Dvd *mod;

    mod = em->mod;
    free(mod->dvd);
}
