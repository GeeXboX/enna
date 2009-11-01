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

#include "enna_config.h"
#include "image.h"
#include "metadata.h"
#include "mediaplayer.h"
#include "mediaplayer_obj.h"
#include "mediacontrol.h"
#include "utils.h"

#define SMART_NAME "smart_mediaplayer"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_cover;
    Evas_Object *o_mediacontrol;
};

/* local subsystem globals */
static Evas_Smart *_smart = NULL;
static Enna_Playlist *_enna_playlist;

static void _drag_bar_seek_cb(void *data, Evas_Object *obj,
        const char *emission, const char *source)
{
/*
    double value;
    edje_object_part_drag_value_get(obj, "enna.dragable.pos", &value, NULL);
    enna_mediaplayer_seek(value);
*/
}

void enna_smart_player_position_set(Evas_Object *obj, double pos,
        double len, double percent)
{
/*
    long ph, pm, ps, lh, lm, ls;
    char buf[256];
    char buf2[256];
    API_ENTRY
    ;

    lh = len / 3600000;
    lm = len / 60 - (lh * 60);
    ls = len - (lm * 60);
    ph = pos / 3600;
    pm = pos / 60 - (ph * 60);
    ps = pos - (pm * 60);
    snprintf(buf, sizeof(buf), "%02li:%02li", pm, ps);
    snprintf(buf2, sizeof(buf2), "%02li:%02li", lm, ls);

    edje_object_part_text_set(sd->o_edje, "enna.text.length", buf2);
    edje_object_part_text_set(sd->o_edje, "enna.text.position", buf);
    edje_object_part_drag_value_set(sd->o_edje, "enna.dragable.pos", percent,
            0.0);
*/
}

static void metadata_set_text (Evas_Object *obj, Enna_Metadata *m,
                               const char *name, const char *edje, int max)
{
/*
    char *str;

    API_ENTRY;

    str = enna_metadata_meta_get (m, name, max);
    edje_object_part_text_set(sd->o_edje, edje, str ? str : "");
    ENNA_FREE(str);
*/
}

void enna_smart_player_metadata_set(Evas_Object *obj,
                                    Enna_Metadata *metadata)
{
/*
    char *cover;

    API_ENTRY;

    if (!metadata)
        return;

    metadata_set_text (obj, metadata, "title", "enna.text.title", 1);
    metadata_set_text (obj, metadata, "album", "enna.text.album", 1);
    metadata_set_text (obj, metadata, "author", "enna.text.artist", 1);

    ENNA_OBJECT_DEL(sd->o_cover);
    cover = enna_metadata_meta_get (metadata, "cover", 1);
    if (cover)
    {
        char cv[1024] = { 0 };

        if (*cover == '/')
          snprintf (cv, sizeof (cv), "%s", cover);
        else
          snprintf (cv, sizeof (cv), "%s/.enna/covers/%s",
                    enna_util_user_home_get (), cover);
        sd->o_cover = enna_image_add(evas_object_evas_get(sd->o_edje));
        enna_image_fill_inside_set(sd->o_cover, 0);
        enna_image_file_set(sd->o_cover, cv, NULL);
    }
    else
    {
        sd->o_cover = edje_object_add(evas_object_evas_get(sd->o_edje));
        edje_object_file_set(sd->o_cover,
                             enna_config_theme_get(), "icon/unknown_cover");
    }
    edje_object_part_swallow(sd->o_edje, "enna.swallow.cover", sd->o_cover);
    edje_object_signal_emit(sd->o_edje, "cover,show", "enna");
    ENNA_FREE(cover);
*/
}







/* externally accessible functions */
Evas_Object *
enna_smart_player_add(Evas * evas, Enna_Playlist *enna_playlist)
{
    Evas_Object *obj;
    Evas_Object *fr;
    Evas_Object *tb;
    Evas_Object *bt;
    Evas_Object *cv;
    Evas_Object *lb;


    fr = elm_frame_add(enna->layout);
    tb = elm_table_add(enna->layout);
    evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_frame_content_set(fr, tb);
    evas_object_show(fr);
    evas_object_show(tb);


    cv = elm_image_add(fr);
    elm_image_file_set(cv, enna_config_theme_get(), "icon/unknown_cover");
    //evas_object_size_hint_weight_set(cv, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    //evas_object_size_hint_align_set(cv, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, cv, 0, 0, 1, 4);
    evas_object_show(cv);

    bt = elm_button_add(fr);
    elm_button_label_set(bt, "Button 2");
    evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, bt, 0, 4, 1, 2);
    evas_object_show(bt);

    lb = elm_label_add(fr);
    elm_label_label_set(lb, "Title");
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, lb, 1, 0, 1, 1);
    evas_object_show(lb);


    lb = elm_label_add(fr);
    elm_label_label_set(lb, "Album");
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, lb, 1, 1, 1, 1);
    evas_object_show(lb);

    lb = elm_label_add(fr);
    elm_label_label_set(lb, "Artist");
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, lb, 1, 2, 1, 1);
    evas_object_show(lb);




    bt = elm_button_add(fr);
    elm_button_label_set(bt, "Button 6");
    evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, bt, 1, 3, 1, 1);
    evas_object_show(bt);


    return fr;
    //sd->o_mediacontrol = enna_mediacontrol_add(evas_object_evas_get(sd->o_edje),_enna_playlist);


}
