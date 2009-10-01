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
#include "buffer.h"


typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *scroller;
    Evas_Object *text;
};

/* local subsystem globals */

/* local subsystem globals */


/* externally accessible functions */

Evas_Object *
enna_panel_lyrics_add (Evas *evas)
{
    Smart_Data *sd;
    Evas_Object *obj;
    Evas_Object *sc, *lb;

    sd = calloc(1, sizeof(Smart_Data));

    obj = elm_win_inwin_add(enna->win);
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

/****************************************************************************/
/*                              Lyrics Panel                                */
/****************************************************************************/

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
