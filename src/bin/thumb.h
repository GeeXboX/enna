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

#ifndef ENNA_THUMB_H
#define ENNA_THUMB_H

#include <Evas.h>
#include <Ecore_Ipc.h>

#include "image.h"

int                   enna_thumb_init(void);
int                   enna_thumb_shutdown(void);

Evas_Object          *enna_thumb_icon_add(Evas *evas);
void                  enna_thumb_icon_file_set(Evas_Object *obj, const char *file, const char *key);
void                  enna_thumb_icon_size_set(Evas_Object *obj, int w, int h);
void                  enna_thumb_icon_size_get(Evas_Object *obj, int *w, int *h);
const char*           enna_thumb_icon_file_get(Evas_Object *obj);
void                  enna_thumb_icon_orient_set(Evas_Object *obj, Enna_Image_Orient orient);
void                  enna_thumb_icon_begin(Evas_Object *obj);
void                  enna_thumb_icon_end(Evas_Object *obj);
void                  enna_thumb_icon_rethumb(Evas_Object *obj);

void                  enna_thumb_client_data(Ecore_Ipc_Event_Client_Data *e);
void                  enna_thumb_client_del(Ecore_Ipc_Event_Client_Del *e);

#endif

