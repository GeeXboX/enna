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
#include <Evas.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "input.h"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data {
    Input_Listener *il;
};

static Eina_Bool
_entry_input_listener_cb(void *data, enna_input event)
{
    /* Block Enna events here, to be able to enter characters in entry */
    return EINA_FALSE;
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    if (!sd)
        return;
    /* Add an input lister when entry received focus, that blocks all Enna events */
    sd->il = enna_input_listener_add("search/entry",
                                     _entry_input_listener_cb, sd);
    /* Promote input lister to intercept ALL events */
    enna_input_listener_promote(sd->il);
    
}

static void
_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    
    if (!sd)
        return;
    /* Remove input listener when entry loose focus */
    if (sd->il)
    {
        enna_input_listener_del(sd->il);
        sd->il = NULL;
    }
}

static void
_entry_activated_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    
    if (!sd)
        return;
    /* Remove input listner when user press Enter */
    if (sd->il)
    {
        enna_input_listener_del(sd->il);
        sd->il = NULL;
    }
}

Evas_Object *
enna_search_add(Evas_Object *parent)
{
    Evas_Object *o_layout;
    Evas_Object *o_edit;
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    
    o_layout = elm_layout_add(parent);
    elm_layout_file_set(o_layout, enna_config_theme_get(), "enna/search");

    o_edit = elm_entry_add(o_layout);
    elm_entry_single_line_set(o_edit, EINA_TRUE);
    elm_entry_entry_set(o_edit, _("Search..."));

    elm_layout_content_set(o_layout, "enna.swallow.edit", o_edit);
    elm_object_style_set(o_edit, "enna");
    evas_object_data_set(o_layout, "edit", o_edit);

    evas_object_smart_callback_add(o_edit, "focused", _entry_focused_cb, sd);
    evas_object_smart_callback_add(o_edit, "unfocused", _entry_unfocused_cb, sd);
    evas_object_smart_callback_add(o_edit, "activated", _entry_activated_cb, sd);

    return o_layout;
}



