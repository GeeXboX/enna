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

typedef struct _Mediaplayer_Events Mediaplayer_Events;

static struct _Mediaplayer_Events
{
    Ecore_Event_Handler *play_event_handler;
    Ecore_Event_Handler *stop_event_handler;
    Ecore_Event_Handler *next_event_handler;
    Ecore_Event_Handler *prev_event_handler;
    Ecore_Event_Handler *pause_event_handler;
    Ecore_Event_Handler *unpause_event_handler;
    Ecore_Event_Handler *seek_event_handler;
    Ecore_Event_Handler *eos_event_handler;
} _mediaplayer_events;

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *layout;
    Evas_Object *cv;
    Evas_Object *ctl;
    Evas_Object *sl;
    Evas_Object *artist;
    Evas_Object *album;
    Evas_Object *title;
    Evas_Object *play_btn;
    Evas_Object *btn_box;
    Evas_Object *text_box;
    Eina_List *buttons;
    Ecore_Timer *timer;
    double pos;
    double len;
    unsigned char show : 1;
    Enna_Playlist *playlist;
};

typedef struct _Button_Item
{
    Evas_Object *bt;
    unsigned char selected : 1;
    void (*func)(void *data, Evas_Object *obj, void *event_info);
    void *data;
}Button_Item;

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

#define METADATA_APPLY                                              \
    do                                                              \
    {                                                               \
        Enna_Metadata *metadata;                                    \
        metadata = enna_mediaplayer_metadata_get(sd->playlist);     \
        _metadata_set(sd->layout, metadata);                        \
        enna_metadata_meta_free(metadata);                          \
    }                                                               \
    while (0)


static int
metadata_set_text(Evas_Object *obj,
                  Enna_Metadata *m, const char *name, int bold)
{
    int res = 0;
    char *str = NULL;
    char tmp[4096];

    if (m)
        str = enna_metadata_meta_get(m, name, 1);

    if (!str)
        res = -1;

    if(bold && str)
        snprintf(tmp, sizeof(tmp), "<b>%s</b>",enna_util_str_chomp(str));
    else
        snprintf(tmp, sizeof(tmp), "%s", str ? enna_util_str_chomp(str) : "");

    elm_label_label_set(obj, tmp);
    ENNA_FREE(str);
    return res;
}

static void
_metadata_set(Evas_Object *obj, Enna_Metadata *metadata)
{
    Smart_Data *sd;
    char *cover;
    int res;

    sd = evas_object_data_get(obj, "sd");

    if (!sd)
        return;

    metadata_set_text (sd->title, metadata, "title", 1);
    metadata_set_text (sd->album, metadata, "album", 0);
    res = metadata_set_text (sd->artist, metadata, "author", 0);
    if (res)
        metadata_set_text(sd->artist, metadata, "artist", 0);

    ENNA_OBJECT_DEL(sd->cv);
    sd->cv = enna_image_add(enna->evas);

    cover = enna_metadata_meta_get (metadata, "cover", 1);
    if (cover)
    {
        char cv[1024] = { 0 };

        if (*cover == '/')
            snprintf(cv, sizeof (cv), "%s", cover);
        else
            snprintf(cv, sizeof (cv), "%s/.enna/covers/%s",
                     enna_util_user_home_get (), cover);

        enna_image_file_set(sd->cv, cv, NULL);
    }
    else
    {
        enna_image_file_set(sd->cv,
                           enna_config_theme_get(), "cover/music/file");
    }
    enna_image_fill_inside_set(sd->cv, 0);
    evas_object_size_hint_align_set(sd->cv, 0.5, 0.5);
    evas_object_size_hint_weight_set(sd->cv, 0, 0);
    elm_layout_content_set(sd->layout, "cover.swallow", sd->cv);
    evas_object_show(sd->cv);
}

static void
media_cover_hide (Smart_Data *sd)
{
    if (!sd)
        return;

    ENNA_OBJECT_DEL(sd->cv);
    sd->cv = enna_image_add(enna->evas);
    enna_image_file_set(sd->cv, NULL, NULL);
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
    sd->show = 1;
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
    sd->show = 0;
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
    enna_log(ENNA_MSG_EVENT, NULL, "Media control Event SEEK %d%c",
             ev->seek_value, ev->type == SEEK_ABS_PERCENT ? '%' : 's');
    return 1;
}

