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

/* TODO : remove smart object and use directly elm objects */

#include <Elementary.h>

#include "enna_config.h"
#include "image.h"
#include "input.h"
#include "photo_slideshow_view.h"

#undef FEATURE_ROTATION

#define ROTATION_DURATION 0.5

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    //Evas_Object *event_rect;
    Evas_Object *controls;
    Evas_Object *slideshow;
    Evas_Object *event_rect;
    Input_Listener *listener;
    Eina_List *items;
    Evas_Object *btplay;
    Evas_Object *spin;
    int delay;
    double start;
    Ecore_Animator *animator;
    char state;
    unsigned char mode;
};

/* local subsystem globals */
#ifdef FEATURE_ROTATION
static int
_rotate(void *data)
{
    Smart_Data *sd = data;
    double t = ecore_loop_time_get() - sd->start;
    Evas_Coord x, y, w, h;
    double p, deg = 0.0;
    Evas_Map *map;
    Evas_Object *photocam;
    Elm_Slideshow_Item *item;

    item = elm_slideshow_item_current_get(sd->slideshow);
    if(!item) return 1;
    photocam = elm_slideshow_item_object_get(item);

    if (!sd->animator) return 0;
    t = t / ROTATION_DURATION;
    if (t > 1.0) t = 1.0;

    evas_object_geometry_get(photocam, &x, &y, &w, &h);
    map = evas_map_new(4);
    evas_map_smooth_set(map, 0);

    if (photocam)
        evas_map_util_points_populate_from_object_full(map, photocam, 0);

    x += (w / 2);
    y += (h / 2);

    p = 1.0 - t;
    p = 1.0 - (p * p);

    if (sd->mode)
        deg = 90.0 * p + sd->state * 90;
    else
        deg = - ((3 - sd->state) * 90.0) - (90.0 * p);

    evas_map_util_3d_rotate(map, 0.0, 0.0, deg, x, y, 0);

    evas_object_map_set(photocam, map);
    evas_object_map_enable_set(photocam, 1);
    evas_map_free(map);

    if (t >= 1.0)
    {
        sd->animator = NULL;
        return 0;
    }
    return 1;
}

static void
_rotate_go(Smart_Data *sd, unsigned char mode)
{
    char inc = 1;

    if (!sd->animator) sd->animator = ecore_animator_add(_rotate, sd);
    sd->start = ecore_loop_time_get();

    if (!mode)
        inc = -1;

    sd->state += inc;
    if (sd->state < 0)
        sd->state = 3;
    else if (sd->state > 3)
        sd->state = 0;
    sd->mode = mode;
}
#endif /* FEATURE_ROTATION */

static void
_controls_show(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
    Evas_Object *ic;

    ic = elm_icon_add(obj);

    if (!elm_slideshow_timeout_get(sd->slideshow))
    {
        elm_slideshow_timeout_set(sd->slideshow, sd->delay);
        elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_pause");
    }
    else
    {
        elm_slideshow_timeout_set(sd->slideshow, 0);
        elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_play");
    }
    elm_button_icon_set(sd->btplay, ic);
    evas_object_size_hint_min_set(sd->btplay, 64, 64);
    evas_object_size_hint_weight_set(sd->btplay, 0.0, 1.0);
    evas_object_size_hint_align_set(sd->btplay, 0.5, 0.5);
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

#ifdef FEATURE_ROTATION
static void
 _button_clicked_rotate_ccw_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    _rotate_go(sd, 0);
}

static void
 _button_clicked_rotate_cw_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    _rotate_go(sd, 1);
}
#endif /* FEATURE_ROTATION */

static void
_spin(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    sd->delay = (int)elm_spinner_value_get(sd->spin);
    if (elm_slideshow_timeout_get(sd->slideshow) > 0)
        elm_slideshow_timeout_set(sd->slideshow, sd->delay);
}

static void
_mouse_wheel_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    Evas_Object *photocam;
    Elm_Slideshow_Item *item;
    Evas_Event_Mouse_Wheel *ev = (Evas_Event_Mouse_Wheel*) event_info;
    double zoom;

    //unset the mouse wheel
    ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;

    item = elm_slideshow_item_current_get(sd->slideshow);
    if(!item) return ;
    photocam = elm_slideshow_item_object_get(item);

    zoom = elm_photocam_zoom_get(photocam);

    if (ev->z > 0)
        zoom *= 1.1;
    else
        zoom /= 1.1;


    if (zoom < 10 && zoom > 0.1)
    {
        elm_photocam_zoom_mode_set(photocam, ELM_PHOTOCAM_ZOOM_MODE_MANUAL);
        elm_photocam_zoom_set(photocam, zoom);
    }

}

static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    Smart_Data *sd = evas_object_data_get(data, "sd");
    switch(event)
    {
    case ENNA_INPUT_LEFT:
        _button_clicked_prev_cb(sd, data, NULL);
        break;
    case ENNA_INPUT_RIGHT:
        _button_clicked_next_cb(sd, data, NULL);
        break;
    case ENNA_INPUT_OK:
        _button_clicked_play_cb(sd, data, NULL);
        break;
#ifdef FEATURE_ROTATION
    case ENNA_INPUT_ROTATE:
        _button_clicked_rotate_ccw_cb(sd, data, NULL);
        break;
#endif /* FEATURE_ROTATION */
    case ENNA_INPUT_DOWN:
        sd->delay--;
        if (sd->delay < 1)
            sd->delay = 1;
        if (elm_slideshow_timeout_get(sd->slideshow))
            elm_slideshow_timeout_set(sd->slideshow, sd->delay);
        elm_spinner_value_set(sd->spin, sd->delay);
        break;
    case ENNA_INPUT_UP:
        sd->delay++;
        if (sd->delay > 100)
            sd->delay = 100;
        if (elm_slideshow_timeout_get(sd->slideshow))
            elm_slideshow_timeout_set(sd->slideshow, sd->delay);
        elm_spinner_value_set(sd->spin, sd->delay);

        break;
    default:
        break;
    }
    return ENNA_EVENT_CONTINUE;
}

