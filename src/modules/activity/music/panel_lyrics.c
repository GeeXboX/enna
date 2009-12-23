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
#include "buffer.h"


typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *scroller;
    Evas_Object *text;
};

/* externally accessible functions */

Evas_Object *
enna_panel_lyrics_add (Evas *evas)
{
    Smart_Data *sd;
    Evas_Object *obj;
    Evas_Object *sc, *lb;

    sd = calloc(1, sizeof(Smart_Data));

    obj = elm_win_inwin_add(enna->win);
    elm_object_style_set(obj, "enna");
    elm_win_inwin_activate(obj);
    sc = elm_scroller_add (enna->layout);
    lb = elm_label_add(enna->layout);

    elm_scroller_content_set (sc, lb);
    elm_win_inwin_content_set(obj, sc);

    evas_object_show (lb);
    evas_object_show (sc);

    sd->scroller = sc;
    sd->text = lb;

    evas_object_data_set(obj, "sd", sd);

    return obj;
}

void
enna_panel_lyrics_set_text (Evas_Object *obj, Enna_Metadata *m)
{
    buffer_t *buf;
    char *lyrics, *title;
    char *b;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!m)
      return;

    title = enna_metadata_meta_get(m, "title", 1);
    if (!title)
        return;

    lyrics = enna_metadata_meta_get(m, "lyrics", 1);
    if (!lyrics)
    {
        free (title);
        elm_label_label_set(sd->text, _("No lyrics found ..."));
        return;
    }

    buf = buffer_new ();

    /* display song name */
    buffer_append  (buf, "<h4><hl><sd><b>");
    buffer_appendf (buf, "%s", title);
    buffer_append  (buf, "</b></sd></hl></h4><br>");
    free (title);

    /* display song associated lyrics */
    buffer_append  (buf, "<br/>");
    b = lyrics;
    while (*b)
    {
        if (*b == '\n')
            buffer_append (buf, "<br>");
        else
            buffer_appendf (buf, "%c", *b);
        (void) *b++;
    }
    free (lyrics);

    elm_label_label_set (sd->text, buf->buf);
}
