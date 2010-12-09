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

#include <string.h>

#include <Ecore.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "image.h"
#include "logs.h"
#include "input.h"
#include "box.h"

#define SMART_NAME "enna_box"

#define EDJE_GROUP_BOX_LAYOUT "enna/box/layout"

typedef struct _Smart_Data Smart_Data;
typedef struct _Smart_Item Smart_Item;

struct _Smart_Item
{
    Evas_Object *o_edje;
    Evas_Object *o_icon; // Elm image object
    Smart_Data *sd;
    const char *label;
    Enna_File *file;
    void *data;
    void (*func_activated) (void *data);
    unsigned char selected : 1;
};

struct _Smart_Data
{
    Evas_Object *o_layout;
    Evas_Object *o_scroll;
    Evas_Object *o_box;
    Eina_List *items;
    int horizontal;
    const char *style;
    Evas_Coord h;
};

/* local subsystem functions */
static void _box_select(Evas_Object *obj, int pos);
static Smart_Item *_smart_selected_item_get(Smart_Data *sd, int *nth);
static void _smart_item_unselect(Smart_Data *sd, Smart_Item *si);
static void _smart_item_select(Smart_Data *sd, Smart_Item *si);
static void _smart_event_mouse_up(void *data, Evas *evas, Evas_Object *obj,
        void *event_info);

/* local subsystem globals */

static Smart_Item *
_append_helper(Evas_Object *obj, const char *label,
               const char *description, const char *icon,
               void (*func_activated) (void *data), void *data)
{
    Evas_Object *o_edje;
    Smart_Item *si;
    Smart_Data *sd;
    Evas_Coord w, h;
    char tmp_style[128];

    sd = evas_object_data_get(obj, "sd");

    si = calloc(1, sizeof(Smart_Item));

    if (sd->style)
        snprintf(tmp_style, sizeof(tmp_style), "enna/box/item/%s", sd->style);
    else
        snprintf(tmp_style, sizeof(tmp_style), "enna/box/item/default");

    si->o_edje = elm_layout_add(sd->o_box);
    elm_layout_file_set(si->o_edje, enna_config_theme_get(), tmp_style);
    si->label = eina_stringshare_add(label);
    si->data = data;
    si->func_activated = func_activated;

    o_edje = elm_layout_edje_get(si->o_edje);
    if (icon)
    {
        si->o_icon = elm_icon_add(enna->layout);
        elm_icon_file_set(si->o_icon, enna_config_theme_get(), icon);
        evas_object_show(si->o_icon);
        edje_object_part_swallow (o_edje, "enna.swallow.icon", si->o_icon);
    }
    if (label)
        edje_object_part_text_set(o_edje, "enna.text.label", label);
    if(description)
        edje_object_part_text_set(o_edje, "enna.text.description", description);

    evas_object_show(si->o_edje);

    evas_object_size_hint_weight_set(si->o_edje, 1.0, 1.0);
    evas_object_size_hint_align_set(si->o_edje, -1.0, -1.0);

    edje_object_size_min_calc(si->o_edje, &w, &h);
    evas_object_size_hint_min_set(o_edje, w, h);

    si->sd = sd;
    si->selected = 0;

    sd->items = eina_list_append(sd->items, si);

    elm_box_pack_end(sd->o_box, si->o_edje);

    evas_object_event_callback_add(si->o_edje, EVAS_CALLBACK_MOUSE_UP,
            _smart_event_mouse_up, si);

    return si;
}

void
enna_box_file_append(Evas_Object *obj, Enna_File *file,
                            void (*func_activated) (void *data), void *data)
{
    Smart_Item *si;

    si = _append_helper(obj, file->label, NULL, file->icon,
                   func_activated, data);
    si->file = file;
}

void enna_box_append(Evas_Object *obj, const char *label,
                     const char *description, const char *icon,
                     void (*func_activated) (void *data), void *data)
{
    _append_helper(obj, label, description, icon,
                   func_activated, data);
}

