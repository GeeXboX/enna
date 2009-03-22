/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
