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

#include "enna_config.h"
#include "image.h"
#include "metadata.h"
#include "mediaplayer.h"
#include "mediaplayer_obj.h"
#include "mediacontrol.h"
#include "utils.h"
#include "logs.h"

#define SMART_NAME "smart_mediaplayer"

typedef struct _Smart_Btn_Item
{
    Evas_Object *bt;
    Evas_Object *ic;
}Smart_Btn_Item;

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *layout;
    Evas_Object *cv;
    Evas_Object *ctl;
    Evas_Object *sl;
    Evas_Object *current_time;
    Evas_Object *total_time;
    Evas_Object *artist;
    Evas_Object *album;
    Evas_Object *title;
    Eina_List *btns;
    Ecore_Event_Handler *play_event_handler;
    Ecore_Event_Handler *stop_event_handler;
    Ecore_Event_Handler *next_event_handler;
    Ecore_Event_Handler *prev_event_handler;
    Ecore_Event_Handler *pause_event_handler;
    Ecore_Event_Handler *unpause_event_handler;
    Ecore_Event_Handler *seek_event_handler;
};

/* local subsystem globals */
static Enna_Playlist *_enna_playlist;

static int _start_cb(void *data, int type, void *event);
static int _pause_cb(void *data, int type, void *event);
static int _next_cb(void *data, int type, void *event);
static int _prev_cb(void *data, int type, void *event);
static int _pause_cb(void *data, int type, void *event);
static int _unpause_cb(void *data, int type, void *event);
static int _seek_cb(void *data, int type, void *event);

static void show_play_button(Smart_Data * sd);
static void show_pause_button(Smart_Data * sd);

/* Event from mediaplayer*/
static int
_start_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    show_pause_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event PLAY ");
    edje_object_signal_emit(elm_layout_edje_get(sd->layout), "controls,show", "enna");
    return 1;
}

static int
_stop_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    show_play_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event STOP ");
    edje_object_signal_emit(elm_layout_edje_get(sd->layout), "controls,hide", "enna");
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
_slider_seek_cb(void *data, Evas_Object *obj, void *event_info)
{
    double value;

    value = elm_slider_value_get(data);
    enna_mediaplayer_seek(value / 100.0);
}

static void
show_play_button(Smart_Data * sd)
{
}

static void
show_pause_button(Smart_Data * sd)
{

}

void
enna_smart_player_position_set(Evas_Object *obj,
                               double pos, double len, double percent)
{
    Smart_Data *sd;

    long ph, pm, ps, lh, lm, ls;
    char buf[256];
    char buf2[256];

    sd = evas_object_data_get(obj, "sd");
    if (!sd) return;

    lh = len / 3600000;
    lm = len / 60 - (lh * 60);
    ls = len - (lm * 60);
    ph = pos / 3600;
    pm = pos / 60 - (ph * 60);
    ps = pos - (pm * 60);
    snprintf(buf, sizeof(buf), "%02li:%02li", pm, ps);
    snprintf(buf2, sizeof(buf2), "%02li:%02li", lm, ls);

    elm_label_label_set(sd->total_time, buf2);
    elm_label_label_set(sd->current_time, buf);

    elm_slider_value_set(sd->sl, pos/len * 100.0);
}

static void
metadata_set_text(Evas_Object *obj,
                  Enna_Metadata *m, const char *name, int bold)
{
    char *str;
    char tmp[4096];

    str = enna_metadata_meta_get(m, name, 1);

    if(bold && str)
        snprintf(tmp, sizeof(tmp), "<b>%s</b>",enna_util_str_chomp(str));
    else
        snprintf(tmp, sizeof(tmp), "%s", str ? enna_util_str_chomp(str) : "");

    elm_label_label_set(obj, tmp);
    ENNA_FREE(str);
}

void
enna_smart_player_metadata_set(Evas_Object *obj, Enna_Metadata *metadata)
{
    Smart_Data *sd;
    char *cover;

    sd = evas_object_data_get(obj, "sd");

    if (!metadata || !sd)
        return;

    metadata_set_text (sd->title, metadata, "title", 1);
    metadata_set_text (sd->album, metadata, "album", 0);
    metadata_set_text (sd->artist, metadata, "author", 0);

    cover = enna_metadata_meta_get (metadata, "cover", 1);
    if (cover)
    {
        char cv[1024] = { 0 };

        if (*cover == '/')
            snprintf(cv, sizeof (cv), "%s", cover);
        else
            snprintf(cv, sizeof (cv), "%s/.enna/covers/%s",
                     enna_util_user_home_get (), cover);

        ENNA_OBJECT_DEL(sd->cv);
        sd->cv = elm_image_add(sd->layout);
        elm_image_file_set(sd->cv, cv, NULL);
        evas_object_size_hint_align_set(sd->cv, 1, 1);
        evas_object_size_hint_weight_set(sd->cv, -1, -1);
        elm_layout_content_set(sd->layout, "cover.swallow", sd->cv);
    }
    else
    {
        elm_image_file_set(sd->cv, NULL, NULL);
    }
    evas_object_show(sd->cv);
}

