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

#ifndef PHOTO_SLIDESHOW_VIEW_H
#define PHOTO_SLIDESHOW_VIEW_H

Evas_Object *enna_photo_slideshow_add(Evas_Object *parent);
void enna_photo_slideshow_next(Evas_Object *obj);
void enna_photo_slideshow_previous(Evas_Object *obj);
int enna_photo_slideshow_timeout_get(Evas_Object *obj);
void enna_photo_slideshow_timeout_set(Evas_Object *obj, int to);
void enna_photo_slideshow_image_add(Evas_Object *obj, const char *file, const char *group);
void enna_photo_slideshow_goto(Evas_Object *obj, int nth);

#endif /* PHOTO_SLIDESHOW_VIEW_H */
