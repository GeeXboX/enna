
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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "logs.h"
#include "image.h"
#include "buffer.h"
#include "music_infos.h"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *pager;
    Enna_File *file;
    Evas_Object *page;
    Evas_Object *ic;
};

static void
_update(Smart_Data *sd, Enna_File *file)
{
    switch(file->type)
    {
    case ENNA_FILE_MENU:
    case ENNA_FILE_DIRECTORY:
    {
        elm_icon_file_set(sd->ic, enna_config_theme_get(), file->icon);
        elm_layout_text_set(sd->page, "enna.text", file->label);
        break;
      }
    case ENNA_FILE_TRACK:
    case ENNA_FILE_FILE:
    {
        const char *artist;
        const char *track;
        const char *album;
        char tmp[4096];

        printf("Enna_File Track\n");

        artist = enna_file_meta_get(file, "author");
        album = enna_file_meta_get(file, "album");
        track = enna_file_meta_get(file, "title");
        elm_icon_file_set(sd->ic, enna_file_meta_get(file, "cover"), NULL);
        snprintf(tmp, sizeof(tmp), "%s %s %s", track, album, artist);
        elm_layout_text_set(sd->page, "enna.text", file->label);
        break;
    }
    default:
        break;
    }

}

static void
_file_update_cb(void *data, Enna_File *file)
{
    Smart_Data *sd = data;

    DBG("File %s has meta update\n", file->uri);
    _update(sd, file);
}

/* externally accessible functions */

Evas_Object *
enna_music_infos_add (Evas_Object *parent)
{
    Smart_Data *sd;
 
    sd = calloc(1, sizeof(Smart_Data));

    sd->pager = elm_pager_add(parent);
    elm_object_style_set(sd->pager, "slide_invisible");
    evas_object_size_hint_weight_set(sd->pager, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show (sd->pager);
    evas_object_data_set(sd->pager, "sd", sd);

    return sd->pager;
}

void
enna_music_infos_file_set(Evas_Object *obj, Enna_File *file)
{
    Evas_Object *page;
    Smart_Data *sd;
    Evas_Object *ic;
    if (!obj || !file)
        return;

    sd = evas_object_data_get(obj, "sd");
    if (!sd)
        return;

    //if (sd->file)
    //    enna_file_free(sd->file);

    //sd->file = enna_file_dup(file);
    enna_file_meta_callback_add(file, _file_update_cb, sd);


    switch(file->type)
    {
    case ENNA_FILE_MENU:
    case ENNA_FILE_DIRECTORY:
    {
        page = elm_layout_add(sd->pager);
        elm_layout_file_set(page, enna_config_theme_get(), "panel/infos/menu");
        ic = elm_icon_add(page);
        elm_icon_file_set(ic, enna_config_theme_get(), file->icon);
        evas_object_show(ic);
        elm_layout_content_set(page, "enna.icon.swallow", ic);
        elm_layout_text_set(page, "enna.text", file->label);
        evas_object_show(page);
        elm_pager_content_pop(sd->pager);
        elm_pager_content_push(sd->pager, page);
        break;
      }
    case ENNA_FILE_TRACK:
    case ENNA_FILE_FILE:
    {
        const char *artist;
        const char *track;
        const char *album;
        char tmp[4096];

        printf("Enna_File Track\n");

        artist = enna_file_meta_get(file, "author");
        album = enna_file_meta_get(file, "album");
        track = enna_file_meta_get(file, "title");

        page = elm_layout_add(sd->pager);
        elm_layout_file_set(page, enna_config_theme_get(), "panel/infos/menu");
        ic = elm_icon_add(page);
        elm_icon_file_set(ic, enna_file_meta_get(file, "cover"), NULL);
        evas_object_show(ic);
        snprintf(tmp, sizeof(tmp), "%s %s %s", track, album, artist);
        elm_layout_content_set(page, "enna.icon.swallow", ic);
        elm_layout_text_set(page, "enna.text", file->label);
        evas_object_show(page);
        elm_pager_content_pop(sd->pager);
        elm_pager_content_push(sd->pager, page);
        break;
    }
    default:
        break;
        
    }
    sd->ic = ic;
    sd->page = page;
}

void
enna_music_infos_set_text (Evas_Object *obj, Enna_Metadata *m)
{
#if 0
    Enna_Buffer *buf;
    char *lyrics, *title;
    char *b;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!m || !sd)
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

    buf = enna_buffer_new ();

    /* display song name */
    enna_buffer_append  (buf, "<h4><hl><sd><b>");
    enna_buffer_appendf (buf, "%s", title);
    enna_buffer_append  (buf, "</b></sd></hl></h4><br>");
    free (title);

    /* display song associated lyrics */
    enna_buffer_append  (buf, "<br/>");
    b = lyrics;
    while (*b)
    {
        if (*b == '\n')
            enna_buffer_append (buf, "<br>");
        else
            enna_buffer_appendf (buf, "%c", *b);
        (void) *b++;
    }
    free (lyrics);

    elm_label_label_set (sd->text, buf->buf);
#endif
}