Eina_Bool
enna_box_input_feed(Evas_Object *obj, enna_input event)
{
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    si = _smart_selected_item_get(sd, NULL);
    switch (event)
    {
    case ENNA_INPUT_LEFT:
        if (sd->horizontal)
        {
            _box_select (obj, 0);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_RIGHT:
        if (sd->horizontal)
        {
            _box_select (obj, 1);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_UP:
        if (!sd->horizontal)
        {
            _box_select (obj, 0);
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_DOWN:
        if (!sd->horizontal)
        {
            _box_select (obj, 1);
        }
        break;
    case ENNA_INPUT_OK:
        if (si && si->func_activated)
            si->func_activated(si->data);
        return ENNA_EVENT_BLOCK;
    default:
        break;
    }

    return ENNA_EVENT_CONTINUE;
}

void
enna_box_select_nth(Evas_Object *obj, int nth)
{
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    si = eina_list_nth(sd->items, nth);
    if (!si) return;

    _smart_item_unselect(sd, _smart_selected_item_get(sd, NULL));
    _smart_item_select(sd, si);
}

Eina_List *
enna_box_files_get(Evas_Object* obj)
{
    Eina_List *files = NULL;
    Eina_List *l;
    Smart_Item *it;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
        files = eina_list_append(files, it->file);

    return files;
}

void *
enna_box_selected_data_get(Evas_Object *obj)
{
    Smart_Item *si;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    si = _smart_selected_item_get(sd, NULL);
    return si ? si->data : NULL;
}

int
enna_box_jump_label(Evas_Object *obj, const char *label)
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
            enna_box_select_nth(obj, i);
            return i;
        }
        i++;
    }

    return -1;
}

void
enna_box_jump_ascii(Evas_Object *obj, char k)
{
    Smart_Item *it;
    Eina_List *l;
    int i = 0;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label[0] == k || it->label[0] == k - 32)
        {
            enna_box_select_nth(obj, i);
            return;
        }
        i++;
    }
}

static void
enna_box_item_remove(Evas_Object *obj, Smart_Item *item)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd || !item) return;

    sd->items = eina_list_remove(sd->items, item);
    ENNA_OBJECT_DEL(item->o_icon);
    ENNA_OBJECT_DEL(item->o_edje);
    ENNA_STRINGSHARE_DEL(item->label);
    ENNA_FREE(item);

    return;
}


void
enna_box_clear(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    Smart_Item *item;
    Eina_List *l, *l_next;

    elm_box_clear(sd->o_box);

    EINA_LIST_FOREACH_SAFE(sd->items, l, l_next, item)
    {
        enna_box_item_remove(obj, item);
    }
}

/* local subsystem globals */
static void
_box_select(Evas_Object *obj, int pos)
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
        elm_scroller_region_get(sd->o_scroll, &x, &y, &w, &h);

        if (sd->horizontal)
            x += xedje;
        else
            y += yedje;

        elm_scroller_region_bring_in(sd->o_scroll, x, y, wedje, hedje);

        _smart_item_select(sd, si);
        if (ssi) _smart_item_unselect(sd, ssi);
    }

}

static Smart_Item *
_smart_selected_item_get(Smart_Data *sd, int *nth)
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

static void
_smart_item_unselect(Smart_Data *sd, Smart_Item *si)
{
    Evas_Object *o_edje;

    if (!si || !si->selected) return;

    o_edje = elm_layout_edje_get(si->o_edje);

    si->selected = 0;
    edje_object_signal_emit(o_edje, "unselect", "enna");
    evas_object_lower(si->o_edje);

}

static void
_smart_item_select(Smart_Data *sd, Smart_Item *si)
{
    Evas_Object *o_edje;
    if (si->selected) return;

    o_edje = elm_layout_edje_get(si->o_edje);

    si->selected = 1;
    edje_object_signal_emit(o_edje, "select", "enna");
    evas_object_raise(si->o_edje);
    evas_object_smart_callback_call (sd->o_layout, "hilight", si->data);
}

static void
_smart_event_mouse_up(void *data, Evas *evas,
                        Evas_Object *obj, void *event_info)
{
    Smart_Item *si = data;
    Smart_Item *spi;
 
    if (!si) return;
    _smart_item_unselect(si->sd, _smart_selected_item_get(si->sd, NULL));
    _smart_item_select(si->sd, si);

    spi = _smart_selected_item_get(si->sd, NULL);
    if (spi && spi != si)
    {
        _smart_item_unselect(si->sd, _smart_selected_item_get(si->sd, NULL));
    }
    else if (spi == si)
    {
        if (si->func_activated)
            si->func_activated(si->data);
        return;
    }
}

static void
_box_resize(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Smart_Data *sd = data;
    Eina_List *l;
    Evas_Coord w, h;
    Smart_Item *si;

    elm_scroller_region_get(sd->o_scroll, NULL, NULL, &w, &h);
    evas_object_size_hint_min_get(sd->o_box, NULL, &h);

    if (h)
        sd->h = h;

    evas_object_size_hint_min_set(sd->o_box, w, sd->h);
    evas_object_resize(sd->o_box, w, sd->h);

    EINA_LIST_FOREACH(sd->items, l, si)
    {
        edje_object_size_min_calc(si->o_edje, &w, &h);
        evas_object_size_hint_min_set(si->o_edje, w, h);
    }
}

