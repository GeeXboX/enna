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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "location.h"

#define SMART_NAME "enna_location"

typedef struct _Smart_Data Smart_Data;
struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *smart_obj;
    Evas_Object *o_box;
    Evas_Object *o_scroll;
    Eina_List *items;
    unsigned char on_hold : 1;
};

typedef struct _Enna_Location_Item Enna_Location_Item;

struct _Enna_Location_Item
{
    Smart_Data *sd;
    Evas_Object *o_base;
    Evas_Object *o_icon;
    unsigned char selected : 1;
    void (*func)(void *data, void *data2);
    void *data;
    void *data2;
};

/* local subsystem functions */
static void _enna_location_smart_reconfigure(Smart_Data * sd);
static void _enna_location_smart_init(void);
static void _smart_add(Evas_Object * obj);
static void _smart_del(Evas_Object * obj);
static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object * obj);
static void _smart_hide(Evas_Object * obj);
static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void _smart_clip_unset(Evas_Object * obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
enna_location_add(Evas * evas)
{
    _enna_location_smart_init();
    return evas_object_smart_add(evas, _e_smart);
}

void enna_location_append(Evas_Object *obj, const char *label,
        Evas_Object *icon, void (*func) (void *data, void *data2), void *data, void *data2)
{
    Enna_Location_Item *si;
    Evas_Coord mw = 0, mh = 0;
    API_ENTRY return;

    si = ENNA_NEW(Enna_Location_Item, 1);

    si->sd = sd;
    si->o_base = edje_object_add(evas_object_evas_get(sd->o_box));
    edje_object_file_set(si->o_base, enna_config_theme_get(), "enna/location/item");
    edje_object_part_text_set(si->o_base, "enna.text.label", label);
    si->func = func;
    si->data = data;
    si->data2 = data2;
    sd->items = eina_list_append(sd->items, si);
    edje_object_size_min_calc(si->o_base, &mw, &mh);
    mh = sd->h ? sd->h : 64;
    evas_object_size_hint_min_set(si->o_base, mw, mh);
    evas_object_size_hint_align_set(si->o_base, -1.0, -1.0);
    elm_table_pack(sd->o_box, si->o_base, evas_list_count(sd->items) - 1, 0, 1 , 1);
    evas_object_lower(si->o_base);
    edje_object_signal_emit(si->o_base, "location,show", "enna");
    evas_object_show(si->o_base);
}

static void _location_hide_end(void *data, Evas_Object *o, const char *sig,
        const char *src)
{
    Smart_Data *sd;
    Enna_Location_Item *si;
    si = data;

    if (!si) return;

    sd = si->sd;
    sd->items = eina_list_remove(sd->items, si);
    edje_object_signal_callback_del(si->o_base, "location,hide,end", "edje",
	_location_hide_end);
     evas_object_del(si->o_icon);
     evas_object_del(si->o_base);
     free(si);
}

void enna_location_remove_nth(Evas_Object *obj, int n)
{
    Enna_Location_Item *si = NULL;

    API_ENTRY return;
    if (!sd->items) return;
    if (!(si = eina_list_nth(sd->items, n))) return;

    edje_object_signal_emit(si->o_base, "location,hide", "enna");
    edje_object_signal_callback_add(si->o_base, "location,hide,end", "edje",
            _location_hide_end, si);
}

const char * enna_location_label_get_nth(Evas_Object *obj, int n)
{
    Enna_Location_Item *si = NULL;

    API_ENTRY return NULL;
    if (!sd->items) return NULL;
    if (!(si = eina_list_nth(sd->items, n))) return NULL;
    return edje_object_part_text_get(si->o_base, "enna.text.label");
}

int enna_location_count(Evas_Object *obj)
{
    API_ENTRY return 0;
    return eina_list_count(sd->items);
}

/* local subsystem globals */
static void _enna_location_smart_reconfigure(Smart_Data * sd)
{
    evas_object_move(sd->o_box, sd->x, sd->y);
    evas_object_resize(sd->o_box, sd->w, sd->h);
}

static void _enna_location_smart_init(void)
{
    if (_e_smart)
        return;
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
    _e_smart = evas_smart_class_new(&sc);
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;

    sd->o_box = elm_table_add(obj);
    evas_object_box_layout_set(sd->o_box, evas_object_box_layout_horizontal, NULL, NULL);
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;
    sd->h = 0;
    sd->items = NULL;
    sd->smart_obj = obj;
    evas_object_smart_member_add(sd->o_box, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;

    evas_object_del(sd->o_box);

    while (sd->items)
    {
        Enna_Location_Item *si = sd->items->data;
        sd->items = eina_list_remove_list(sd->items, sd->items);
        evas_object_del(si->o_base);
        evas_object_del(si->o_icon);
        free(si);
    }
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _enna_location_smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _enna_location_smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_box);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_box);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_box, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_box, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_box);
}
