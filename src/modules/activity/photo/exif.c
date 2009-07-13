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
#include "buffer.h"
#include "exif.h"

#ifdef BUILD_LIBEXIF
#include <libexif/exif-data.h>
#endif

#define BUF_SIZE 2048

#ifdef BUILD_LIBEXIF
static void
photo_exif_content_foreach_func (ExifEntry *entry, void *data)
{
    char buf[BUF_SIZE] = { 0 };
    char buf_txtblk[BUF_SIZE] = { 0 };
    photo_exif_t *exif = data;

    if (!exif)
        return;

    exif_entry_get_value (entry, buf, BUF_SIZE);

    snprintf (buf_txtblk, BUF_SIZE, "<hilight>%s</hilight> : %s<br>",
              exif_tag_get_name (entry->tag),
              exif_entry_get_value (entry, buf, BUF_SIZE));

    buffer_appendf (exif->str, "%s", buf_txtblk);
}

static void
photo_exif_data_foreach_func (ExifContent *content, void *data)
{
  exif_content_foreach_entry (content, photo_exif_content_foreach_func, data);
}
#endif

void
photo_exif_parse_metadata (Evas *evas, Evas_Object *edje,
                           Evas_Object *preview, photo_exif_t *exif,
                           const char *filename)
{
#ifdef BUILD_LIBEXIF
  ExifData *d;
  Evas_Coord mw, mh;

  if (!exif || !filename)
      return;

  exif->o_exif = edje_object_add (evas);
  edje_object_file_set (exif->o_exif, enna_config_theme_get (), "exif/data");

  exif->o_scroll = elm_scroller_add (edje);
  edje_object_part_swallow (preview, "enna.swallow.exif", exif->o_scroll);

  exif->str = buffer_new ();

  d = exif_data_new_from_file (filename);
  exif_data_foreach_content (d, photo_exif_data_foreach_func, exif);
  exif_data_unref (d);

  if (exif->str->len == 0)
      buffer_append (exif->str, _("No EXIF information found."));

  edje_object_part_text_set (exif->o_exif, "enna.text.exif", exif->str->buf);
  edje_object_size_min_calc (exif->o_exif, &mw, &mh);
  evas_object_resize (exif->o_exif, mw, mh);
  evas_object_size_hint_min_set (exif->o_exif, mw, mh);
  elm_scroller_content_set (exif->o_scroll, exif->o_exif);
#endif
}

void
photo_exif_free (photo_exif_t *exif)
{
#ifdef BUILD_LIBEXIF
    ENNA_OBJECT_DEL (exif->o_exif);
    ENNA_OBJECT_DEL (exif->o_scroll);
    buffer_free (exif->str);
#endif
}

void
photo_exif_show (Evas_Object *obj)
{
#ifdef BUILD_LIBEXIF
    edje_object_signal_emit (obj, "show,exif", "enna");
#endif
}

void
photo_exif_hide (Evas_Object *obj)
{
#ifdef BUILD_LIBEXIF
    edje_object_signal_emit (obj, "hide,exif", "enna");
#endif
}
