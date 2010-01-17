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

#ifndef LIST2_H
#define LIST2_H

#include <Elementary.h>

#include "enna.h"
#include "vfs.h"
#include "input.h"


Evas_Object      *enna_list2_add(Evas *evas);
Elm_Genlist_Item *enna_list2_append(Evas_Object *obj, const char *label1,
                                    const char *label2, const char *icon,
                                    void (*func)(void *data), void *func_data);
Elm_Genlist_Item *enna_list2_item_insert_after(Evas_Object *obj,
                                               Elm_Genlist_Item *after,
                                               const char *label1, const char *label2,
                                               const char *icon,
                                               void(*func)(void *data),
                                               void *func_data);
Elm_Genlist_Item *enna_list2_item_insert_before(Evas_Object *obj,
                                               Elm_Genlist_Item *before,
                                               const char *label1, const char *label2,
                                               const char *icon,
                                               void(*func)(void *data),
                                               void *func_data);
void          enna_list2_file_append(Evas_Object *obj, Enna_Vfs_File *file,
                                    void (*func) (void *data), void *func_data);

void          enna_list2_item_button_add(Elm_Genlist_Item *item,
                                         const char *icon, const char *label,
                                         void (*func) (void *data), void *func_data);
void          enna_list2_item_toggle_add(Elm_Genlist_Item *item,
                                         const char *icon, const char *label,
                                         void (*func) (void *data), void *func_data);
void          enna_list2_item_check_add(Elm_Genlist_Item *item,
                                        const char *icon, const char *label,
                                        Eina_Bool status,
                                        void (*func) (void *data), void *func_data);
void          enna_list2_item_entry_add(Elm_Genlist_Item *item,
                                        const char *icon, const char *label,
                                        void (*func) (void *data), void *func_data);
void          enna_list2_item_del(Elm_Genlist_Item *item);

//~ Eina_List* enna_list_files_get(Evas_Object* obj);
//~ void enna_list_select_nth(Evas_Object *obj, int nth);
Eina_Bool     enna_list2_input_feed(Evas_Object *obj, enna_input event);
//~ void * enna_list_selected_data_get(Evas_Object *obj);
//~ int enna_list_jump_label(Evas_Object *obj, const char *label);
//~ void enna_list_jump_ascii(Evas_Object *obj, char k);

#endif /* LIST2_H */

