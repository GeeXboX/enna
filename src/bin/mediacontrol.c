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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "mediaplayer.h"
#include "mediacontrol.h"
#include "logs.h"

#define SMART_NAME "enna_MEDIACONTROL"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_play;
    Evas_Object *o_pause;
    Evas_Object *o_prev;
    Evas_Object *o_rewind;
    Evas_Object *o_forward;
    Evas_Object *o_next;
    Evas_Object *o_stop;
    Evas_Object *o_btn_box;
    Ecore_Event_Handler *play_event_handler;
    Ecore_Event_Handler *stop_event_handler;
    Ecore_Event_Handler *next_event_handler;
    Ecore_Event_Handler *prev_event_handler;
    Ecore_Event_Handler *pause_event_handler;
    Ecore_Event_Handler *unpause_event_handler;
    Ecore_Event_Handler *seek_event_handler;
};

/* local subsystem functions */
static void _smart_reconfigure(Smart_Data * sd);

/*event from Media player*/
static int _start_cb(void *data, int type, void *event);
static int _pause_cb(void *data, int type, void *event);
static int _next_cb(void *data, int type, void *event);
static int _prev_cb(void *data, int type, void *event);
static int _pause_cb(void *data, int type, void *event);
static int _unpause_cb(void *data, int type, void *event);
static int _seek_cb(void *data, int type, void *event);

static void show_play_button(Smart_Data * sd);
static void show_pause_button(Smart_Data * sd);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;
static Enna_Playlist *_enna_playlist;

/* Event from mediaplayer*/
static int
_start_cb(void *data, int type, void *event)
{
    show_pause_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event PLAY ");
    return 1;
}

static int
_stop_cb(void *data, int type, void *event)
{
    show_play_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event STOP ");
    return 1;
}

static int
_prev_cb(void *data, int type, void *event)
{
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event PREV");
    return 1;
}

static int
_next_cb(void *data, int type, void *event)
{
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event NEXT");
    return 1;
}

static int
_unpause_cb(void *data, int type, void *event)
{
    show_pause_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event UN_PAUSE");
    return 1;
}

static int
_pause_cb(void *data, int type, void *event)
{
    show_play_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event PAUSE ");
    return 1;
}

static int
_seek_cb(void *data, int type, void *event)
{
    Enna_Event_Mediaplayer_Seek_Data *ev;
    ev=event;
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event SEEK %d%%",(int) (100 * ev->seek_value));
    return 1;
}

/* events from buttons*/
static void
_button_clicked_play_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_play(_enna_playlist);
}

static void
_button_clicked_pause_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_pause();
}

static void
_button_clicked_prev_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_prev(_enna_playlist);
}

static void
_button_clicked_rewind_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_default_seek_backward ();
}

static void
_button_clicked_forward_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_default_seek_forward ();
}

static void
_button_clicked_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_next(_enna_playlist);
}

static void
_button_clicked_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_stop();
}

static void
show_play_button(Smart_Data * sd)
{
    edje_object_signal_emit(sd->o_edje, "play,show", "enna");
}

static void
show_pause_button(Smart_Data * sd)
{
    edje_object_signal_emit(sd->o_edje, "play,hide", "enna");
}


/* local subsystem globals */
static void
_smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, x, y);
    evas_object_resize(sd->o_edje, w, h);

}

#define ELM_ADD(icon, cb)                                            \
    ic = elm_icon_add(obj);                                          \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(obj);                                        \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    elm_button_icon_set(bt, ic);                                     \
    evas_object_size_hint_weight_set(bt, 1.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, -1.0, -1.0);                 \
    elm_box_pack_end(sd->o_btn_box, bt);                             \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \

static void
_smart_add(Evas_Object * obj)
{
    Evas_Object *ic, *bt;
    Smart_Data *sd;
    Evas *evas;
    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;
    sd->h = 0;
    evas=evas_object_evas_get(obj);

    sd->o_edje = edje_object_add(evas);
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "mediacontrol");
    evas_object_show(sd->o_edje);

    sd->o_btn_box = elm_box_add(obj);
    elm_box_homogenous_set(sd->o_btn_box, 0);
    elm_box_horizontal_set(sd->o_btn_box, 1);
    evas_object_size_hint_align_set(sd->o_btn_box, 0, 0.5);
    evas_object_size_hint_weight_set(sd->o_btn_box, 1.0, 1.0);
    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", sd->o_btn_box);

    ELM_ADD ("icon/mp_play",    _button_clicked_play_cb);
    ELM_ADD ("icon/mp_pause",   _button_clicked_pause_cb);
    ELM_ADD ("icon/mp_prev",    _button_clicked_prev_cb);
    ELM_ADD ("icon/mp_rewind",  _button_clicked_rewind_cb);
    ELM_ADD ("icon/mp_forward", _button_clicked_forward_cb);
    ELM_ADD ("icon/mp_next",    _button_clicked_next_cb);
    ELM_ADD ("icon/mp_stop",    _button_clicked_stop_cb);

    sd->play_event_handler = ecore_event_handler_add(
             ENNA_EVENT_MEDIAPLAYER_START, _start_cb, sd);
    sd->stop_event_handler = ecore_event_handler_add(
             ENNA_EVENT_MEDIAPLAYER_STOP, _stop_cb, sd);
    sd->prev_event_handler = ecore_event_handler_add(
             ENNA_EVENT_MEDIAPLAYER_PREV, _prev_cb, sd);
    sd->next_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_NEXT, _next_cb, sd);
    sd->pause_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_PAUSE, _pause_cb, sd);
    sd->unpause_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_UNPAUSE, _unpause_cb, sd);
    sd->seek_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_SEEK, _seek_cb, sd);
    if (enna_mediaplayer_state_get() == PLAYING)
        show_pause_button(sd);
    else
        show_play_button(sd);

    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void
_smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;

    ecore_event_handler_del(sd->play_event_handler);
    ecore_event_handler_del(sd->stop_event_handler);
    ecore_event_handler_del(sd->next_event_handler);
    ecore_event_handler_del(sd->prev_event_handler);
    ecore_event_handler_del(sd->pause_event_handler);
    ecore_event_handler_del(sd->unpause_event_handler);
    ecore_event_handler_del(sd->seek_event_handler);
    evas_object_del(sd->o_play);
    evas_object_del(sd->o_pause);
    evas_object_del(sd->o_prev);
    evas_object_del(sd->o_rewind);
    evas_object_del(sd->o_forward);
    evas_object_del(sd->o_next);
    evas_object_del(sd->o_stop);
    evas_object_del(sd->o_edje);
    free(sd);
}

static void
_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void
_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
}

static void
_smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_edje);
}

static void
_smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void
_smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void
_smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_edje, clip);
}

static void
_smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_edje);
}

static void
_smart_init(void)
{
    static const Evas_Smart_Class sc =
    {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _smart_add,
        _smart_del,
        _smart_move,
        _smart_resize,
        _smart_show,
        _smart_hide,
        _smart_color_set,
        _smart_clip_set,
        _smart_clip_unset,
        NULL,
        NULL
    };
    if (!_smart)
       _smart = evas_smart_class_new(&sc);
}

/* externally accessible functions */
Evas_Object *
enna_mediacontrol_add(Evas * evas,Enna_Playlist *enna_playlist)
{
    _enna_playlist=enna_playlist;
    _smart_init();
    return evas_object_smart_add(evas, _smart);
}
