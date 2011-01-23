
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

static const char *
_update_label(Evas_Object *parent __UNUSED__, Enna_File *file, const char *key)
{
    const char *meta;
    meta = enna_file_meta_get(file, key);

    return meta;
}

static Evas_Object *
_update_icon(Evas_Object *parent, Enna_File *file, const char *key)
{
    Evas_Object *ic = NULL;
    const char *path;

    ic = elm_icon_add(parent);
    path = enna_file_meta_get(file, key);

    if (path && path[0] != '/')
    {
        elm_icon_file_set(ic, enna_config_theme_get(), path);
        eina_stringshare_del(path);
    }
    else if (path)
    {
        elm_icon_file_set(ic, path, NULL);
        eina_stringshare_del(path);

    }
    else if (!strcmp(key, "cover"))
    {
        elm_icon_file_set(ic, enna_config_theme_get(), file->icon);
    }

    if (ic)
        evas_object_show(ic);

    return ic;
}


static void
_update(Smart_Data *sd, Enna_File *file)
{
    const char *ly_data;
    const char *label_name;
    const char *icon_name;
    Eina_List *labels;
    Eina_List *icons;
    Eina_List *l;

    ly_data = elm_layout_data_get(sd->page, "labels");
    labels = enna_util_stringlist_get(ly_data);
    EINA_LIST_FOREACH(labels, l, label_name)
    {
        const char *str;
        const char *style;
        str = _update_label(sd->page, file, label_name);

        style = eina_stringshare_printf("enna.text.%s", label_name);
        if (str)
            elm_layout_text_set(sd->page, style, str);
        else
            elm_layout_text_set(sd->page, style, "");
        eina_stringshare_del(style);
        eina_stringshare_del(str);
    }
    enna_util_stringlist_free(labels);

    ly_data = elm_layout_data_get(sd->page, "icons");
    icons = enna_util_stringlist_get(ly_data);
    EINA_LIST_FOREACH(icons, l, icon_name)
    {
        Evas_Object *ic;
        const char *style;

        ic = _update_icon(sd->page, file, icon_name);
        style = eina_stringshare_printf("enna.swallow.%s", icon_name);
        elm_layout_content_set(sd->page, style, ic);
        eina_stringshare_del(style);
    }
    enna_util_stringlist_free(icons);

    switch(file->type)
    {
    case ENNA_FILE_VOLUME:
    {
        const char *lb;
        Evas_Object *pg;
        const char *val;
        double v;

        lb = eina_stringshare_printf(_("Size : %s Freespace : %s In use %s%%"),
                                     enna_file_meta_get(file, ENNA_META_KEY_SIZE),
                                     enna_file_meta_get(file, ENNA_META_KEY_FREESPACE),
                                     enna_file_meta_get(file, ENNA_META_KEY_PERCENT_USED));
        elm_layout_text_set(sd->page, "enna.text.description", lb);

        pg = elm_progressbar_add(sd->page);
        val = enna_file_meta_get(file, ENNA_META_KEY_PERCENT_USED);
        v = enna_util_atof(val) / 100.0;
        elm_progressbar_value_set(pg, v);
        eina_stringshare_del(val);
        elm_progressbar_unit_format_set(pg, "%1.0f %%");
        elm_progressbar_horizontal_set(pg, EINA_TRUE);
        evas_object_show(pg);
        elm_layout_content_set(sd->page, "enna.progress.swallow", pg);
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

static const struct {
    Enna_File_Type type;
    const char *style;
} infos_style_mapping[] = {
    { ENNA_FILE_MENU,       "panel/infos/menu"      },
    { ENNA_FILE_DIRECTORY,  "panel/infos/directory" },
    { ENNA_FILE_VOLUME,     "panel/infos/volume"    },
    { ENNA_FILE_FILE,       "panel/infos/file"      },
    { ENNA_FILE_ARTIST,     "panel/infos/artist"    },
    { ENNA_FILE_ALBUM,      "panel/infos/album"     },
    { ENNA_FILE_TRACK,      "panel/infos/track"     },
    { ENNA_FILE_FILM,       "panel/infos/film"      }
};

void
enna_infos_file_set(Evas_Object *obj, Enna_File *file)
{
    Smart_Data *sd = NULL;
    int i;

    if (!obj || !file)
        return;

    sd = evas_object_data_get(obj, "sd");
    if (!sd)
        return;

    if (!sd->file || file->type != sd->file->type)
    {
        /* Different info layout, create a new info page */
        sd->page = elm_layout_add(sd->pager);
        for (i = 0; infos_style_mapping[i].style; i++)
        {
            if (file->type == infos_style_mapping[i].type)
            {
                elm_layout_file_set(sd->page, enna_config_theme_get(),
                                    infos_style_mapping[i].style);
                break;
            }
        }
        elm_pager_content_pop(sd->pager);
        elm_pager_content_push(sd->pager, sd->page);
    }

    if (sd->file)
    {
        enna_file_meta_callback_del(sd->file, _file_update_cb);
        enna_file_free(sd->file);
    }
    sd->file = enna_file_ref(file);

    _update(sd, file);

    enna_file_meta_callback_add(file, _file_update_cb, sd);
}

