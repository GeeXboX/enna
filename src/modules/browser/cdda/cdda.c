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
#include "parser_cdda.h"

#define ENNA_MODULE_NAME "cdda"



typedef struct _Class_Private_Data
{
    const char *uri;
    const char *device;
    unsigned char state;
    Enna_Volumes_Listener *vl;
} Class_Private_Data;

typedef struct _Enna_Module_Cdda
{
    Evas *e;
    Enna_Module *em;
    Class_Private_Data *cdda;

} Enna_Module_Cdda;

static Enna_Module_Cdda *mod;

static Eina_List *_class_browse_up(const char *path, void *cookie)
{
    Enna_Vfs_File *f;
    Eina_List *l = NULL;
    char uri[4096];
    cdda_t *cd;
    int i;

    cd = cdda_parse(mod->cdda->device);
    if (!cd) return NULL;
    for (i = 0; i < cd->total_tracks; i++)
    {
	snprintf(uri, sizeof(uri), "cdda://%d/%s", i+1, mod->cdda->device);
	f = enna_vfs_create_file(eina_stringshare_add(uri), cd->tracks[i]->name, "icon/music", NULL);
	l = eina_list_append(l, f);
    }
    cdda_free(cd);

    return l;
}


static Eina_List * _class_browse_down(void *cookie)
{
    return NULL;
}

static Enna_Vfs_File * _class_vfs_get(void *cookie)
{

    return enna_vfs_create_directory(mod->cdda->uri,
        ecore_file_file_get(mod->cdda->uri),
        eina_stringshare_add("icon/cdda"), NULL);
}



static Enna_Class_Vfs class_cdda = {
    "cdda_cdda",
    0,
    N_("Play audio CD"),
    NULL,
    "icon/dev/cdda",
    {
        NULL,
        NULL,
        _class_browse_up,
        _class_browse_down,
        _class_vfs_get,
    },
    NULL
};

static void _add_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_CDDA)
    {
	mod->cdda->device = eina_stringshare_add(v->device_name);
        enna_vfs_append("cdda", ENNA_CAPS_MUSIC, &class_cdda);
    }
}

static void _remove_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_CDDA)
    {
        enna_vfs_class_remove("cdda", ENNA_CAPS_MUSIC);
    }
}



/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_cdda
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_cdda",
    N_("CDDA module"),
    "icon/dev/cdda",
    N_("Listen your favorite CDs"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Cdda));
    mod->em = em;
    em->mod = mod;

    mod->cdda = calloc(1, sizeof(Class_Private_Data));

    mod->cdda->vl =
        enna_volumes_listener_add ("cdda", _add_volumes_cb,
                                   _remove_volumes_cb, mod->cdda);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Module_Cdda *mod;

    mod = em->mod;
    free(mod->cdda);
}
