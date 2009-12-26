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

#include "video_picture.h"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_img;
};

static void
_del(void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    ENNA_OBJECT_DEL(sd->o_img);
    ENNA_FREE(sd);
}


/* externally accessible functions */
Evas_Object *
enna_video_picture_add(Evas * evas)
{
    Smart_Data *sd;

    sd = ENNA_NEW(Smart_Data, 1);

    sd->o_edje = edje_object_add(evas);
    edje_object_file_set(sd->o_edje, enna_config_theme_get(),
                         "activity/video/picture");
    evas_object_show(sd->o_edje);
    evas_object_data_set(sd->o_edje, "sd", sd);

    evas_object_event_callback_add(sd->o_edje, EVAS_CALLBACK_DEL,
                                   _del, sd);

    return sd->o_edje;
}

void
enna_video_picture_set (Evas_Object *obj, char *file, int from_vfs)
{
    Evas_Object *o_img_old;

    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!file)
    {
        edje_object_signal_emit(sd->o_edje, "snapshot,hide", "enna");
        ENNA_OBJECT_DEL(sd->o_img);
        return;
    }

    enna_log(ENNA_MSG_EVENT, "video_picture",
             "using snapshot filename: %s", file);
    o_img_old = sd->o_img;

    if (from_vfs)
    {
      sd->o_img = enna_image_add(evas_object_evas_get(sd->o_edje));
      enna_image_fill_inside_set(sd->o_img, 0);
      enna_image_file_set(sd->o_img, file, NULL);
    }
    else
    {
      sd->o_img = edje_object_add(evas_object_evas_get(sd->o_edje));
      edje_object_file_set(sd->o_img, enna_config_theme_get(), file);
    }
    edje_object_part_swallow(sd->o_edje,
                             "content.swallow", sd->o_img);
    edje_object_signal_emit(sd->o_edje, "snapshot,show", "enna");
    evas_object_del(o_img_old);
    evas_object_show(sd->o_img);
}

void
enna_video_picture_unset (Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    edje_object_signal_emit(sd->o_edje, "snapshot,hide", "enna");

    return;
}
