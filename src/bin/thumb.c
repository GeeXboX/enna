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

#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Ipc.h>

#include "image.h"

typedef struct _Enna_Thumb Enna_Thumb;

struct _Enna_Thumb
{
    int   objid;
    int   w, h;
    const char *file;
    const char *key;
    unsigned char queued : 1;
    unsigned char busy : 1;
    unsigned char done : 1;
};

/* local subsystem functions */
static void _thumb_gen_begin(int objid, const char *file, const char *key, int w, int h);
static void _thumb_gen_end(int objid);
static void _thumb_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _thumb_hash_add(int objid, Evas_Object *obj);
static void _thumb_hash_del(int objid);
static Evas_Object *_thumb_hash_find(int objid);
static void _thumb_thumbnailers_kill(void);
static void _thumb_thumbnailers_kill_cancel(void);
static int _thumb_cb_kill(void *data);
static int _thumb_cb_exe_event_del(void *data, int type, void *event);

/* local subsystem globals */
static Eina_List *_thumbnailers = NULL;
static Eina_List *_thumbnailers_exe = NULL;
static Eina_List *_thumb_queue = NULL;
static int _objid = 0;
static Eina_Hash *_thumbs = NULL;
static int _pending = 0;
static int _num_thumbnailers = 1;
static Ecore_Event_Handler *_exe_del_handler = NULL;
static Ecore_Timer *_kill_timer = NULL;

/* externally accessible functions */
int
enna_thumb_init(void)
{
    _exe_del_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
        _thumb_cb_exe_event_del,
        NULL);
    _thumbs = eina_hash_string_superfast_new(NULL);
    return 1;
}

int
enna_thumb_shutdown(void)
{
    _thumb_thumbnailers_kill_cancel();
    _thumb_cb_kill(NULL);
    ecore_event_handler_del(_exe_del_handler);
    _exe_del_handler = NULL;
    _thumbnailers = eina_list_free(_thumbnailers);
    while (_thumbnailers_exe)
    {
        ecore_exe_free(_thumbnailers_exe->data);
        _thumbnailers_exe = eina_list_remove_list(_thumbnailers_exe, _thumbnailers_exe);
    }
    _thumb_queue = eina_list_free(_thumb_queue);
    _objid = 0;
    eina_hash_free(_thumbs);
    _thumbs = NULL;
    _pending = 0;
    return 1;
}

static void
_thumb_preloaded(void *data, Evas_Object *obj, void *event)
{
    evas_object_smart_callback_call(data, "enna_thumb_gen", NULL);
}

Evas_Object *
enna_thumb_icon_add(Evas *evas)
{
    Evas_Object *obj;
    Enna_Thumb *eth;

    obj = enna_image_add(evas);
    enna_image_fill_inside_set(obj, 0);
    evas_object_smart_callback_add(obj, "preloaded", _thumb_preloaded, obj);
    _objid++;
    eth = ENNA_NEW(Enna_Thumb, 1);
    eth->objid = _objid;
    eth->w = 64;
    eth->h = 64;
    evas_object_data_set(obj, "enna_thumbdata", eth);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
        _thumb_del_hook, NULL);
    _thumb_hash_add(eth->objid, obj);
    return obj;
}

void
enna_thumb_icon_file_set(Evas_Object *obj, const char *file, const char *key)
{
    Enna_Thumb *eth;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    if (eth->file) eina_stringshare_del(eth->file);
    eth->file = NULL;
    if (eth->key) eina_stringshare_del(eth->key);
    eth->key = NULL;
    if (file) eth->file = eina_stringshare_add(file);
    if (key) eth->key = eina_stringshare_add(key);
}

void
enna_thumb_icon_size_set(Evas_Object *obj, int w, int h)
{
    Enna_Thumb *eth;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    if ((w < 1) || (h <1)) return;
    eth->w = w;
    eth->h = h;
}

void
enna_thumb_icon_size_get(Evas_Object *obj, int *w, int *h)
{
    Enna_Thumb *eth;
    Evas_Object *im;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    im = _thumb_hash_find(eth->objid);
    if (!im)
    {
        if (w) *w = 0;
        if (h) *h = 0;
    }
    else
        enna_image_size_get(im, w, h);
}