static int
_eos_cb(void *data, int type, void *event)
{
    Smart_Data *sd = data;

    /* EOS received, update metadata */
    enna_mediaplayer_next(sd->playlist);
    return 1;
}

#define SNPRINTF_TIME(buf, h, m, s) \
    snprintf(buf, sizeof(buf), "%.0li%c%02li:%02li", h, h ? ':' : ' ', m, s)

static void
slider_position_update(Smart_Data *sd)
{
    long ph, pm, ps, lh, lm, ls;
    char buf[256];
    char buf2[256];

    /* FIXME : create a dedicated function to do that */
    lh = sd->len / 3600;
    lm = sd->len / 60 - (lh * 60);
    ls = sd->len - (lm * 60) - lh * 3600;
    ph = sd->pos / 3600;
    pm = sd->pos / 60 - (ph * 60);
    ps = sd->pos - (pm * 60) - ph * 3600;
    SNPRINTF_TIME(buf,  ph, pm, ps);
    SNPRINTF_TIME(buf2, lh, lm, ls);

    edje_object_part_text_set(elm_layout_edje_get(sd->layout), "text.length", buf2);
    edje_object_part_text_set(elm_layout_edje_get(sd->layout), "text.pos", buf);

    if (sd->len)
        elm_slider_value_set(sd->sl, sd->pos/sd->len * 100.0);
    enna_log(ENNA_MSG_EVENT, NULL, "Position %f %f", sd->pos, sd->len);
}

/* Update position Timer callback */
static int
_timer_cb(void *data)
{
    Smart_Data *sd = data;

    if(enna_mediaplayer_state_get() == PLAYING)
    {
        sd->pos += 1.0;
        slider_position_update(sd);
    }

    return 1;
}

/* events from buttons*/
static void
_button_clicked_play_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    enna_mediaplayer_play(sd->playlist);
}

static void
_button_clicked_prev_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    enna_mediaplayer_prev(sd->playlist);
}

static void
_button_clicked_rewind_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    enna_mediaplayer_default_seek_backward ();
    sd->pos = enna_mediaplayer_position_get();
    slider_position_update(sd);
}

static void
_button_clicked_forward_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    enna_mediaplayer_default_seek_forward ();
    sd->pos = enna_mediaplayer_position_get();
    slider_position_update(sd);
}

static void
_button_clicked_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    enna_mediaplayer_next(sd->playlist);
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
    enna_mediaplayer_seek_percent((int) value);
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

int
_selected_button_get(Evas_Object *obj)
{
    Eina_List *l;
    Button_Item *button;
    int i = 0;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd->buttons) return -1;
    EINA_LIST_FOREACH(sd->buttons,l, button)
    {
        if (button->selected)
            return i;
        i++;
    }
    return -1;
}

static void
_unselect_button(Smart_Data *sd, int n)
{
    Button_Item *button;

    button = eina_list_nth(sd->buttons, n);
    if (!button) return;
    button->selected = 0;
    elm_object_disabled_set(button->bt, EINA_FALSE);
}

static void
_select_button(Smart_Data *sd, int n)
{
    Button_Item *button;

    button = eina_list_nth(sd->buttons, n);
    if (!button) return;
    button->selected = 1;
    elm_object_disabled_set(button->bt, EINA_TRUE);
}

static void
_activate_button(Smart_Data *sd, int n)
{
    Button_Item *button;

    button = eina_list_nth(sd->buttons, n);
    if (!button || !button->func) return;
    button->func(button->data, NULL, NULL);
}

static int
_set_button(Smart_Data *sd, int start, int right)
{
    int n, ns;

    ns = start;
    n = start;

    int boundary = right ? eina_list_count(sd->buttons) - 1 : 0;

    if (n == boundary)
        return ENNA_EVENT_CONTINUE;

    n = right ? n + 1 : n - 1;

    if (n != ns)
    {
        _unselect_button(sd, ns);
        _select_button(sd, n);
    }
    return ENNA_EVENT_BLOCK;
}

static void
_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    enna_mediaplayer_obj_event_release();

    ENNA_OBJECT_DEL(sd->cv);
    ENNA_OBJECT_DEL(sd->btn_box);
    ENNA_OBJECT_DEL(sd->text_box);
    eina_list_free(sd->buttons);
    ENNA_TIMER_DEL(sd->timer);
    free(sd);
}

