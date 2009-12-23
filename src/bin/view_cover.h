/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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

#ifndef VIEW_COVER_H
#define VIEW_COVER_H

#include "enna.h"
#include "vfs.h"

Evas_Object * enna_view_cover_add(Evas * evas, int horizontal);
void enna_view_cover_file_append(Evas_Object *obj, Enna_Vfs_File *file,
     void (*func_activated) (void *data), void *data);
Eina_List* enna_view_cover_files_get(Evas_Object* obj);
void enna_view_cover_select_nth(Evas_Object *obj, int nth);
Eina_Bool enna_view_cover_input_feed(Evas_Object *obj, enna_input event);
void *enna_view_cover_selected_data_get(Evas_Object *obj);
int enna_view_cover_jump_label(Evas_Object *obj, const char *label);
void enna_view_cover_jump_ascii(Evas_Object *obj, char k);
void enna_view_cover_clear(Evas_Object *obj);

#endif /* VIEW_COVER_H */