static Evas_Object *
_slideshow_item_get(void *data, Evas_Object *obj)
{
    Evas_Object *im;
    im = elm_photocam_add(obj);
    elm_photocam_zoom_mode_set(im,ELM_PHOTOCAM_ZOOM_MODE_AUTO_FIT);
    elm_photocam_file_set(im, data);
    return im;
}

static void
_sd_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    ENNA_OBJECT_DEL(sd->controls);
    ENNA_OBJECT_DEL(sd->slideshow);
    eina_list_free(sd->items);
    enna_input_listener_del(sd->listener);
    ENNA_FREE(sd);
}

#define ELM_ADD(icon, cb)                                            \
    ic = elm_icon_add(obj);                                          \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(obj);                                        \
    elm_object_style_set(bt, "simple");                              \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    elm_button_icon_set(bt, ic);                                     \
    evas_object_size_hint_min_set(bt, 64, 64);                       \
    evas_object_size_hint_weight_set(bt, 0.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, 0.5, 0.5);                   \
    elm_box_pack_end(bx, bt);                                        \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \

static Elm_Slideshow_Item_Class itc =
{
    {
        _slideshow_item_get,
        NULL
    }
};

/* externally accessible functions */
Evas_Object *
enna_photo_slideshow_add(Evas * evas)
{
    Evas_Object *obj;
    Smart_Data *sd;
    Evas_Object *bx, *bt, *ic;
    Evas_Coord w, h;

    sd = calloc(1, sizeof(Smart_Data));

    sd->delay = enna_config->slideshow_delay;

    obj = elm_layout_add(enna->layout);
    elm_layout_file_set(obj, enna_config_theme_get(), "enna/slideshow");
    evas_object_size_hint_weight_set(obj, 1.0, 1.0);
    evas_object_show(obj);

    sd->slideshow = elm_slideshow_add(enna->layout);
    elm_slideshow_transition_set(sd->slideshow, "horizontal");
    elm_slideshow_loop_set(sd->slideshow, 1);

    sd->controls = elm_notify_add(enna->win);
    elm_notify_orient_set(sd->controls, ELM_NOTIFY_ORIENT_BOTTOM);
    evas_object_geometry_get(enna->layout, NULL, NULL, &w, &h);
    evas_object_move(sd->controls, 0, 0);
    evas_object_resize(sd->controls, w, h);
    //elm_object_style_set(sd->controls, "enna_bottom");
    /* Fixme : add a config value */
    elm_notify_timeout_set(sd->controls, 10);

    bx = elm_box_add(obj);
    elm_box_horizontal_set(bx, 1);
    elm_notify_content_set(sd->controls, bx);
    evas_object_show(bx);

    evas_object_event_callback_add(bx, EVAS_CALLBACK_MOUSE_IN, _mouse_in, sd);
    evas_object_event_callback_add(bx, EVAS_CALLBACK_MOUSE_OUT, _mouse_out, sd);

    ELM_ADD ("icon/mp_prev",    _button_clicked_prev_cb);
    ELM_ADD ("icon/mp_pause",    _button_clicked_play_cb);
    sd->btplay = bt;
    ELM_ADD ("icon/mp_next",    _button_clicked_next_cb);
    ELM_ADD ("icon/mp_stop",    _button_clicked_stop_cb);

    sd->spin = elm_spinner_add(obj);
    elm_spinner_label_format_set(sd->spin, "%2.f secs.");
    evas_object_smart_callback_add(sd->spin, "changed", _spin, sd);
    elm_spinner_step_set(sd->spin, 1);
    elm_spinner_min_max_set(sd->spin, 1, 100);
    elm_spinner_value_set(sd->spin, sd->delay);
    elm_box_pack_end(bx, sd->spin);
    evas_object_show(sd->spin);

#ifdef FEATURE_ROTATION
    ELM_ADD ("icon/rotate_ccw", _button_clicked_rotate_ccw_cb);
    ELM_ADD ("icon/rotate_cw",  _button_clicked_rotate_cw_cb);
#endif /* FEATURE_ROTATION */

    evas_object_show(obj);
    evas_object_show(sd->slideshow);
    elm_layout_content_set(obj, "enna.content.swallow",
                           sd->slideshow);

    evas_object_data_set(obj, "sd", sd);
    sd->state = 4;
    /* Catch mouse wheel event */
    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_WHEEL,
                                   _mouse_wheel_cb, sd);
    /* connect to the input signal */
    sd->listener = enna_input_listener_add("slideshow", _input_events_cb, obj);
    enna_input_listener_demote(sd->listener);

    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_UP, _controls_show, sd);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_MOUSE_MOVE, _controls_show, sd);

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
    Elm_Slideshow_Item *it;
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    it = elm_slideshow_item_add(sd->slideshow, &itc, file);
    sd->items = eina_list_append(sd->items, it);
}

void enna_photo_slideshow_goto(Evas_Object *obj, int nth)
{
    Elm_Slideshow_Item *it;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    it = eina_list_nth(sd->items, nth);
    elm_slideshow_show(it);
}
