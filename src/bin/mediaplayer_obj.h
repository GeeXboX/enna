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

#ifndef MEDIAPLAYER_OBJ_H
#define MEDIAPLAYER_OBJ_H

#include "enna.h"

Evas_Object *enna_mediaplayer_obj_add(Evas * evas, Enna_Playlist *enna_playlist);
Eina_Bool enna_mediaplayer_obj_input_feed(Evas_Object *obj, enna_input event);
unsigned char enna_mediaplayer_show_get(Evas_Object *obj);
void enna_mediaplayer_obj_layout_set(Evas_Object *obj, const char *layout);
void enna_mediaplayer_position_update(Evas_Object *obj);
void enna_mediaplayer_obj_event_catch(Evas_Object *obj);
void enna_mediaplayer_obj_event_release(void);
#endif /* MEDIAPLAYER_OBJ_H */
