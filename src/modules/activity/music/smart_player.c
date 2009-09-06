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
#include "smart_player.h"
#include "mediacontrol.h"

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
    double value;
    edje_object_part_drag_value_get(obj, "enna.dragable.pos", &value, NULL);
    enna_mediaplayer_seek(value);
}

void enna_smart_player_position_set(Evas_Object *obj, double pos,
        double len, double percent)
{
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
}

static void metadata_set_text (Evas_Object *obj, Enna_Metadata *m,
                               const char *name, const char *edje, int max)
{
    char *str;

    API_ENTRY;

    str = enna_metadata_meta_get (m, name, max);
    edje_object_part_text_set(sd->o_edje, edje, str ? str : "");
    ENNA_FREE(str);
}

void enna_smart_player_metadata_set(Evas_Object *obj,
                                    Enna_Metadata *metadata)
{
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
        sd->o_cover = enna_image_add(evas_object_evas_get(sd->o_edje));
        enna_image_fill_inside_set(sd->o_cover, 0);
        enna_image_file_set(sd->o_cover, cover, NULL);
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
}

/* local subsystem globals */
static void _enna_mediaplayer_smart_reconfigure(Smart_Data * sd)
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
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "mediaplayer");
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;
    sd->h = 0;
    sd->o_mediacontrol=enna_mediacontrol_add(evas_object_evas_get(sd->o_edje),_enna_playlist);
    edje_object_part_swallow(sd->o_edje, "enna.swallow.mediacontrol", sd->o_mediacontrol);

    evas_object_smart_member_add(sd->o_mediacontrol, obj);
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
    edje_object_signal_callback_add(sd->o_edje, "drag", "enna.dragable.pos",
            _drag_bar_seek_cb, NULL);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    ENNA_OBJECT_DEL(sd->o_mediacontrol);
    ENNA_OBJECT_DEL(sd->o_cover);
    ENNA_OBJECT_DEL(sd->o_edje);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _enna_mediaplayer_smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _enna_mediaplayer_smart_reconfigure(sd);
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

static void _enna_mediaplayer_smart_init(void)
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
enna_smart_player_add(Evas * evas, Enna_Playlist *enna_playlist)
{
    _enna_playlist = enna_playlist;
    _enna_mediaplayer_smart_init();
    return evas_object_smart_add(evas, _smart);
}
