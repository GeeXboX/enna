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

static void
_get_children(void *priv, Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    Enna_File *f;
    char mrl[4096];
    
    snprintf(mrl, sizeof(mrl), "dvd://%s", mod->dvd->device);
    f = enna_file_file_add("play", "video/dvd/dvd", mrl, _("Play"), "icon/video");
    enna_browser_file_add(browser, f);
    snprintf(mrl, sizeof(mrl), "dvdnav://%s", mod->dvd->device);
    f = enna_file_file_add("dvd", "video/dvd/dvdnav", mrl, _("Play (with menus)"), "icon/video");
    enna_browser_file_add(browser, f);
    return;
}
static Enna_Vfs_Class class_dvd = {
    "dvd",
    0,
    N_("Watch DVD video"),
    NULL,
    "icon/dev/dvd",
    {
        NULL,
        _get_children,
        NULL
    },
    NULL
};


static void
_add_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_DVD_VIDEO)
    {
        mod->dvd->device = eina_stringshare_add(v->device_name);
        enna_vfs_register(&class_dvd, ENNA_CAPS_VIDEO);
    }
}

static void
_remove_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_DVD_VIDEO)
    {
        enna_vfs_unregister(&class_dvd, ENNA_CAPS_VIDEO);
    }
}



/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_dvd
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
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

static void
module_shutdown(Enna_Module *em)
{
    Enna_Module_Dvd *mod;

    mod = em->mod;
    free(mod->dvd);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_dvd",
    N_("DVD module"),
    "icon/dev/dvd",
    N_("Watch movies from DVD"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

