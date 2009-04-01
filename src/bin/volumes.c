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

#include <stdlib.h>
#include <string.h>

#include <Ecore.h>

#include "enna.h"
#include "volumes.h"
#include "logs.h"

static Eina_Hash *_volumes = NULL;

int ENNA_EVENT_VOLUME_ADDED;
int ENNA_EVENT_VOLUME_REMOVED;
int ENNA_EVENT_REFRESH_BROWSER;

void _hash_data_free_cb(void *data)
{
    Eina_List *l;
    Enna_Volume *v;

    EINA_LIST_FOREACH(data, l, v)
    {
        l = eina_list_remove(l, v);
        eina_stringshare_del(v->icon);
        eina_stringshare_del(v->type);
        eina_stringshare_del(v->uri);
        eina_stringshare_del(v->name);
        eina_stringshare_del(v->label);

    }
    eina_list_free(l);

}

void enna_volumes_init(void)
{

    if (_volumes)
        eina_hash_free(_volumes);
    _volumes = NULL;
    ENNA_EVENT_VOLUME_ADDED = ecore_event_type_new();
    ENNA_EVENT_VOLUME_REMOVED = ecore_event_type_new();
    ENNA_EVENT_REFRESH_BROWSER = ecore_event_type_new();
    _volumes = eina_hash_pointer_new(_hash_data_free_cb);
}

void enna_volumes_shutdown(void)
{
    if (_volumes)
        eina_hash_free(_volumes);
    _volumes = NULL;
}

void enna_volumes_append(const char *type, Enna_Volume *v)
{
    Eina_List *l = NULL;
    Enna_Volume *ev;

    if (!v) return;

    l = eina_hash_find(_volumes, type);
    l = eina_list_append(l, v);

    if (eina_hash_add(_volumes, type, l))
    {
        ev = calloc(1, sizeof(Enna_Volume));
        memcpy(ev, v, sizeof(Enna_Volume));
        enna_log(ENNA_MSG_EVENT, NULL, "ENNA_EVENT_VOLUME_ADDED Sent");
        ecore_event_add(ENNA_EVENT_VOLUME_ADDED, ev, NULL, NULL);
    }

}

void enna_volumes_remove(const char *type, Enna_Volume *v)
{
    Enna_Volume *ev;
    Eina_List *l = eina_hash_find(_volumes, type);

    l = eina_list_remove(l, v);

    ev = calloc(1, sizeof(Enna_Volume));
    memcpy(ev, v, sizeof(Enna_Volume));
    enna_log(ENNA_MSG_EVENT, NULL, "ENNA_EVENT_VOLUME_REMOVED Sent");
    ecore_event_add(ENNA_EVENT_VOLUME_REMOVED, ev, NULL, NULL);

}

Eina_List *enna_volumes_get(const char *type)
{
    return eina_hash_find(_volumes, type);
}

