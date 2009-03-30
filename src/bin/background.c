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

#include "enna.h"
#include "enna_config.h"
#include "background.h"

#define SMART_NAME "enna_background"

typedef struct _smart_Data Smart_Data;

struct _smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *obj;
    Evas_Object *o_bg;
    Evas_Object *o_edje;
};

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* local subsystem globals */
static void _enna_background_smart_reconfigure(Smart_Data * sd)
{
    evas_object_move(sd->o_bg, sd->x, sd->y);
    evas_object_resize(sd->o_bg, sd->w, sd->h);
    evas_object_move(sd->o_edje, sd->x, sd->y);
    evas_object_resize(sd->o_edje, sd->w, sd->h);
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;
    Evas *e;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;

    e = evas_object_evas_get(obj);

    sd->x = 0;
    sd->y = 0;
    sd->w = 0;
    sd->h = 0;

    sd->o_bg = evas_object_rectangle_add(e);
    evas_object_color_set(sd->o_bg, 0, 0, 0, 255);
    evas_object_layer_set(sd->o_bg, 0);
    evas_object_show(sd->o_bg);

    sd->o_edje = edje_object_add(e);
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "enna/background");

    evas_object_smart_member_add(sd->o_bg, obj);
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_del(sd->o_bg);
    evas_object_del(sd->o_edje);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _enna_background_smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _enna_background_smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_bg);
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_bg);
    evas_object_hide(sd->o_edje);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_bg, clip);
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_bg);
    evas_object_clip_unset(sd->o_edje);
}

static void _enna_background_smart_init(void)
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
    if (!_e_smart)
       _e_smart = evas_smart_class_new(&sc);
}

/* externally accessible functions */
Evas_Object *
enna_background_add(Evas * evas)
{
    _enna_background_smart_init();
    return evas_object_smart_add(evas, _e_smart);
}