void
enna_mediaplayer_obj_event_catch(Evas_Object *obj)
{
    Mediaplayer_Events *ev = &_mediaplayer_events;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd)
        return;

    enna_mediaplayer_obj_event_release();

    ev->play_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_START, _start_cb, sd);
    ev->stop_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_STOP, _stop_cb, sd);
    ev->prev_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_PREV, _prev_cb, sd);
    ev->next_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_NEXT, _next_cb, sd);
    ev->pause_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_PAUSE, _pause_cb, sd);
    ev->unpause_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_UNPAUSE, _unpause_cb, sd);
    ev->seek_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_SEEK, _seek_cb, sd);
    ev->eos_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_EOS, _eos_cb, sd);
}

void
enna_mediaplayer_obj_event_release(void)
{
    Mediaplayer_Events *ev = &_mediaplayer_events;

    ENNA_EVENT_HANDLER_DEL(ev->play_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->stop_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->prev_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->next_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->pause_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->unpause_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->seek_event_handler);
    ENNA_EVENT_HANDLER_DEL(ev->eos_event_handler);
}

#define ELM_ADD(icon, cb)                                            \
    it = ENNA_NEW(Button_Item, 1);                                   \
    ic = elm_icon_add(layout);                                       \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(layout);                                     \
    it->bt = bt;                                                     \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    it->func = cb;                                                   \
    it->data = sd;                                                   \
    elm_button_icon_set(bt, ic);                                     \
    elm_object_style_set(bt, "mediaplayer");                         \
    evas_object_size_hint_weight_set(bt, 0.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, 0.5, 0.5);                   \
    elm_box_pack_end(btn_box, bt);                                   \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \
    sd->buttons = eina_list_append(sd->buttons, it);                 \

/* externally accessible functions */
Evas_Object *
enna_mediaplayer_obj_add(Evas * evas, Enna_Playlist *enna_playlist)
{

    Evas_Object *layout;
    Evas_Object *bx;
    Evas_Object *btn_box;
    Evas_Object *lb;
    Evas_Object *sl;
    Evas_Object *ic;
    Evas_Object *bt;
    Smart_Data  *sd;
    Button_Item *it;

    sd = ENNA_NEW(Smart_Data, 1);

    layout = elm_layout_add(enna->layout);
    elm_layout_file_set(layout, enna_config_theme_get(), "core/mediaplayer");
    evas_object_size_hint_weight_set(layout, 0.0, 0.0);
    evas_object_size_hint_align_set(layout, 0.5, 0.5);
    sd->layout = layout;

    bx = elm_box_add(enna->layout);
    evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_layout_content_set(layout, "text.swallow", bx);
    evas_object_show(bx);
    sd->text_box = bx;

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

    evas_object_size_hint_weight_set(btn_box, EVAS_HINT_EXPAND, 0.5);
    evas_object_size_hint_align_set(btn_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_layout_content_set(layout, "buttons.swallow", btn_box);
    evas_object_show(btn_box);
    sd->btn_box = btn_box;

    sl = elm_slider_add(layout);
    elm_slider_span_size_set(sl, 0);
    evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_slider_min_max_set(sl, 0.0, 100.0);
    elm_slider_indicator_format_set(sl, "%1.0f %%");
    evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(sl);
    evas_object_smart_callback_add(sl, "delay,changed", _slider_seek_cb, sd);
    sd->sl = sl;
    elm_layout_content_set(layout, "slider.swallow", sl);

    evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL, _del_cb, sd);

    sd->playlist = enna_playlist;

    evas_object_data_set(layout, "sd", sd);

    return layout;
 }

Eina_Bool
enna_mediaplayer_obj_input_feed(Evas_Object *obj, enna_input event)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    int ns;

    ns = _selected_button_get(obj);

    switch (event)
    {
    case ENNA_INPUT_LEFT:
        return _set_button(sd, ns, 0);
    case ENNA_INPUT_RIGHT:
        return _set_button(sd, ns, 1);
    case ENNA_INPUT_OK:
        _activate_button(sd, ns);
        return ENNA_EVENT_BLOCK;
    default:
        return ENNA_EVENT_CONTINUE;
    }

}

unsigned char
enna_mediaplayer_show_get(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    return sd->show;
}

void
enna_mediaplayer_obj_layout_set(Evas_Object *obj, const char *layout)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    edje_object_signal_emit(elm_layout_edje_get(sd->layout), layout, "enna");
}

void
enna_mediaplayer_position_update(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    sd->pos = enna_mediaplayer_position_get();
    slider_position_update(sd);
}
