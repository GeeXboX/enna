
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
#include "buffer.h"
#include "infos.h"
#include "utils.h"

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
    case ENNA_FILE_FILE:
    {
        elm_icon_file_set(sd->ic, enna_config_theme_get(), file->icon);
        elm_layout_text_set(sd->page, "enna.text", file->label);
        break;
      }
    case ENNA_FILE_TRACK:
    {
        const char *artist;
        const char *track;
        const char *album;
        const char *cover;
        const char *cover_path;
        char tmp[4096];

        artist = enna_file_meta_get(file, "author");
        album = enna_file_meta_get(file, "album");
        track = enna_file_meta_get(file, "title");
        cover = enna_file_meta_get(file, "cover");
        if (cover && cover[0] != '/')
        {
            cover_path = eina_stringshare_printf("%s/covers/%s",
                                                     enna_util_data_home_get(), cover);
            printf("cover: %s\n", cover_path);
            elm_icon_file_set(sd->ic, cover_path, NULL);
            eina_stringshare_del(cover_path);
        }
        else if (cover)
        {
            printf("cover: %s\n", cover);
            elm_icon_file_set(sd->ic, cover, NULL);
            eina_stringshare_del(cover);
        }
        else
        {
            printf("cover: %s\n", file->icon);
            elm_icon_file_set(sd->ic, enna_config_theme_get(), file->icon);
        }
        snprintf(tmp, sizeof(tmp), "%s %s %s", track, album, artist);
        elm_layout_text_set(sd->page, "enna.text", file->label);
        break;
    }
    default:
        break;
    }

}

