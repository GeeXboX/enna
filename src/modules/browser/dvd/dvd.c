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

static Eina_List *_class_browse_up(const char *path, void *cookie)
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


static Eina_List * _class_browse_down(void *cookie)
{
    return NULL;
}

static Enna_Vfs_File * _class_vfs_get(void *cookie)
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

static void _add_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_DVD_VIDEO)
    {
	mod->dvd->device = eina_stringshare_add(v->device_name);
        enna_vfs_append("dvd", ENNA_CAPS_VIDEO, &class_dvd);
    }
}

static void _remove_volumes_cb(void *data, Enna_Volume *v)
{
    if (v && v->type == VOLUME_TYPE_DVD_VIDEO)
    {
        enna_vfs_class_remove("dvd", ENNA_CAPS_VIDEO);
    }
}



/* Module interface */

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "browser_dvd",
    N_("DVD module"),
    "icon/dev/dvd",
    N_("Watch movies from DVD"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void module_init(Enna_Module *em)
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

void module_shutdown(Enna_Module *em)
{
    Enna_Module_Dvd *mod;

    mod = em->mod;
    free(mod->dvd);
}
