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
#include "parser_cdda.h"

#define ENNA_MODULE_NAME "cdda"



typedef struct _Class_Private_Data
{
    const char *uri;
    const char *device;
    unsigned char state;
    Ecore_Event_Handler *volume_add_handler;
    Ecore_Event_Handler *volume_remove_handler;
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
        evas_stringshare_add("icon/cdda"), NULL);
}



static Enna_Class_Vfs class_cdda = {
    "cdda_cdda",
    0,
    "Play Audio CD",
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

static int _add_volumes_cb(void *data, int type, void *event)
{
    Enna_Volume *v =  event;

    if (!strcmp(v->type, "cdda://"))
    {
	mod->cdda->device = eina_stringshare_add(v->device);
        enna_vfs_append("cdda", ENNA_CAPS_MUSIC, &class_cdda);
	ecore_event_add(ENNA_EVENT_REFRESH_BROWSER, NULL, NULL, NULL);
    }
    return 1;
}

static int _remove_volumes_cb(void *data, int type, void *event)
{
    Enna_Volume *v = event;

    if (!strcmp(v->type, "cdda://"))
    {
        enna_vfs_class_remove("cdda", ENNA_CAPS_MUSIC);
	ecore_event_add(ENNA_EVENT_REFRESH_BROWSER, NULL, NULL, NULL);
    }
    return 1;
}



/* Module interface */

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_BROWSER,
    "browser_cdda"
};

void module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Cdda));
    mod->em = em;
    em->mod = mod;

    mod->cdda = calloc(1, sizeof(Class_Private_Data));

    mod->cdda->volume_add_handler =
        ecore_event_handler_add(ENNA_EVENT_VOLUME_ADDED,
            _add_volumes_cb, mod->cdda);
    mod->cdda->volume_remove_handler =
        ecore_event_handler_add(ENNA_EVENT_VOLUME_REMOVED,
            _remove_volumes_cb, mod->cdda);
}

void module_shutdown(Enna_Module *em)
{
    Enna_Module_Cdda *mod;

    mod = em->mod;
    free(mod->cdda);
}
