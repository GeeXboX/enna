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

#include "enna.h"
#include "reflection.h"


#define SMART_NAME "reflection"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord          x, y, w, h;
    Evas_Object        *obj;
    Evas_Object        *reflection;
    char                fill_inside:1;
    unsigned int       *pixels;
};

/* local subsystem functions */
static void         _enna_reflection_smart_reconfigure(Smart_Data * sd);
static void         _enna_reflection_smart_init(void);

/* local subsystem globals */
static Evas_Smart  *_e_smart = NULL;

EAPI void
enna_reflection_file_set(Evas_Object * obj, const char *file)
{
    INTERNAL_ENTRY;
    evas_object_image_load_size_set(sd->obj, 800, 800);
    evas_object_image_file_set(sd->obj, file, NULL);
    _enna_reflection_smart_reconfigure(sd);
}

EAPI const char    *
enna_reflection_file_get(Evas_Object * obj)
{
    const char         *file;
    INTERNAL_ENTRY_RETURN NULL;

    evas_object_image_file_get(sd->obj, &file, NULL);
    return file;
}

EAPI void
enna_reflection_smooth_scale_set(Evas_Object * obj, int smooth)
{
    INTERNAL_ENTRY;
    evas_object_image_smooth_scale_set(sd->obj, smooth);
    evas_object_image_smooth_scale_set(sd->reflection, smooth);
}

EAPI int
enna_reflection_smooth_scale_get(Evas_Object * obj)
{
    INTERNAL_ENTRY_RETURN 0;
    return evas_object_image_smooth_scale_get(sd->obj);
}

EAPI void
enna_reflection_alpha_set(Evas_Object * obj, int alpha)
{
    INTERNAL_ENTRY;
    evas_object_image_alpha_set(sd->obj, alpha);
    evas_object_image_alpha_set(sd->reflection, alpha);
}

EAPI int
enna_reflection_alpha_get(Evas_Object * obj)
{
    INTERNAL_ENTRY_RETURN 0;
    return evas_object_image_alpha_get(sd->obj);
}

EAPI void
enna_reflection_size_get(Evas_Object * obj, int *w, int *h)
{
    Evas_Coord          rw, rh;
    INTERNAL_ENTRY;

    evas_object_image_size_get(sd->obj, w, h);
    evas_object_image_size_get(sd->reflection, &rw, &rh);

    *w += rw;
    *h += rh;
}

EAPI int
enna_reflection_fill_inside_get(Evas_Object * obj)
{
    INTERNAL_ENTRY_RETURN 0;
    return (sd->fill_inside) ? 1 : 0;
}

EAPI void
enna_reflection_fill_inside_set(Evas_Object * obj, int fill_inside)
{
    INTERNAL_ENTRY;

    if (((sd->fill_inside) && (fill_inside)) ||
        ((!sd->fill_inside) && (!fill_inside)))
        return;
    sd->fill_inside = fill_inside;
    _enna_reflection_smart_reconfigure(sd);
}

EAPI void
enna_reflection_data_set(Evas_Object * obj, void *data, int w, int h)
{
    INTERNAL_ENTRY;
    evas_object_image_size_set(sd->obj, w, h);
    evas_object_image_data_copy_set(sd->obj, data);
}

EAPI void          *
enna_reflection_data_get(Evas_Object * obj, int *w, int *h)
{
    INTERNAL_ENTRY_RETURN NULL;
    evas_object_image_size_get(sd->obj, w, h);
    return evas_object_image_data_get(sd->obj, 0);
}

