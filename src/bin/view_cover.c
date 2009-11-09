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
#include "input.h"
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
    Evas_Object *obj;
    Evas_Object *o_box;
    Eina_List *items;
    int horizontal;
};

/* local subsystem functions */
static void _view_cover_select(Evas_Object *obj, int pos);
static Smart_Item *_smart_selected_item_get(Smart_Data *sd, int *nth);
static void _smart_item_unselect(Smart_Data *sd, Smart_Item *si);
static void _smart_item_select(Smart_Data *sd, Smart_Item *si);
static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj,
        void *event_info);

/* local subsystem globals */

static void
enna_view_cover_display_icon (Evas_Object *o, Evas_Object *p, Evas_Object *e,
                              const char *file, const char *group,
                              Evas_Coord w, Evas_Coord h,
                              char *signal)
{
    elm_image_file_set (p, file, group);
    elm_image_smooth_set (p, 1);

    /* Fit container but keep aspect ratio */
    evas_object_size_hint_min_set (o, w, h);
    evas_object_show (p);
    edje_object_part_swallow (o, "enna.swallow.content", p);
    edje_object_signal_emit (e, signal, "enna");
    evas_object_show (o);
}

void enna_view_cover_file_append(Evas_Object *obj, Enna_Vfs_File *file,
    void (*func) (void *data), void *data)
{
    Evas_Object *o, *o_pict;
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    si = calloc(1, sizeof(Smart_Item));

    o = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(o, enna_config_theme_get(), "enna/mainmenu/item");
    si->label = eina_stringshare_add(file->label);
    o_pict = elm_image_add(obj);
    si->data = data;
    si->func = func;
    si->file = file;

    if (file->icon && file->icon[0] != '/')
        enna_view_cover_display_icon (o, o_pict, si->o_edje,
                                      enna_config_theme_get (), file->icon,
                                      32, 32, "shadow,hide");
    else
        enna_view_cover_display_icon (o, o_pict, si->o_edje,
                                      file->icon, NULL,
                                      32, 32 * 3/2, "shadow,show");
    edje_object_part_text_set(o, "enna.text.label", si->label);

    elm_box_pack_end(sd->o_box, o);
    si->o_pict = o_pict;
    si->o_edje = o;
    si->sd = sd;

    si->selected = 0;
    sd->items = eina_list_append(sd->items, si);

    evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
            _smart_event_mouse_down, si);
}

Eina_Bool
enna_view_cover_input_feed(Evas_Object *obj, enna_input event)
{
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    switch (event)
    {
    case ENNA_INPUT_LEFT:
        if (sd->horizontal)
        {
            _view_cover_select (obj, 0);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_RIGHT:
        if (sd->horizontal)
        {
            _view_cover_select (obj, 1);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_UP:
        if (!sd->horizontal)
        {
            _view_cover_select (obj, 0);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_DOWN:
        if (!sd->horizontal)
        {
            _view_cover_select (obj, 1);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_OK:
        si = _smart_selected_item_get(sd, NULL);
        if (si && si->func)
            si->func(si->data);
        return ENNA_EVENT_BLOCK;
    default:
        break;
    }

    return ENNA_EVENT_CONTINUE;
}

void enna_view_cover_select_nth(Evas_Object *obj, int nth)
{
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

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
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
        files = eina_list_append(files, it->file);

    return files;
}

void *enna_view_cover_selected_data_get(Evas_Object *obj)
{
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    si = _smart_selected_item_get(sd, NULL);
    return si ? si->data : NULL;
}

int enna_view_cover_jump_label(Evas_Object *obj, const char *label)
{
    Smart_Data *sd;
    Smart_Item *it = NULL;
    Eina_List *l;
    int i = 0;

    sd = evas_object_data_get(obj, "sd");

    if (!sd || !label) return -1;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label && !strcmp(it->label, label))
        {
            enna_view_cover_select_nth(obj, i);
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
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label[0] == k || it->label[0] == k - 32)
        {
            enna_view_cover_select_nth(obj, i);
            return;
        }
        i++;
    }
}

static void enna_view_cover_item_remove(Evas_Object *obj, Smart_Item *item)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd || !item) return;

    sd->items = eina_list_remove(sd->items, item);

    //~ enna_vfs_remove(item->file); //TODO need to free file ?
    ENNA_OBJECT_DEL(item->o_pict);
    ENNA_OBJECT_DEL(item->o_edje);
    ENNA_STRINGSHARE_DEL(item->label);
    ENNA_FREE(item);

    return;
}


void enna_view_cover_clear(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    Smart_Item *item;
    Eina_List *l, *l_next;

    EINA_LIST_FOREACH_SAFE(sd->items, l, l_next, item)
    {
        enna_view_cover_item_remove(obj, item);
    }
}

/* local subsystem globals */
static void _view_cover_select(Evas_Object *obj, int pos)
{
    Smart_Data *sd;
    Smart_Item *si, *ssi;
    int nth;

    sd = evas_object_data_get(obj, "sd");
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
        Evas_Coord x, y, w, h;
        Evas_Coord xedje, yedje, wedje, hedje;

        evas_object_geometry_get(si->o_edje, &xedje, &yedje, &wedje, &hedje);
        elm_scroller_region_get(obj, &x, &y, &w, &h);

        x += xedje;
        elm_scroller_region_bring_in(obj, x, y, wedje, hedje);

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

static void
_custom_resize(void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    Evas_Coord x, y, w, h;
    Smart_Data *sd = data;
    Eina_List *l;
    Smart_Item *it;

    elm_scroller_region_get(obj, &x, &y, &w, &h);

    EINA_LIST_FOREACH(sd->items, l, it)
        evas_object_size_hint_min_set (it->o_edje, w, h);
}

/* externally accessible functions */
Evas_Object * enna_view_cover_add(Evas * evas, int horizontal)
{

    Evas_Object *obj;
    Smart_Data *sd;

    obj = elm_scroller_add(enna->layout);
    sd = calloc(1, sizeof(Smart_Data));

    sd->obj = obj;

    elm_scroller_policy_set(obj, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
    elm_scroller_bounce_set(obj, 0, 0);

    sd->o_box = elm_box_add(obj);
    elm_box_homogenous_set(sd->o_box, 0);
    elm_box_horizontal_set(sd->o_box, horizontal);
    sd->horizontal = horizontal;

    evas_object_show(sd->o_box);
    elm_scroller_content_set(obj, sd->o_box);

    evas_object_event_callback_add(obj, EVAS_CALLBACK_RESIZE,
                                   _custom_resize, sd);

    evas_object_data_set(obj, "sd", sd);

    return obj;
}