static void
_del_cb(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Smart_Data *sd = data;

    enna_box_clear(sd->o_layout);
    ENNA_OBJECT_DEL(sd->o_scroll);
    //ENNA_OBJECT_DEL(sd->o_layout);
    eina_stringshare_del(sd->style);
    ENNA_FREE(sd);
}

/* externally accessible functions */
Evas_Object *
enna_box_add(Evas_Object *parent, const char *style)
{
    Smart_Data *sd;
    Evas_Object *o_edje;
    const char *s;
    Eina_Bool bw, bh;
    Elm_Scroller_Policy pw, ph;
    char tmp_style[128];

    sd = calloc(1, sizeof(Smart_Data));

    sd->style = eina_stringshare_add(style);

    if (sd->style)
        snprintf(tmp_style, sizeof(tmp_style), EDJE_GROUP_BOX_LAYOUT"/%s", sd->style);
    else
        snprintf(tmp_style, sizeof(tmp_style), EDJE_GROUP_BOX_LAYOUT"/default");

    sd->o_layout = elm_layout_add(parent);
    if (!elm_layout_file_set(sd->o_layout, enna_config_theme_get(), tmp_style))
    { 
        CRIT("Unable to find group \"%s\" in theme %s", tmp_style, enna_config_theme_get());
        return NULL;
    }
    o_edje = elm_layout_edje_get(sd->o_layout);
    evas_object_show(sd->o_layout);

    s = edje_object_data_get(o_edje, "scroll_horizontal");
    if (s && (strcmp(s, "off") == 0))
        pw = ELM_SCROLLER_POLICY_OFF;
    else if (s && (strcmp(s, "on") == 0))
        pw  = ELM_SCROLLER_POLICY_ON;
    else
        pw = ELM_SCROLLER_POLICY_AUTO;

    s = edje_object_data_get(o_edje, "scroll_vertical");
    if (s && (strcmp(s, "off") == 0))
        ph = ELM_SCROLLER_POLICY_OFF;
    else if (s && (strcmp(s, "on") == 0))
        ph  = ELM_SCROLLER_POLICY_ON;
    else
        ph = ELM_SCROLLER_POLICY_AUTO;

    s = edje_object_data_get(o_edje, "bounce_horizontal");
    if (s && (strcmp(s, "off") == 0))
        bw = EINA_FALSE;
    else if (s && (strcmp(s, "on") == 0))
        bw  = EINA_TRUE;
    else
        bw = EINA_TRUE;

    s = edje_object_data_get(o_edje, "bounce_vertical");
    if (s && (strcmp(s, "off") == 0))
        bh = EINA_FALSE;
    else if (s && (strcmp(s, "on") == 0))
        bh  = EINA_TRUE;
    else
        bh = EINA_TRUE;

    s = edje_object_data_get(o_edje, "orientation");
    if (s && (strcmp(s, "vertical") == 0))
        sd->horizontal = EINA_FALSE;
    else if (s && (strcmp(s, "horizontal") == 0))
        sd->horizontal  = EINA_TRUE;
    else
        sd->horizontal = EINA_FALSE;

    sd->o_scroll = elm_scroller_add(sd->o_layout);
    elm_object_style_set(sd->o_scroll, "enna");

    elm_scroller_policy_set(sd->o_scroll, pw, ph);
    elm_scroller_bounce_set(sd->o_scroll, bw, bh);

    sd->o_box = elm_box_add(sd->o_scroll);
    elm_box_horizontal_set(sd->o_box, sd->horizontal);
    elm_box_homogenous_set(sd->o_box, 1);
    evas_object_show(sd->o_box);

    elm_scroller_content_set(sd->o_scroll, sd->o_box);
    elm_layout_content_set(sd->o_layout, "enna.swallow.content", sd->o_scroll);
    elm_scroller_content_min_limit(sd->o_scroll, 1, 1);
    evas_object_size_hint_weight_set(sd->o_box, 1.0, 1.0);
    evas_object_size_hint_align_set(sd->o_box, -1.0, -1.0);

    evas_object_show(sd->o_scroll);

    evas_object_event_callback_add
        (sd->o_scroll, EVAS_CALLBACK_RESIZE, _box_resize, sd);
    evas_object_event_callback_add
        (sd->o_layout, EVAS_CALLBACK_DEL, _del_cb, sd);

    evas_object_data_set(sd->o_layout, "sd", sd);

    return sd->o_layout;
}