/* local subsystem globals */
static void
_enna_reflection_smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord          x, y, w, h;
    unsigned int       *old_pixels;
    int                 i, j;
    unsigned int        alpha_old;
    char                alpha;
    const char         *file;


    x = sd->x;
    y = sd->y;
    h = sd->h;
    w = sd->w;
    evas_object_image_file_get(sd->obj, &file, NULL);
    evas_object_image_load_size_set(sd->obj, w, w);
    evas_object_image_file_set(sd->obj, file, NULL);
    evas_object_move(sd->obj, x, y);
    evas_object_image_fill_set(sd->obj, 0, 0, w, h*2/3);
    evas_object_resize(sd->obj, w, h*2/3);

    evas_object_image_load_size_set(sd->reflection, w, w);
    evas_object_image_file_set(sd->reflection, file, NULL);
    evas_object_move(sd->reflection, x, y + h*2/3+2);
    evas_object_image_fill_set(sd->reflection, 0, 0, w, h*2/3);
    evas_object_resize(sd->reflection, w, h*2/3);
    evas_object_image_alpha_set(sd->reflection, 1);

    /* Get pixels */
    old_pixels = evas_object_image_data_get(sd->obj, 1);
    evas_object_image_size_get(sd->obj, &w, &h);

    if (!old_pixels)
        return;

    if (sd->pixels)
        free(sd->pixels);

    sd->pixels = malloc(sizeof(int) * w * h * 2/3);

    /* For each pixels decrease alpha after each line */
    alpha = 80;
    for (i = 0; i < h * 2/3; i++)
    {
        for (j = 0; j < w; j++)
        {
            /* Premul :
             * r_old = r_vrai * (a_old / 255)
             * r_new = r_vrai * a_new / 255
             * donc r_new = r_old * a_new / a_old
             * r_old = Red Value of old_pixels
             */
            alpha_old = (0xFF000000 & old_pixels[i * w + j] >> 24);

            if (alpha_old == 0)
                alpha_old = 255;

            sd->pixels[i * w + j] = (alpha << 24) |
                (((unsigned char)((0x00FF0000 & old_pixels[(h - i - 1) * w + j]) >> 16) * alpha /    alpha_old) << 16)
                | (((unsigned char)((0x0000FF00 & old_pixels[(h - i - 1) * w + j]) >> 8) * alpha / alpha_old) << 8)
                | (((unsigned char)((0x000000FF & old_pixels[(h - i - 1) * w + j]) >> 0) * alpha / alpha_old) << 0);
        }
        alpha--;
        if (alpha <= 0)
            alpha = 0;

    }
    /* Set pixels */
    evas_object_image_data_set(sd->reflection, sd->pixels);
    evas_object_show(sd->obj);
    evas_object_show(sd->reflection);

}

static void
_e_smart_add(Evas_Object * obj)
{
    Smart_Data       *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    sd->obj = evas_object_image_add(evas_object_evas_get(obj));
    sd->reflection = evas_object_image_add(evas_object_evas_get(obj));
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;
    sd->h = 0;
    sd->fill_inside = 1;
    sd->pixels = NULL;

    evas_object_image_alpha_set(sd->obj, 1);
    evas_object_image_alpha_set(sd->reflection, 1);
    evas_object_smart_member_add(sd->obj, obj);
    evas_object_smart_member_add(sd->reflection, obj);
    evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_del(sd->obj);
    evas_object_del(sd->reflection);
    free(sd->pixels);
    free(sd);
}

static void
_e_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _enna_reflection_smart_reconfigure(sd);
}

static void
_e_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _enna_reflection_smart_reconfigure(sd);
}

static void
_e_smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->obj);
    evas_object_show(sd->reflection);

}

static void
_e_smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->obj);
    evas_object_hide(sd->reflection);
}

static void
_e_smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->obj, r, g, b, a);
    evas_object_color_set(sd->reflection, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->obj, clip);
    evas_object_clip_set(sd->reflection, clip);
}

static void
_e_smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->obj);
    evas_object_clip_unset(sd->reflection);
}

static void
_enna_reflection_smart_init(void)
{
    static const Evas_Smart_Class sc = {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _e_smart_add,
        _e_smart_del,
        _e_smart_move,
        _e_smart_resize,
        _e_smart_show,
        _e_smart_hide,
        _e_smart_color_set,
        _e_smart_clip_set,
        _e_smart_clip_unset,
        NULL
    };

    if (!_e_smart)
       _e_smart = evas_smart_class_new(&sc);
}

/* externally accessible functions */
EAPI Evas_Object   *
enna_reflection_add(Evas * evas)
{
    _enna_reflection_smart_init();
    return evas_object_smart_add(evas, _e_smart);
}
