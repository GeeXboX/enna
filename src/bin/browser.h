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

#ifndef BROWSER_H
#define BROWSER_H

#include "vfs.h"
#include "input.h"

typedef struct _Browser_Selected_File_Data Browser_Selected_File_Data;

struct _Browser_Selected_File_Data
{
    Enna_Class_Vfs *vfs;
    Enna_Vfs_File *file;
    Eina_List *files;
};

typedef enum _Enna_Browser_View_Type Enna_Browser_View_Type;

enum _Enna_Browser_View_Type
{
    ENNA_BROWSER_VIEW_LIST,
    ENNA_BROWSER_BOX,
    ENNA_BROWSER_VIEW_WALL
};

Evas_Object    *enna_browser_add(Evas * evas);
void            enna_browser_view_add(Evas_Object *obj, Enna_Browser_View_Type view_type);
void            enna_browser_root_set(Evas_Object *obj, Enna_Class_Vfs *vfs);
void            enna_browser_show_file_set(Evas_Object *obj, unsigned char show);
void            enna_browser_input_feed(Evas_Object *obj, enna_input event);
int             enna_browser_select_label(Evas_Object *obj, const char *label);
Eina_List      *enna_browser_files_get(Evas_Object *obj);
#endif /* BROWSER_H */
