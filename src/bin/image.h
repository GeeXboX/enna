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

#ifndef IMAGE_H
#define IMAGE_H

#include "enna.h"

typedef enum _Enna_Image_Orient
{
    ENNA_IMAGE_ORIENT_NONE,
    ENNA_IMAGE_ROTATE_90_CW,
    ENNA_IMAGE_ROTATE_180_CW,
    ENNA_IMAGE_ROTATE_90_CCW,
    ENNA_IMAGE_FLIP_HORIZONTAL,
    ENNA_IMAGE_FLIP_VERTICAL,
    ENNA_IMAGE_FLIP_TRANSPOSE,
    ENNA_IMAGE_FLIP_TRANSVERSE
} Enna_Image_Orient;

Evas_Object *enna_image_add(Evas * evas);
void enna_image_file_set(Evas_Object * obj, const char *file, const char *key);
const char *enna_image_file_get(Evas_Object * obj);
void enna_image_smooth_scale_set(Evas_Object * obj, int smooth);
int enna_image_smooth_scale_get(Evas_Object * obj);
void enna_image_alpha_set(Evas_Object * obj, int smooth);
int enna_image_alpha_get(Evas_Object * obj);
void enna_image_load_size_set(Evas_Object * obj, int w, int h);
void enna_image_size_get(Evas_Object * obj, int *w, int *h);
int enna_image_fill_inside_get(Evas_Object * obj);
void enna_image_fill_inside_set(Evas_Object * obj, int fill_inside);
void enna_image_data_set(Evas_Object * obj, void *data, int w, int h);
void *enna_image_data_get(Evas_Object * obj, int *w, int *h);
void enna_image_preload(Evas_Object *obj, Eina_Bool cancel);
void enna_image_orient_set(Evas_Object *obj, Enna_Image_Orient orient);
#endif /* IMAGE_H */
