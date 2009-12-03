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
#include "utils.h"
#include "logs.h"

#define SMART_NAME "mediaplayer_obj"

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
    Evas_Object *play_btn;
    Ecore_Timer *timer;
    Ecore_Event_Handler *play_event_handler;
    Ecore_Event_Handler *stop_event_handler;
    Ecore_Event_Handler *next_event_handler;
    Ecore_Event_Handler *prev_event_handler;
    Ecore_Event_Handler *pause_event_handler;
    Ecore_Event_Handler *unpause_event_handler;
    Ecore_Event_Handler *seek_event_handler;
    Ecore_Event_Handler *eos_event_handler;
    double pos;
    double len;
};

/* local subsystem globals */
static Enna_Playlist *_enna_playlist;

static int _start_cb(void *data, int type, void *event);
static int _pause_cb(void *data, int type, void *event);
static int _next_cb(void *data, int type, void *event);
static int _prev_cb(void *data, int type, void *event);
static int _unpause_cb(void *data, int type, void *event);
static int _seek_cb(void *data, int type, void *event);
static int _eos_cb(void *data, int type, void *event);

static void show_play_button(Smart_Data * sd);
static void show_pause_button(Smart_Data * sd);

static int _timer_cb(void *data);

#define METADATA_APPLY                                          \
    Enna_Metadata *metadata;                                    \
    metadata = enna_mediaplayer_metadata_get(_enna_playlist);   \
    _metadata_set(sd->layout, metadata);                        \
    enna_metadata_meta_free(metadata);                          \


static void
metadata_set_text(Evas_Object *obj,
                  Enna_Metadata *m, const char *name, int bold)
{
    char *str = NULL;
    char tmp[4096];

    if (m)
        str = enna_metadata_meta_get(m, name, 1);

    if(bold && str)
        snprintf(tmp, sizeof(tmp), "<b>%s</b>",enna_util_str_chomp(str));
    else
        snprintf(tmp, sizeof(tmp), "%s", str ? enna_util_str_chomp(str) : "");

    elm_label_label_set(obj, tmp);
    ENNA_FREE(str);
}

static void
_metadata_set(Evas_Object *obj, Enna_Metadata *metadata)
{
    Smart_Data *sd;
    char *cover;

    sd = evas_object_data_get(obj, "sd");

    if (!sd)
        return;

    metadata_set_text (sd->title, metadata, "title", 1);
    metadata_set_text (sd->album, metadata, "album", 0);
    metadata_set_text (sd->artist, metadata, "author", 0);

    ENNA_OBJECT_DEL(sd->cv);
    sd->cv = elm_image_add(sd->layout);

    cover = enna_metadata_meta_get (metadata, "cover", 1);
    if (cover)
    {
        char cv[1024] = { 0 };

        if (*cover == '/')
            snprintf(cv, sizeof (cv), "%s", cover);
        else
            snprintf(cv, sizeof (cv), "%s/.enna/covers/%s",
                     enna_util_user_home_get (), cover);

        elm_image_file_set(sd->cv, cv, NULL);
    }
    else
    {
        elm_image_file_set(sd->cv,
                           enna_config_theme_get(), "cover/music/file");
    }

    evas_object_size_hint_align_set(sd->cv, 1, 1);
    evas_object_size_hint_weight_set(sd->cv, -1, -1);
    elm_layout_content_set(sd->layout, "cover.swallow", sd->cv);
    evas_object_show(sd->cv);
}

static void
media_cover_hide (Smart_Data *sd)
{
    if (!sd)
        return;

    ENNA_OBJECT_DEL(sd->cv);
    sd->cv = elm_image_add(sd->layout);
    elm_image_file_set(sd->cv, NULL, NULL);
    elm_layout_content_set(sd->layout, "cover.swallow", sd->cv);
    evas_object_show(sd->cv);
}

/* Event from mediaplayer*/
static int
_start_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    show_play_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event PLAY ");
    METADATA_APPLY;
    edje_object_signal_emit(elm_layout_edje_get(sd->layout), "controls,show", "enna");
    ENNA_TIMER_DEL(sd->timer);
    sd->timer = ecore_timer_add(1, _timer_cb, sd);
    sd->len = enna_mediaplayer_length_get();
    sd->pos = 0.0;
    return 1;
}

