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

#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Data.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "image.h"
#include "logs.h"
#include "event_key.h"
#include "view_cover.h"

#define SMART_NAME "enna_view_cover"

typedef struct _Smart_Data Smart_Data;
typedef struct _Smart_Item Smart_Item;

struct _Smart_Item
{
    Evas_Object *o_edje;
    int row;
    Evas_Object *o_pict; // Elm image object
    Smart_Data *sd;
    const char *label;
    Enna_Vfs_File *file;
    void *data;
    void (*func) (void *data);
    unsigned char selected : 1;
};

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *obj;
    Evas_Object *o_edje;
    Evas_Object *o_scroll;
    Evas_Object *o_table;
    Evas_Object *o_box;
    Eina_List *items;
    int nb;
};

struct _Preload_Data
{
    Smart_Data *sd;
    Evas_Object *item;
};

/* local subsystem functions */
static void _view_cover_h_select(Evas_Object *obj, int pos);
static Smart_Item *_smart_selected_item_get(Smart_Data *sd, int *nth);
static void _smart_item_unselect(Smart_Data *sd, Smart_Item *si);
static void _smart_item_select(Smart_Data *sd, Smart_Item *si);
static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj,
        void *event_info);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;

static void
enna_view_cover_display_icon (Evas_Object *o, Evas_Object *p, Evas_Object *e,
                              const char *file, const char *group,
                              Evas_Coord w, Evas_Coord h,
                              char *signal)
{
    elm_image_file_set (p, file, group);
    elm_image_no_scale_set (p, 1);
    elm_image_smooth_set (p, 1);

    /* Fit container but keep aspect ratio */
    elm_image_scale_set (p, 1, 1);
    evas_object_size_hint_min_set (o, w, h);
    evas_object_size_hint_align_set (o, 1, 1);
    evas_object_show (p);
    edje_object_part_swallow (o, "enna.swallow.icon", p);
    edje_object_signal_emit (e, signal, "enna");
    evas_object_show (o);
}

void enna_view_cover_file_append(Evas_Object *obj, Enna_Vfs_File *file,
    void (*func) (void *data), void *data)
{
    Evas_Object *o, *o_pict;
    Smart_Item *si;

    API_ENTRY return;

    si = calloc(1, sizeof(Smart_Item));

    sd->nb++;
    o = edje_object_add(evas_object_evas_get(sd->o_scroll));
    edje_object_file_set(o, enna_config_theme_get(), "enna/covervideo/item");
    si->label = eina_stringshare_add(file->label);
    o_pict = elm_image_add(sd->o_scroll);
    si->data = data;
    si->func = func;
    si->file = file;
    
    if (file->icon && file->icon[0] != '/')
        enna_view_cover_display_icon (o, o_pict, si->o_edje,
                                      enna_config_theme_get (), file->icon,
                                      96, 96, "shadow,hide");
    else
        enna_view_cover_display_icon (o, o_pict, si->o_edje,
                                      file->icon, NULL,
                                      96, 96 * 3/2, "shadow,show");

    elm_box_pack_end(sd->o_box, o);
    si->o_pict = o_pict;
    si->o_edje = o;
    si->sd = sd;

    si->selected = 0;
    sd->items = eina_list_append(sd->items, si);

    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
            _smart_event_mouse_down, si);
}

void enna_view_cover_event_feed(Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key(ev);

    API_ENTRY return;

    switch (key)
    {
    case ENNA_KEY_LEFT:
        _view_cover_h_select (obj, 0);
        break;
    case ENNA_KEY_RIGHT:
        _view_cover_h_select (obj, 1);
        break;
    default:
        break;
    }
}

void enna_view_cover_select_nth(Evas_Object *obj, int nth)
{
    Smart_Item *si;

    API_ENTRY return;

    si = eina_list_nth(sd->items, nth);
    if (!si) return;

    _smart_item_unselect(sd, _smart_selected_item_get(sd, NULL));
    _smart_item_select(sd, si);
}

Eina_List* enna_view_cover_files_get(Evas_Object* obj)
{
    Eina_List *files = NULL;
    Eina_List *l;
    Smart_Item *it;
    
    API_ENTRY return NULL;
    
    EINA_LIST_FOREACH(sd->items, l, it)
        files = eina_list_append(files, it->file);
        
    return files;
}

void *enna_view_cover_selected_data_get(Evas_Object *obj)
{
    Smart_Item *si;
    API_ENTRY return NULL;

    si = _smart_selected_item_get(sd, NULL);
    return si ? si->data : NULL;
}

int enna_view_cover_jump_label(Evas_Object *obj, const char *label)
{
    Smart_Item *it = NULL;
    Eina_List *l;
    int i = 0;

    API_ENTRY return -1;

    if (!sd || !label) return -1;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label && !strcmp(it->label, label))
        {
            enna_view_cover_select_nth(sd->obj, i);
            return i;
        }
        i++;
    }

    return -1;
}

