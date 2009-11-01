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
    Evas_Object *cv;
    Evas_Object *ctl;
    Evas_Object *sl;
    Evas_Object *current_time;
    Evas_Object *total_time;
    Evas_Object *artist;
    Evas_Object *album;
    Evas_Object *title;
};

/* local subsystem globals */
static Enna_Playlist *_enna_playlist;

static void _seek_cb(void *data, Evas_Object *obj, void *event_info)
{
    double value;

    value = elm_slider_value_get(data);
    enna_mediaplayer_seek(value / 100.0);
}

void enna_smart_player_position_set(Evas_Object *obj, double pos,
        double len, double percent)
{
    Smart_Data *sd;

    long ph, pm, ps, lh, lm, ls;
    char buf[256];
    char buf2[256];

    sd = evas_object_data_get(obj, "sd");
    if (!sd) return;

    lh = len / 3600000;
    lm = len / 60 - (lh * 60);
    ls = len - (lm * 60);
    ph = pos / 3600;
    pm = pos / 60 - (ph * 60);
    ps = pos - (pm * 60);
    snprintf(buf, sizeof(buf), "%02li:%02li", pm, ps);
    snprintf(buf2, sizeof(buf2), "%02li:%02li", lm, ls);

    elm_label_label_set(sd->total_time, buf2);
    elm_label_label_set(sd->current_time, buf);
    elm_slider_value_set(sd->sl, pos/len * 100.0);

}

static void metadata_set_text(Evas_Object *obj, Enna_Metadata *m, const char *name)
{
    char *str;
    str = enna_metadata_meta_get(m, name, 1);
    elm_label_label_set(obj, enna_util_str_chomp(str));
    ENNA_FREE(str);
}

void enna_smart_player_metadata_set(Evas_Object *obj,
                                    Enna_Metadata *metadata)
{
    Smart_Data *sd;
    char *cover;

    sd = evas_object_data_get(obj, "sd");

    if (!metadata || !sd)
        return;

    metadata_set_text (sd->title, metadata, "title");
    metadata_set_text (sd->album, metadata, "album");
    metadata_set_text (sd->artist, metadata, "author");

    cover = enna_metadata_meta_get (metadata, "cover", 1);
    if (cover)
    {
        char cv[1024] = { 0 };

        if (*cover == '/')
            snprintf(cv, sizeof (cv), "%s", cover);
        else
            snprintf(cv, sizeof (cv), "%s/.enna/covers/%s",
                     enna_util_user_home_get (), cover);

        elm_image_file_set(sd->cv, cv, NULL);
    }
    else
    {
        elm_image_file_set(sd->cv, NULL, NULL);
    }
    evas_object_show(sd->cv);
}

/* externally accessible functions */
Evas_Object *
enna_smart_player_add(Evas * evas, Enna_Playlist *enna_playlist)
{

    Evas_Object *fr;
    Evas_Object *tb;
    Evas_Object *bt;
    Evas_Object *cv;
    Evas_Object *lb;
    Evas_Object *sl;
    Evas_Object *bx;
    Smart_Data *sd;

    sd = malloc(sizeof(Smart_Data));

    fr = elm_frame_add(enna->layout);
    tb = elm_table_add(enna->layout);
    evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_frame_content_set(fr, tb);
    evas_object_show(fr);
    evas_object_show(tb);

    cv = elm_image_add(fr);
    evas_object_size_hint_weight_set(cv, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(cv, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, cv, 0, 0, 1, 4);
    evas_object_show(cv);
    sd->cv = cv;

    lb = elm_label_add(fr);
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, 0.5, 0.5);
    elm_label_label_set(lb, "Title");
    elm_table_pack(tb, lb, 1, 0, 1, 1);
    evas_object_show(lb);
    sd->title = lb;


    lb = elm_label_add(fr);
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, 0.5, 0.5);
    elm_label_label_set(lb, "Album");
    elm_table_pack(tb, lb, 1, 1, 1, 1);
    evas_object_show(lb);
    sd->album = lb;

    lb = elm_label_add(fr);
    evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lb, 0.5, 0.5);
    elm_label_label_set(lb, "Artist");
    elm_table_pack(tb, lb, 1, 2, 1, 1);
    evas_object_show(lb);
    sd->artist = lb;


    bt = enna_mediacontrol_add(evas_object_evas_get(fr), enna_playlist);
    evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(tb, bt, 1, 3, 1, 1);
    evas_object_show(bt);

    bx = elm_box_add(fr);
    elm_box_horizontal_set(bx, 1);
    evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(bx);
    elm_table_pack(tb, bx, 1, 4, 1, 1);

    lb = elm_label_add(fr);
    elm_box_pack_end(bx, lb);
    evas_object_show(lb);
    sd->current_time = lb;

    sl = elm_slider_add(fr);
    elm_slider_span_size_set(sl, 80);
    evas_object_size_hint_align_set(sl, EVAS_HINT_FILL, 0.5);
    elm_slider_min_max_set(sl, 0.0, 100.0);
    elm_slider_indicator_format_set(sl, "%1.0f %%");
    evas_object_size_hint_weight_set(sl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_pack_end(bx, sl);
    evas_object_show(sl);
    evas_object_smart_callback_add(sl, "delay,changed", _seek_cb, sl);
    sd->sl = sl;

    lb = elm_label_add(fr);
    elm_box_pack_end(bx, lb);
    evas_object_show(lb);
    sd->total_time = lb;

    /* FIXME : WTF ? why we have a ref to the playlist there ? */
    _enna_playlist = enna_playlist;

    evas_object_data_set(fr, "sd", sd);

    return fr;
 }
