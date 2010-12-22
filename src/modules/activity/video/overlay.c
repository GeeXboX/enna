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

#include <Elementary.h>

#include "enna.h"

static Evas_Object *_win = NULL;

void enna_overlay_show()
{
    Evas_Object *o_layout;

    if (_win)
        return;
    _win = elm_win_add(NULL, "window-state", ELM_WIN_SPLASH);
    elm_win_borderless_set(_win, EINA_FALSE);
    elm_win_shaped_set(_win, EINA_TRUE);
    elm_win_alpha_set(_win, EINA_TRUE);

    o_layout = elm_layout_add(_win);
    elm_layout_file_set(o_layout, enna_config_theme_get(), "activity/video/overlay");

    evas_object_size_hint_weight_set(o_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(_win, o_layout); 
    evas_object_show(o_layout);
    elm_win_raise(_win);
    evas_object_resize(_win, 400, 260);
    evas_object_show(_win);
}

void enna_overlay_hide()
{
    ENNA_OBJECT_DEL(_win);
}
