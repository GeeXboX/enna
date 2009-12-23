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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "logs.h"
#include "image.h"
#include "photo_infos.h"

#define SMART_NAME "photo_panel_infos"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_pict;
};

/* local subsystem globals */
static Evas_Smart *_smart = NULL;

/* local subsystem globals */
static void _smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, x, y);
    evas_object_resize(sd->o_edje, w, h);
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;

    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->o_edje, enna_config_theme_get(),
                         "activity/photo/panel_infos");
    evas_object_show(sd->o_edje);
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    ENNA_OBJECT_DEL(sd->o_edje);
    ENNA_OBJECT_DEL(sd->o_pict);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_edje);
}

static void _smart_init(void)
{
    static const Evas_Smart_Class sc =
    {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _smart_add,
        _smart_del,
        _smart_move,
        _smart_resize,
        _smart_show,
        _smart_hide,
        _smart_color_set,
        _smart_clip_set,
        _smart_clip_unset,
        NULL,
        NULL
    };

    if (!_smart)
       _smart = evas_smart_class_new(&sc);
}

/* externally accessible functions */
Evas_Object *
photo_panel_infos_add(Evas * evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _smart);
}

/****************************************************************************/
/*                          Information Panel                               */
/****************************************************************************/

void
photo_panel_infos_set_text (Evas_Object *obj, const char *filename)
{
    Enna_Metadata *m;
    char *meta, *meta2 = NULL;

    API_ENTRY return;

    if (!filename || !ecore_file_exists(filename))
    {
        edje_object_part_text_set (sd->o_edje, "infos.panel.textblock",
	    _("No EXIF information found ..."));
        return;
    }

    m = enna_metadata_meta_new (filename);
    meta = enna_metadata_meta_get_all (m);
    if (meta)
    {
        int i, j;

        meta2 = calloc (2, strlen (meta));
        for (i = 0, j = 0; i < strlen (meta); i++)
        {
            if (meta[i] != '\n')
            {
                meta2[j] = meta[i];
                j++;
            }
            else
            {
                meta2[j]   = '<';
                meta2[j+1] = 'b';
                meta2[j+2] = 'r';
                meta2[j+3] = '>';
                j += 4;
            }
        }
    }

    edje_object_part_text_set (sd->o_edje, "infos.panel.textblock",
                               meta2 ? meta2:
                               _("No EXIF such information found ..."));
    ENNA_FREE (meta);
    ENNA_FREE (meta2);
    enna_metadata_meta_free (m);
}

void
photo_panel_infos_set_cover(Evas_Object *obj, const char *filename)
{
    Evas_Object *o_pict;

    API_ENTRY return;

    if (!filename) return;

    o_pict = enna_image_add (evas_object_evas_get(sd->o_edje));
    enna_image_fill_inside_set (o_pict, 0);
    enna_image_file_set (o_pict, filename, NULL);
    ENNA_OBJECT_DEL (sd->o_pict);
    sd->o_pict = o_pict;
    edje_object_part_swallow (sd->o_edje,
                              "infos.panel.cover.swallow", sd->o_pict);
    edje_object_signal_emit (sd->o_edje, "cover,show", "enna");
}

