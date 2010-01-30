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

#ifndef WALL_H
#define WALL_H

#include "enna.h"
#include "vfs.h"
#include "input.h"

Evas_Object *enna_wall_add(Evas * evas);
void enna_wall_file_append(Evas_Object *obj, Enna_Vfs_File *file,
    void (*func_activated) (void *data), void *data);
Eina_List* enna_wall_files_get(Evas_Object* obj);
void enna_wall_select_nth(Evas_Object *obj, int col, int row);
Eina_Bool enna_wall_input_feed(Evas_Object *obj, enna_input ev);
void *enna_wall_selected_data_get(Evas_Object *obj);

void enna_wall_selected_geometry_get(Evas_Object *obj, int *x, int *y, int *w, int *h);
const char *enna_wall_selected_filename_get(Evas_Object *obj);

int enna_wall_jump_label(Evas_Object *obj, const char *label);
void enna_wall_jump_ascii(Evas_Object *obj, char k);

#endif /* WALL_H */
