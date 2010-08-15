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

#ifndef BOX_H
#define BOX_H

#include "enna.h"
#include "vfs.h"
#include "input.h"

Evas_Object * enna_box_add(Evas_Object *parent, const char *stylel);
void enna_box_file_append(Evas_Object *obj, Enna_File *file,
     void (*func_activated) (void *data), void *data);
void enna_box_append(Evas_Object *obj, const char *label,
                     const char *description, const char *icon,
                     void (*func_activated) (void *data), void *data);
Eina_List* enna_box_files_get(Evas_Object* obj);
void enna_box_select_nth(Evas_Object *obj, int nth);
Eina_Bool enna_box_input_feed(Evas_Object *obj, enna_input event);
void *enna_box_selected_data_get(Evas_Object *obj);
int enna_box_jump_label(Evas_Object *obj, const char *label);
void enna_box_jump_ascii(Evas_Object *obj, char k);
void enna_box_clear(Evas_Object *obj);

#endif /* BOX_H */
