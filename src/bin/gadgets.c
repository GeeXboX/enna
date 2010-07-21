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

#include "gadgets.h"

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "box.h"

#define DEFAULT_NB_ROWS 4
#define DEFAULT_NB_COLUMNS 4
#define DEFAULT_PADDING 4

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *o_layout;
    Evas_Object *o_scroll;
    Evas_Object *o_box;
    Evas_Object *o_table;
    int nb_rows;
    int nb_columns;
    int row;
    int column;
    Eina_List *items;
    Eina_List *pad_top;
    Eina_List *pad_bottom;
    Eina_List *pad_left;
    Eina_List *pad_right;
};

static int _gadgets_init_count = -1;
static Smart_Data *sd;
static Eina_List *_gadgets;

void
enna_gadgets_register(Enna_Gadget *gad)
{

    _gadgets = eina_list_append(_gadgets, gad);

}

static void
_add_gadgets(Evas_Object *o_gad)
{
    Evas_Object *pad;

    if (sd->row == 0 && sd->column == 0)
    {
        sd->o_table = elm_table_add(sd->o_box);
        evas_object_size_hint_weight_set(sd->o_table, 0.0, 0.0);
        evas_object_size_hint_align_set(sd->o_table, 0.5, 0.5);

        pad = evas_object_rectangle_add(enna->evas);
        evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
//        elm_table_pack(sd->o_table, pad, 0, 0, sd->nb_columns + 2, 1);
        elm_table_pack(sd->o_table, pad, 0, 0, sd->nb_columns, sd->nb_rows);
        sd->pad_top = eina_list_append(sd->pad_top, pad);

/*         pad = evas_object_rectangle_add(enna->evas); */
/*         evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL); */
/*         elm_table_pack(sd->o_table, pad, 0, sd->nb_columns + 1, sd->nb_rows + 2, 1); */
/*         sd->pad_bottom = eina_list_append(sd->pad_bottom, pad); */


/*         pad = evas_object_rectangle_add(enna->evas); */
/*         evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL); */
/*         elm_table_pack(sd->o_table, pad, 0, 0, 1, sd->nb_rows + 2); */
/*         sd->pad_left = eina_list_append(sd->pad_left, pad); */

/*         pad = evas_object_rectangle_add(enna->evas); */
/*         evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL); */
/*         elm_table_pack(sd->o_table, pad, sd->nb_columns + 1, 0, 1, sd->nb_rows + 2); */
/*         sd->pad_right = eina_list_append(sd->pad_right, pad); */

        elm_table_homogenous_set(sd->o_table, 1);
        elm_box_pack_end(sd->o_box, sd->o_table);
        evas_object_show(sd->o_table);
    }

    //evas_object_size_hint_weight_set(o_gad, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    //evas_object_size_hint_align_set(o_gad, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(sd->o_table, o_gad, sd->column, sd->row, 1, 1);
    evas_object_show(o_gad);

    sd->row++;
    if (!(sd->row % sd->nb_rows))
    {
        sd->row = 0;
        sd->column++;
        if (!(sd->column % sd->nb_columns))
            sd->column = 0;
    }


}