const char*
enna_thumb_icon_file_get(Evas_Object *obj)
{
    Enna_Thumb *eth;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (eth) return eth->file;
    else return NULL;
}

void
enna_thumb_icon_orient_set(Evas_Object *obj, Enna_Image_Orient orient)
{
    Enna_Thumb *eth;
    Evas_Object *im;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    im = _thumb_hash_find(eth->objid);
    if (!im) return;
    enna_image_orient_set(im, orient);

}

void
enna_thumb_icon_begin(Evas_Object *obj)
{
    Enna_Thumb *eth, *eth2;
    char buf[4096];

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    if (eth->queued) return;
    if (eth->busy) return;
    if (eth->done) return;
    if (!eth->file) return;
    if (!_thumbnailers)
    {
        while (eina_list_count(_thumbnailers_exe) < _num_thumbnailers)
        {
            Ecore_Exe *exe;

            snprintf(buf, sizeof(buf), PACKAGE_BIN_DIR"/enna_thumb --nice=%d", 19);
            exe = ecore_exe_run(buf, NULL);
            _thumbnailers_exe = eina_list_append(_thumbnailers_exe, exe);
        }
        _thumb_queue = eina_list_append(_thumb_queue, eth);
        eth->queued = 1;
        return;
    }
    while (_thumb_queue)
    {
        eth2 = _thumb_queue->data;
        _thumb_queue = eina_list_remove_list(_thumb_queue, _thumb_queue);
        eth2->queued = 0;
        eth2->busy = 1;
        _pending++;
        if (_pending == 1) _thumb_thumbnailers_kill_cancel();
        _thumb_gen_begin(eth2->objid, eth2->file, eth2->key, eth2->w, eth2->h);
    }
    eth->busy = 1;
    _pending++;
    if (_pending == 1) _thumb_thumbnailers_kill_cancel();
    _thumb_gen_begin(eth->objid, eth->file, eth->key, eth->w, eth->h);
}

void
enna_thumb_icon_end(Evas_Object *obj)
{
    Enna_Thumb *eth;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    if (eth->queued)
    {
        _thumb_queue = eina_list_remove(_thumb_queue, eth);
        eth->queued = 0;
    }
    if (eth->busy)
    {
        _thumb_gen_end(eth->objid);
        eth->busy = 0;
        _pending--;
        if (_pending == 0) _thumb_thumbnailers_kill();
    }
}

void
enna_thumb_icon_rethumb(Evas_Object *obj)
{
    Enna_Thumb *eth;
    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;

    if (eth->done) eth->done = 0;
    else enna_thumb_icon_end(obj);

    enna_thumb_icon_begin(obj);
}


void
enna_thumb_client_data(Ecore_Ipc_Event_Client_Data *e)
{
    int objid;
    char *icon;
    Enna_Thumb *eth;
    Evas_Object *obj;

    if (!eina_list_data_find(_thumbnailers, e->client))
        _thumbnailers = eina_list_prepend(_thumbnailers, e->client);
    if (e->minor == 2)
    {
        objid = e->ref;
        icon = e->data;
        if ((icon) && (e->size > 1) && (icon[e->size - 1] == 0))
        {
            obj = _thumb_hash_find(objid);
            if (obj)
            {
                eth = evas_object_data_get(obj, "enna_thumbdata");
                if (eth)
                {
                    eth->busy = 0;
                    _pending--;
                    eth->done = 1;
                    if (_pending == 0) _thumb_thumbnailers_kill();
                    enna_image_preload(obj, 0);
                    enna_image_file_set(obj, icon, "/thumbnail/data");
                    evas_object_smart_callback_call(obj, "enna_thumb_gen", NULL);
                }
            }
        }
    }
    if (e->minor == 1)
    {
        /* hello message */
        while (_thumb_queue)
        {
            eth = _thumb_queue->data;
            _thumb_queue = eina_list_remove_list(_thumb_queue, _thumb_queue);
            eth->queued = 0;
            eth->busy = 1;
            _pending++;
            if (_pending == 1) _thumb_thumbnailers_kill_cancel();
            _thumb_gen_begin(eth->objid, eth->file, eth->key, eth->w, eth->h);
        }
    }
}

