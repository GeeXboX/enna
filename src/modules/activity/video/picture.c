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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "logs.h"
#include "image.h"
#include "picture.h"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_img;
};

/* local subsystem globals */


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
        evas_object_del(sd->o_img);
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