static void
_layout_resize(void *data, Evas *e, Evas_Object *o, void *event_info)
{
    Evas_Object *pad, *o_gad;
    Eina_List *l;
    Evas_Coord w, h;
    Evas_Coord ow, oh;
 
    elm_scroller_region_get(sd->o_scroll, NULL, NULL, &w, &h);
    ow = w / sd->nb_columns;
    oh = h / sd->nb_rows;

    EINA_LIST_FOREACH(sd->pad_top, l, pad)
    {
        evas_object_size_hint_min_set(pad, w, h);
        evas_object_size_hint_weight_set(pad, 0.0, 0.0);
    }

  /*   EINA_LIST_FOREACH(sd->pad_bottom, l, pad) */
/*     { */
/*         evas_object_size_hint_min_set(pad, ow, 4); */
/*         evas_object_size_hint_weight_set(pad, 0.0, 0.0); */
/*     } */

/*     EINA_LIST_FOREACH(sd->pad_right, l, pad) */
/*     { */
/*         evas_object_size_hint_min_set(pad, 4, oh); */
/*         evas_object_size_hint_weight_set(pad, 0.0, 0.0); */
/*     } */

/*     EINA_LIST_FOREACH(sd->pad_left, l, pad) */
/*     { */
/*         evas_object_size_hint_min_set(pad, 4, oh); */
/*         evas_object_size_hint_weight_set(pad, 0.0, 0.0); */
/*     } */

    EINA_LIST_FOREACH(sd->items, l, o_gad)
    {
        //evas_object_size_hint_min_set(o_gad, ow, oh);
        //evas_object_size_hint_weight_set(o_gad, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        //evas_object_size_hint_align_set(o_gad, EVAS_HINT_FILL, EVAS_HINT_FILL);
    }

}
void
enna_gadgets_show()
{
    Evas_Object *o;
    Evas_Object *o_edje;
    Eina_List *l;
    Enna_Gadget *gad;
    const char *s;

    if (!sd)
        return;

    ENNA_OBJECT_DEL(sd->o_scroll);

    sd->o_layout = elm_layout_add(enna->win);
    elm_layout_file_set(sd->o_layout, enna_config_theme_get(), "enna/gadgets");
    o_edje = elm_layout_edje_get(sd->o_layout);
    evas_object_show(sd->o_layout);

    s = edje_object_data_get(o_edje, "rows");
    if (s)
        sd->nb_rows = atoi(s);
    else
        sd->nb_rows = DEFAULT_NB_ROWS;

    s = edje_object_data_get(o_edje, "columns");
    if (s)
        sd->nb_columns = atoi(s);
    else
        sd->nb_columns = DEFAULT_NB_COLUMNS;


    sd->o_scroll = elm_scroller_add(enna->win);
    elm_object_style_set(sd->o_scroll, "enna");

    elm_scroller_policy_set(sd->o_scroll, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
    evas_object_size_hint_weight_set(sd->o_scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    elm_scroller_page_relative_set(sd->o_scroll, 1.0, 1.0);
    evas_object_show(sd->o_scroll);

    sd->o_box = elm_box_add(sd->o_layout);
    elm_box_homogenous_set(sd->o_box, 1);
    elm_box_horizontal_set(sd->o_box, 1);

    elm_scroller_content_set(sd->o_scroll, sd->o_box);

    elm_layout_content_set(sd->o_layout,
                           "enna.swallow.content", sd->o_scroll);

    elm_layout_content_set(enna->layout,
                           "enna.gadgets.swallow", sd->o_layout);

    evas_object_event_callback_add
        (sd->o_layout, EVAS_CALLBACK_RESIZE, _layout_resize, sd);


    EINA_LIST_FOREACH(_gadgets, l, gad)
    {
        if (gad->add)
        {
            o = gad->add();
            _add_gadgets(o);
        }
    }
}

void
enna_gadgets_hide()
{
    Eina_List *l;
    Enna_Gadget *gad;

    EINA_LIST_FOREACH(_gadgets, l, gad)
    {
        if (gad->del)
            gad->del();
    }
    ENNA_OBJECT_DEL(sd->o_scroll);
}

int
enna_gadgets_init()
{
    /* Prevent multiple loads */
    if (_gadgets_init_count > 0)
        return ++_gadgets_init_count;

    sd = calloc(1, sizeof(Smart_Data));

    _gadgets_init_count = 1;
    return 1;
}

int
enna_gadgets_shutdown()
{
    _gadgets_init_count--;
    if (_gadgets_init_count == 0)
    {
        ENNA_OBJECT_DEL(sd->o_scroll);
        ENNA_FREE(sd);
    }

    return _gadgets_init_count;
}