void enna_view_cover_jump_ascii(Evas_Object *obj, char k)
{
    Smart_Item *it;
    Eina_List *l;
    int i = 0;
    
    API_ENTRY return;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label[0] == k || it->label[0] == k - 32)
        {
            enna_view_cover_select_nth(sd->obj, i);
            return;
        }
        i++;
    }
}

/* local subsystem globals */
static void _view_cover_h_select(Evas_Object *obj, int pos)
{
    Smart_Item *si, *ssi;
    int nth;

    API_ENTRY return;

    ssi = _smart_selected_item_get(sd, &nth);
    if (!ssi)
	nth = 0;
    else
    {
	if (pos)
	    nth++;
	else
	    nth--;
    }
    si = eina_list_nth(sd->items, nth);
    if (si)
    {
        Evas_Coord x, xedje, wedje, xbox;

        evas_object_geometry_get(si->o_edje, &xedje, NULL, &wedje, NULL);
        evas_object_geometry_get(sd->o_box, &xbox, NULL, NULL, NULL);
        if (pos)
            x = (xedje + wedje / 2 - xbox + sd->w / 2 );
        else
            x = (xedje + wedje / 2 - xbox - sd->w / 2 );
        elm_scroller_region_show(sd->o_scroll, x, 0, 0, 0);

	_smart_item_select(sd, si);
	if (ssi) _smart_item_unselect(sd, ssi);
    }

}

static Smart_Item *_smart_selected_item_get(Smart_Data *sd, int *nth)
{
    Eina_List *l;
    Smart_Item *si;
    int i = 0;

    EINA_LIST_FOREACH(sd->items, l, si)
    {
	if (si->selected)
	{
	    if (nth)  *nth = i;
	    return si;
	}
	i++;
    }
    if (nth) *nth = -1;
    return NULL;
}

static void _smart_item_unselect(Smart_Data *sd, Smart_Item *si)
{
    if (!si || !si->selected) return;

    si->selected = 0;
    edje_object_signal_emit(si->o_edje, "unselect", "enna");
    evas_object_lower(si->o_edje);

}

static void _smart_item_select(Smart_Data *sd, Smart_Item *si)
{
    if (si->selected) return;

    si->selected = 1;
    edje_object_signal_emit(si->o_edje, "select", "enna");
    evas_object_raise(si->o_edje);
    evas_object_smart_callback_call (sd->obj, "hilight", si->data);
    edje_object_part_text_set(sd->o_edje, "enna.text.label", si->label);
}

static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj,
        void *event_info)
{
    Smart_Item *si = data;
    Smart_Item *spi;
    Evas_Event_Mouse_Down *ev = event_info;

    if (!si) return;
    _smart_item_unselect(si->sd, _smart_selected_item_get(si->sd, NULL));
    _smart_item_select(si->sd, si);

    spi = _smart_selected_item_get(si->sd, NULL);
    if (spi && spi != si)
    {
        _smart_item_unselect(si->sd, _smart_selected_item_get(si->sd, NULL));
    }
    else if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
    {
        if (si->func)
            si->func(si->data);
	    return;
    }
    else if (spi == si)
    {
        return;
    }

    _smart_item_select(si->sd, si);
}

static void _smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, sd->x, sd->y);
    evas_object_resize(sd->o_edje, sd->w, sd->h);
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;
    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    sd->obj = obj;
    sd->nb = -1;

    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "enna/covervideo");

    sd->o_scroll = elm_scroller_add(obj);
    evas_object_show(sd->o_scroll);
    elm_scroller_policy_set(sd->o_scroll, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
    //elm_scroller_bounce_set(sd->o_scroll, 0, 0);

    edje_object_part_swallow(sd->o_edje, "swallow.content", sd->o_scroll);

    sd->o_box = elm_box_add(sd->o_scroll);
    elm_box_homogenous_set(sd->o_box, 0);
    elm_box_horizontal_set(sd->o_box, 1);
    evas_object_size_hint_align_set(sd->o_box, 0.5, 0.5);
    evas_object_size_hint_weight_set(sd->o_box, 1.0, 1.0);

    evas_object_show(sd->o_box);
    evas_object_size_hint_weight_set(sd->o_scroll, 1.0, 1.0);
    elm_scroller_content_set(sd->o_scroll, sd->o_box);

    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_del(sd->o_edje);
    evas_object_del(sd->o_scroll);
    while (sd->items)
    {
	Smart_Item *si = sd->items->data;
	sd->items = eina_list_remove_list(sd->items, sd->items);
	evas_object_del(si->o_edje);
	evas_object_del(si->o_pict);
	free(si);
    }
    eina_list_free(sd->items);
    evas_object_del(sd->o_scroll);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_edje);
}

static void _smart_init(void)
{
    static const Evas_Smart_Class sc = {
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
Evas_Object * enna_view_cover_add(Evas * evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _smart);
}