static void _set_text(Evas_Object *obj,
               const char *part,
               const char *text,
               const char *prefix,
               const char *alt)
{
    const char *tmp;

    if (!text || !text[0] || text[0] == ' ')
        elm_layout_text_set(obj, "enna.text.genre", alt);
    else
    {
        tmp = eina_stringshare_printf("%s%s", prefix, text);
        elm_layout_text_set(obj, part, tmp);
        eina_stringshare_del(text);
        eina_stringshare_del(tmp);
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
enna_infos_add (Evas_Object *parent)
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
enna_infos_file_set(Evas_Object *obj, Enna_File *file)
{
    Evas_Object *page = NULL;
    Smart_Data *sd = NULL;
    Evas_Object *ic = NULL;

    if (!obj || !file)
        return;

    sd = evas_object_data_get(obj, "sd");
    if (!sd)
        return;

    //if (sd->file)
    //    enna_file_free(sd->file);

    //sd->file = enna_file_dup(file);
    printf("Add infos file set for %s\n", file->uri);
    enna_file_meta_callback_add(file, _file_update_cb, sd);


    switch(file->type)
    {
    case ENNA_FILE_MENU:
    case ENNA_FILE_DIRECTORY:
    case ENNA_FILE_FILE:
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
    case ENNA_FILE_VOLUME:
    {
        const char *lb;
        Evas_Object *pg;
        const char *val;
        double v;

        page = elm_layout_add(sd->pager);
        elm_layout_file_set(page, enna_config_theme_get(), "panel/infos/volume");
        ic = elm_icon_add(page);
        elm_icon_file_set(ic, enna_config_theme_get(), file->icon);
        evas_object_show(ic);
        elm_layout_content_set(page, "enna.icon.swallow", ic);
        elm_layout_text_set(page, "enna.text.label", file->label);
        lb = eina_stringshare_printf("Size : %s Freespace : %s In use %s%%",
                                     enna_file_meta_get(file, ENNA_META_KEY_SIZE),
                                     enna_file_meta_get(file, ENNA_META_KEY_FREESPACE),
                                     enna_file_meta_get(file, ENNA_META_KEY_PERCENT_USED));
        elm_layout_text_set(page, "enna.text.description", lb);
        pg = elm_progressbar_add(sd->pager);
        val = enna_file_meta_get(file, ENNA_META_KEY_PERCENT_USED);
        v = atof(val) / 100.0;
        elm_progressbar_value_set(pg, v);
        printf("%s %3.3f\n", val, v);
        eina_stringshare_del(val);
        elm_progressbar_unit_format_set(pg, "%1.0f %%");
        elm_progressbar_horizontal_set(pg, EINA_TRUE);
        evas_object_show(pg);
        elm_layout_content_set(page, "enna.progress.swallow", pg);
        evas_object_show(page);
        elm_pager_content_pop(sd->pager);
        elm_pager_content_push(sd->pager, page);
        break;
      }
    case ENNA_FILE_TRACK:
    {
        const char *title;
        const char *track;
        const char *duration;
        const char *sduration;
        const char *codec;
        const char *year;
        const char *album;
        const char *artist;
        const char *cover;
        const char *cover_path;
        const char *tmp;

        artist = enna_file_meta_get(file, "author");
        album = enna_file_meta_get(file, "album");
        track = enna_file_meta_get(file, "title");

        page = elm_layout_add(sd->pager);
        elm_layout_file_set(page, enna_config_theme_get(), "panel/infos/track");
        ic = elm_icon_add(page);
        cover = enna_file_meta_get(file, "cover");
        if (cover && cover[0] != '/')
        {
            cover_path = eina_stringshare_printf("%s/covers/%s",
                                                     enna_util_data_home_get(), cover);
            elm_icon_file_set(ic, cover_path, NULL);
            eina_stringshare_del(cover_path);
        }
        else if (cover)
        {
            elm_icon_file_set(ic, cover, NULL);
            eina_stringshare_del(cover);
        }
        else
        {
            elm_icon_file_set(ic, enna_config_theme_get(), file->icon);
        }
        evas_object_show(ic);
        elm_layout_content_set(page, "enna.swallow.icon", ic);

        year = enna_file_meta_get(file, "year");
        if (!year || !year[0] || year[0] == ' ')
            elm_layout_text_set(page, "enna.text.year", "");
        else
        {
            tmp = eina_stringshare_printf("Year: %s", year);
            elm_layout_text_set(page, "enna.text.year", tmp);
            eina_stringshare_del(year);
            eina_stringshare_del(tmp);
        }

        codec = enna_file_meta_get(file, "audio_codec");
        if (!codec || !codec[0] || codec[0] == ' ')
            elm_layout_text_set(page, "enna.text.codec", "");
        else
        {
            tmp = eina_stringshare_printf("Type: %s", codec);
            elm_layout_text_set(page, "enna.text.codec", tmp);
            eina_stringshare_del(codec);
            eina_stringshare_del(tmp);
        }

        duration = enna_file_meta_get(file, "duration");
        if (!duration)
            duration = enna_file_meta_get(file, "length");
        if (!duration)
             elm_layout_text_set(page, "enna.text.length", "");
        sduration = enna_util_duration_to_string(duration);
        if (!sduration)
        {
            eina_stringshare_del(duration);
            elm_layout_text_set(page, "enna.text.length", "");
        }
        tmp = eina_stringshare_printf("Length: %s", sduration);
        elm_layout_text_set(page, "enna.text.length", tmp);
        eina_stringshare_del(sduration);
        eina_stringshare_del(tmp);

        title = enna_file_meta_get(file, "title");
        if (!title || !title[0] || title[0] == ' ')
            elm_layout_text_set(page, "enna.text.title", file->label);
        else
        {
            elm_layout_text_set(page, "enna.text.title", title);
            eina_stringshare_del(title);
        }

        track = enna_file_meta_get(file, "track");
        if (!track)
            elm_layout_text_set(page, "enna.text.trackno", "");
        else
        {
            tmp = eina_stringshare_printf("%02d.", atoi(track));
            elm_layout_text_set(page, "enna.text.trackno", tmp);
            eina_stringshare_del(track);
            eina_stringshare_del(tmp);
        }

        album = enna_file_meta_get(file, "album");
        if (!album || !album[0] || album[0] == ' ')
            elm_layout_text_set(page, "enna.text.title", "");
        else
        {
            tmp = eina_stringshare_printf("Album: %s", album);
            elm_layout_text_set(page, "enna.text.album", tmp);
            eina_stringshare_del(title);
            eina_stringshare_del(tmp);
        }

        artist = enna_file_meta_get(file, "author");
        if (!artist || !artist[0] || artist[0] == ' ')
            elm_layout_text_set(page, "enna.text.artist", "");
        else
        {
            tmp = eina_stringshare_printf("Artist: %s", artist);
            elm_layout_text_set(page, "enna.text.artist", tmp);
            eina_stringshare_del(title);
            eina_stringshare_del(tmp);
        }

        evas_object_show(page);
        elm_pager_content_pop(sd->pager);
        elm_pager_content_push(sd->pager, page);
        break;
    }
    case ENNA_FILE_FILM:
    {
        const char *title;
        const char *genre;
        const char *duration;
        const char *sduration = NULL;
        const char *year;
        const char *album;
        const char *artist;
        const char *cover;
        const char *cover_path;

        artist = enna_file_meta_get(file, "author");
        album = enna_file_meta_get(file, "album");
        title = enna_file_meta_get(file, "title");

        page = elm_layout_add(sd->pager);
        elm_layout_file_set(page, enna_config_theme_get(), "panel/infos/film");
        ic = elm_icon_add(page);

        /* Cover Icon */
        cover = enna_file_meta_get(file, "cover");
        if (cover && cover[0] != '/')
        {
            cover_path = eina_stringshare_printf("%s/covers/%s",
                                                     enna_util_data_home_get(), cover);
            elm_icon_file_set(ic, cover_path, NULL);
            eina_stringshare_del(cover_path);
        }
        else if (cover)
        {
            elm_icon_file_set(ic, cover, NULL);
            eina_stringshare_del(cover);
        }
        else
        {
            elm_icon_file_set(ic, enna_config_theme_get(), file->icon);
        }
        evas_object_show(ic);
        elm_layout_content_set(page, "enna.swallow.cover", ic);

        /* Title */
        title = enna_file_meta_get(file, "title");
        _set_text(page, "enna.text.title", title, "", file->label);


        /* Genre */
        genre = enna_file_meta_get(file, "genre");
        _set_text(page, "enna.text.genre", genre, _("Genre: "), "");

        /* Year */
        year = enna_file_meta_get(file, "year");
        _set_text(page, "enna.text.year", year, _("Year: "), "");

        /* Length */
        duration = enna_file_meta_get(file, "duration");
        if (!duration)
            duration = enna_file_meta_get(file, "length");
        if (duration)
            sduration = enna_util_duration_to_string(duration);
        _set_text(page, "enna.text.length", sduration, _("Duration: "), "");

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