void
enna_thumb_client_del(Ecore_Ipc_Event_Client_Del *e)
{
    if (!eina_list_data_find(_thumbnailers, e->client)) return;
    _thumbnailers = eina_list_remove(_thumbnailers, e->client);
    if ((!_thumbs) && (!_thumbnailers)) _objid = 0;
}

/* local subsystem functions */
static void
_thumb_gen_begin(int objid, const char *file, const char *key, int w, int h)
{
    char *buf;
    int l1, l2;
    Ecore_Ipc_Client *cli;

    /* send thumb req */
    l1 = strlen(file);
    l2 = 0;
    if (key) l2 = strlen(key);
    buf = alloca(l1 + 1 + l2 + 1);
    strcpy(buf, file);
    if (key) strcpy(buf + l1 + 1, key);
    else buf[l1 + 1] = 0;
    cli = _thumbnailers->data;
    if (!cli) return;
    _thumbnailers = eina_list_remove_list(_thumbnailers, _thumbnailers);
    _thumbnailers = eina_list_append(_thumbnailers, cli);
    ecore_ipc_client_send(cli, 5, 1, objid, w, h, buf, l1 + 1 + l2 + 1);
}

static void
_thumb_gen_end(int objid)
{
    Eina_List *l;
    Ecore_Ipc_Client *cli;

    /* send thumb cancel */
    for (l = _thumbnailers; l; l = l->next)
    {
        cli = l->data;
        ecore_ipc_client_send(cli, 5, 2, objid, 0, 0, NULL, 0);
    }
}

static void
_thumb_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Enna_Thumb *eth;

    eth = evas_object_data_get(obj, "enna_thumbdata");
    if (!eth) return;
    evas_object_data_del(obj, "enna_thumbdata");
    _thumb_hash_del(eth->objid);
    if (eth->busy)
    {
        _thumb_gen_end(eth->objid);
        eth->busy = 0;
        _pending--;
        if (_pending == 0) _thumb_thumbnailers_kill();
    }
    if (eth->queued)
        _thumb_queue = eina_list_remove(_thumb_queue, eth);
    if (eth->file) eina_stringshare_del(eth->file);
    if (eth->key) eina_stringshare_del(eth->key);
    free(eth);
}

static void
_thumb_hash_add(int objid, Evas_Object *obj)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%i", objid);
    eina_hash_add(_thumbs, buf, obj);
}

static void
_thumb_hash_del(int objid)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%i", objid);
    eina_hash_del(_thumbs, buf, NULL);
    if ((!_thumbs) && (!_thumbnailers)) _objid = 0;
}

static Evas_Object *
_thumb_hash_find(int objid)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%i", objid);
    return eina_hash_find(_thumbs, buf);
}

static void
_thumb_thumbnailers_kill(void)
{
    if (_kill_timer) ecore_timer_del(_kill_timer);
    _kill_timer = ecore_timer_add(1.0, _thumb_cb_kill, NULL);
}

static void
_thumb_thumbnailers_kill_cancel(void)
{
    if (_kill_timer) ecore_timer_del(_kill_timer);
    _kill_timer = NULL;
}

static int
_thumb_cb_kill(void *data)
{
    Eina_List *l;

    for (l = _thumbnailers_exe; l; l = l->next)
        ecore_exe_terminate(l->data);
    _kill_timer = NULL;
    return 0;
}

static int
_thumb_cb_exe_event_del(void *data, int type, void *event)
{
    Ecore_Exe_Event_Del *ev;
    Eina_List *l;

    ev = event;
    for (l = _thumbnailers_exe; l; l = l->next)
    {
        if (l->data == ev->exe)
        {
            _thumbnailers_exe = eina_list_remove_list(_thumbnailers_exe, l);
            break;
        }
    }
    return 1;
}
