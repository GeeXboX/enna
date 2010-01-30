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

#ifndef REFLECTION_H
#define REFLECTION_H

#include "enna.h"

EAPI Evas_Object   *enna_reflection_add(Evas * evas);
EAPI void           enna_reflection_file_set(Evas_Object * obj,
					     const char *file);
EAPI const char    *enna_reflection_file_get(Evas_Object * obj);
EAPI void           enna_reflection_smooth_scale_set(Evas_Object * obj,
						     int smooth);
EAPI int            enna_reflection_smooth_scale_get(Evas_Object * obj);
EAPI void           enna_reflection_alpha_set(Evas_Object * obj, int smooth);
EAPI int            enna_reflection_alpha_get(Evas_Object * obj);
EAPI void           enna_reflection_size_get(Evas_Object * obj, int *w, int *h);
EAPI int            enna_reflection_fill_inside_get(Evas_Object * obj);
EAPI void           enna_reflection_fill_inside_set(Evas_Object * obj,
						    int fill_inside);
EAPI void           enna_reflection_data_set(Evas_Object * obj, void *data,
					     int w, int h);
EAPI void          *enna_reflection_data_get(Evas_Object * obj, int *w, int *h);

#endif /* REFLECTION_H */
