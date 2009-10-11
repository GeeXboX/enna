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

/* TODO : remove smart object and use directly elm objects */

#include <Elementary.h>

#include "enna_config.h"
#include "input.h"
#include "photo_slideshow_view.h"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *controls;
    Evas_Object *slideshow;
    Input_Listener *listener;
};

/* local subsystem globals */

static void
_controls_show(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    evas_object_show(sd->controls);
    elm_notify_timer_init(sd->controls);
}

static void
_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    elm_notify_timeout_set(sd->controls, 0);
}

static void
_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    elm_notify_timeout_set(sd->controls, 3);
}


static void
_button_clicked_play_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    elm_slideshow_timeout_set (sd->slideshow,
                               !elm_slideshow_timeout_get (sd->slideshow)
                               ? enna_config->slideshow_delay : 0);
}

static void
_button_clicked_prev_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    elm_slideshow_previous(sd->slideshow);
}

static void
_button_clicked_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    elm_slideshow_next(sd->slideshow);
}

static void
_button_clicked_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    elm_slideshow_timeout_set(sd->slideshow, 0);
}

static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    return ENNA_EVENT_CONTINUE;
}

static void
_sd_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    ENNA_OBJECT_DEL(sd->controls);
    ENNA_OBJECT_DEL(sd->slideshow);
    ENNA_FREE(sd);
}

#define ELM_ADD(icon, cb)                                            \
    ic = elm_icon_add(obj);                                          \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(obj);                                        \
    elm_button_style_set(bt, "simple");                              \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    elm_button_icon_set(bt, ic);                                     \
    evas_object_size_hint_min_set(bt, 64, 64);                       \
    evas_object_size_hint_weight_set(bt, 0.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, 0.5, 0.5);                   \
    elm_box_pack_end(bx, bt);                                        \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \

/* externally accessible functions */
Evas_Object *
enna_photo_slideshow_add(Evas * evas)
{
    Evas_Object *obj;
    Smart_Data *sd;
    Evas_Object *bx, *bt, *ic;
    Evas_Coord w, h;

    sd = calloc(1, sizeof(Smart_Data));

    obj = elm_layout_add(enna->layout);
    elm_layout_file_set(obj, enna_config_theme_get(), "enna/slideshow");
    evas_object_size_hint_weight_set(obj, 1.0, 1.0);
    evas_object_show(obj);

    sd->slideshow = elm_slideshow_add(enna->layout);
    elm_slideshow_transition_set(sd->slideshow, "horizontal");
    elm_slideshow_ratio_set(sd->slideshow, 1);
    elm_slideshow_loop_set(sd->slideshow, 1);

    sd->controls = elm_notify_add(enna->win);
    elm_notify_orient_set(sd->controls, ELM_NOTIFY_ORIENT_BOTTOM);
    evas_object_geometry_get(enna->layout, NULL, NULL, &w, &h);
    evas_object_move(sd->controls, 0, 0);
    evas_object_resize(sd->controls, w, h);
    /* Fixme : add a config value */
    elm_notify_timeout_set(sd->controls, 10);

    bx = elm_box_add(obj);
    elm_box_horizontal_set(bx, 1);
    elm_notify_content_set(sd->controls, bx);
    evas_object_show(bx);

    evas_object_event_callback_add(bx, EVAS_CALLBACK_MOUSE_IN, _mouse_in, sd);
    evas_object_event_callback_add(bx, EVAS_CALLBACK_MOUSE_OUT, _mouse_out, sd);

    ELM_ADD ("icon/mp_prev",    _button_clicked_prev_cb);
    ELM_ADD ("icon/mp_play",    _button_clicked_play_cb);
    ELM_ADD ("icon/mp_next",    _button_clicked_next_cb);
    ELM_ADD ("icon/mp_stop",    _button_clicked_stop_cb);
//    ELM_ADD ("icon/rotate_ccw", _button_clicked_rotate_ccw_cb);
//    ELM_ADD ("icon/rotate_cw",  _button_clicked_rotate_cw_cb);

    evas_object_show(obj);
    evas_object_show(sd->slideshow);
    elm_layout_content_set(obj, "enna.content.swallow",
                           sd->slideshow);

    evas_object_data_set(obj, "sd", sd);

    /* connect to the input signal */
    sd->listener = enna_input_listener_add("slideshow", _input_events_cb, obj);
    enna_input_listener_demote(sd->listener);

    evas_object_smart_callback_add(sd->slideshow, "clicked", _controls_show, sd);
    evas_object_smart_callback_add(sd->slideshow, "move", _controls_show, sd);

    evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                   _sd_del, sd);
    return obj;
}

void enna_photo_slideshow_next(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    elm_slideshow_next(sd->slideshow);
}

void enna_photo_slideshow_previous(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    elm_slideshow_previous(sd->slideshow);
}

int enna_photo_slideshow_timeout_get(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    return elm_slideshow_timeout_get(sd->slideshow);
}

void enna_photo_slideshow_timeout_set(Evas_Object *obj, int to)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    elm_slideshow_timeout_set(sd->slideshow, to);
}

void enna_photo_slideshow_image_add(Evas_Object *obj, const char *file, const char *group)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    elm_slideshow_image_add(sd->slideshow, file, group);
}

void enna_photo_slideshow_goto(Evas_Object *obj, int nth)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    elm_slideshow_goto(sd->slideshow, nth);
}