static int
_stop_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event STOP ");
    edje_object_signal_emit(elm_layout_edje_get(sd->layout), "controls,hide", "enna");
    sd->pos = 0.0;
    sd->len = 0.0;
    media_cover_hide(sd);

    return 1;
}

static int
_prev_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    METADATA_APPLY;
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event PREV");
    return 1;
}

static int
_next_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    METADATA_APPLY;
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event NEXT");
    return 1;
}

static int
_unpause_cb(void *data, int type, void *event)
{
    show_play_button(data);
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event UN_PAUSE");
    return 1;
}

static int
_pause_cb(void *data, int type, void *event)
{
    show_pause_button(data);
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

static int
_eos_cb(void *data, int type, void *event)
{
    /* EOS received, update metadata */
    enna_mediaplayer_next(_enna_playlist);
    return 1;
}

/* Update position Timer callback */
static int
_timer_cb(void *data)
{
    Smart_Data *sd = data;

    sd->pos += 1.0;

    if(enna_mediaplayer_state_get() == PLAYING)
    {
        long ph, pm, ps, lh, lm, ls;
        char buf[256];
        char buf2[256];

        /* FIXME : create a dedicated function to do that */
        lh = sd->len / 3600000;
        lm = sd->len / 60 - (lh * 60);
        ls = sd->len - (lm * 60);
        ph = sd->pos / 3600;
        pm = sd->pos / 60 - (ph * 60);
        ps = sd->pos - (pm * 60);
        snprintf(buf, sizeof(buf), "%02li:%02li", pm, ps);
        snprintf(buf2, sizeof(buf2), "%02li:%02li", lm, ls);

        elm_label_label_set(sd->total_time, buf2);
        elm_label_label_set(sd->current_time, buf);
        if (sd->len)
            elm_slider_value_set(sd->sl, sd->pos/sd->len * 100.0);
        enna_log(ENNA_MSG_EVENT, NULL, "Position %f %f", sd->pos, sd->len);
    }

    return 1;
}

/* events from buttons*/
static void
_button_clicked_play_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_mediaplayer_play(_enna_playlist);
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
    Smart_Data *sd = data;

    value = elm_slider_value_get(sd->sl);
    enna_mediaplayer_seek(value / 100.0);
    sd->pos = enna_mediaplayer_position_get();
}

static void
show_play_button(Smart_Data * sd)
{
    Evas_Object *ic;
    ic = elm_icon_add(sd->layout);
    elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_play");
    elm_button_icon_set(sd->play_btn, ic);
    evas_object_show(ic);
}

static void
show_pause_button(Smart_Data * sd)
{
    Evas_Object *ic;
    ic = elm_icon_add(sd->layout);
    elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_pause");
    elm_button_icon_set(sd->play_btn, ic);
    evas_object_show(ic);
}

#define ELM_ADD(icon, cb)                                            \
    ic = elm_icon_add(layout);                                       \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(layout);                                     \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    elm_button_icon_set(bt, ic);                                     \
    elm_object_style_set(bt, "mediaplayer");                         \
    evas_object_size_hint_weight_set(bt, 1.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, -1.0, -1.0);                 \
    elm_box_pack_end(btn_box, bt);                                   \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \

/* externally accessible functions */
Evas_Object *
enna_mediaplayer_obj_add(Evas * evas, Enna_Playlist *enna_playlist)
{

    Evas_Object *layout;
    Evas_Object *bx;
    Evas_Object *btn_box;
    Evas_Object *lb;
    Evas_Object *sl_box;
    Evas_Object *sl;
    Evas_Object *ic;
    Evas_Object *bt;
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

    ELM_ADD ("icon/mp_stop",    _button_clicked_stop_cb);
    ELM_ADD ("icon/mp_prev",    _button_clicked_prev_cb);
    ELM_ADD ("icon/mp_rewind",  _button_clicked_rewind_cb);
    ELM_ADD ("icon/mp_play",    _button_clicked_play_cb);
    sd->play_btn = bt;
    ELM_ADD ("icon/mp_forward", _button_clicked_forward_cb);
    ELM_ADD ("icon/mp_next",    _button_clicked_next_cb);

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
    sd->eos_event_handler = ecore_event_handler_add(
        ENNA_EVENT_MEDIAPLAYER_EOS, _eos_cb, sd);
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
    evas_object_smart_callback_add(sl, "delay,changed", _slider_seek_cb, sd);
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