void
enna_smart_player_metadata_unset(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "sd");
    edje_object_signal_emit(elm_layout_edje_get(sd->layout), "controls,hide", "enna");
}

#define ELM_ADD(icon, cb)                                            \
    ic = elm_icon_add(layout);                                       \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(layout);                                     \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    elm_button_icon_set(bt, ic);                                     \
    evas_object_size_hint_weight_set(bt, 1.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, -1.0, -1.0);                 \
    elm_box_pack_end(btn_box, bt);                                   \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \
    it = calloc(1, sizeof(Smart_Btn_Item));                          \
    it->bt = bt;                                                     \
    it->ic = ic;                                                     \
    sd->btns = eina_list_append(sd->btns, it);                       \

/* externally accessible functions */
Evas_Object *
enna_smart_player_add(Evas * evas, Enna_Playlist *enna_playlist)
{

    Evas_Object *layout;
    Evas_Object *bx;
    Evas_Object *btn_box;
    Evas_Object *lb;
    Evas_Object *sl_box;
    Evas_Object *sl;
    Evas_Object *ic;
    Evas_Object *bt;
    Smart_Btn_Item *it;
    Smart_Data *sd;

    sd = ENNA_NEW(Smart_Data, 1);

    layout = elm_layout_add(enna->layout);
    elm_layout_file_set(layout, enna_config_theme_get(), "core/mediaplayer");
    evas_object_size_hint_weight_set(enna->layout, 1.0, 1.0);
    sd->layout = layout;

    bx = elm_box_add(enna->layout);
    evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_layout_content_set(layout, "text.swallow", bx);
    evas_object_show(bx);

    lb = elm_label_add(layout);
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, 0.5, 0.5);
    elm_label_label_set(lb, "");
    elm_box_pack_end(bx, lb);
    evas_object_show(lb);
    sd->title = lb;

    lb = elm_label_add(layout);
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, 0.5, 0.5);
    elm_label_label_set(lb, "");
    elm_box_pack_end(bx, lb);
    evas_object_show(lb);
    sd->album = lb;

    lb = elm_label_add(layout);
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, 0.5, 0.5);
    elm_label_label_set(lb, "");
    elm_box_pack_end(bx, lb);
    evas_object_show(lb);
    sd->artist = lb;

    btn_box = elm_box_add(layout);
    elm_box_homogenous_set(btn_box, 0);
    elm_box_horizontal_set(btn_box, 1);
    ELM_ADD ("icon/mp_play",    _button_clicked_play_cb);
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
    evas_object_size_hint_weight_set(btn_box, EVAS_HINT_EXPAND, 0.5);
    evas_object_size_hint_align_set(btn_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_layout_content_set(layout, "buttons.swallow", btn_box);
    evas_object_show(btn_box);

    sl_box = elm_box_add(layout);
    elm_box_horizontal_set(sl_box, 1);
    //evas_object_size_hint_align_set(sl_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_align_set(sl_box, EVAS_HINT_FILL, 0.5);
    evas_object_size_hint_weight_set(sl_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(sl_box);
    elm_box_pack_end(bx, sl_box);

    lb = elm_label_add(layout);
    elm_box_pack_end(sl_box, lb);
    evas_object_show(lb);
    sd->current_time = lb;

    sl = elm_slider_add(layout);
    elm_slider_span_size_set(sl, 80);
    evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
    elm_slider_min_max_set(sl, 0.0, 100.0);
    elm_slider_indicator_format_set(sl, "%1.0f %%");
    evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_pack_end(sl_box, sl);
    evas_object_show(sl);
    evas_object_smart_callback_add(sl, "delay,changed", _slider_seek_cb, sl);
    sd->sl = sl;

    lb = elm_label_add(layout);
    elm_box_pack_end(sl_box, lb);
    evas_object_show(lb);
    sd->total_time = lb;
    elm_layout_content_set(layout, "slider.swallow", sl_box);

    /* FIXME : WTF ? why we have a ref to the playlist there ? */
    _enna_playlist = enna_playlist;

    evas_object_data_set(layout, "sd", sd);

    return layout;
 }
