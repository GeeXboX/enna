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

Evas_Object *
enna_search_add(Evas_Object *parent)
{
    Evas_Object *o_layout;
    Evas_Object *o_edit;
    
    o_layout = elm_layout_add(parent);
    elm_layout_file_set(o_layout, enna_config_theme_get(), "enna/search");

    o_edit = elm_scrolled_entry_add(o_layout);
    elm_scrolled_entry_bounce_set(o_edit, EINA_FALSE, EINA_FALSE);
    elm_scrolled_entry_line_char_wrap_set(o_edit, EINA_FALSE);
    elm_scrolled_entry_line_wrap_set(o_edit, EINA_FALSE);
    elm_scrolled_entry_entry_set(o_edit, _("Search..."));

    elm_layout_content_set(o_layout, "enna.swallow.edit", o_edit);
    elm_object_style_set(o_edit, "enna");
    evas_object_data_set(o_layout, "edit", o_edit);
    return o_layout;
}



