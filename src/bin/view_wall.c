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

#include <string.h>

#include <Ecore.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "view_wall.h"
#include "image.h"
#include "logs.h"
#include "vfs.h"
#include "input.h"
#include "metadata.h"
#include "kbdnav.h"

#define SMART_NAME "enna_wall"

typedef struct _Smart_Data Smart_Data;
typedef struct _Picture_Item Picture_Item;

struct _Picture_Item
{
    Enna_Vfs_File *file;
    void (*func_activated) (void *data);
    void *data;
    Elm_Gengrid_Item *item;
    Smart_Data *sd;

};

struct _Smart_Data
{
    Evas_Object *o_grid;
    Eina_List *items;
  Enna_Kbdnav *nav;
};

char *
_grid_item_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Picture_Item *pi = data;

    if (!pi || !pi->file) return NULL;

    return pi->file->label ? strdup(pi->file->label) : NULL;
}

Evas_Object *
_grid_item_icon_get(const void *data, Evas_Object *obj, const char *part)
{
	 Picture_Item *pi = (Picture_Item*) data;

    if (!pi)
		return NULL;

	if (!strcmp(part, "elm.swallow.icon"))
	{
		Evas_Object *ic;

		if (!pi->file)
			return NULL;

		if (pi->file->is_directory || pi->file->is_menu)
		{
			ic = elm_icon_add(obj);
			if (pi->file->icon && pi->file->icon[0] == '/')
				elm_icon_file_set(ic, pi->file->icon, NULL);
			else if (pi->file->icon)
				elm_icon_file_set(ic, enna_config_theme_get(), pi->file->icon);
			else
				return NULL;

                        evas_object_size_hint_max_set(ic, 92, 92);
                        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
			evas_object_show(ic);
			return ic;
		}
		else
		{
                    ic = elm_thumb_add(obj);
                    printf("file set : %s\n", pi->file->mrl + 7);

                    elm_object_style_set(ic, "enna");

                    elm_thumb_file_set(ic, pi->file->mrl + 7, NULL);
                    evas_object_show(ic);

                    return ic;
		}
	}

	return NULL;
}

Eina_Bool
_grid_item_state_get(const void *data, Evas_Object *obj, const char *part)
{
	return EINA_FALSE;
}

void
_grid_item_del(const void *data, Evas_Object *obj)
{

}

static void
_item_remove(Evas_Object *obj, Picture_Item *item)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd || !item) return;

    elm_gengrid_item_del(item->item);
    sd->items = eina_list_remove(sd->items, item);
    enna_kbdnav_item_del(sd->nav, item);
    ENNA_FREE(item);

    return;
}

static Elm_Gengrid_Item_Class gic = {
    "enna",
    {
        _grid_item_label_get,
        _grid_item_icon_get,
        _grid_item_state_get,
        _grid_item_del
    }
};

static void
_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    Picture_Item *pi;
    if (!sd)
        return;
    printf("genlist clear\n");
    elm_gengrid_clear(sd->o_grid);

    EINA_LIST_FREE(sd->items, pi)
        free(pi);

    enna_kbdnav_del(sd->nav);
    free(sd);
}



static void
_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Coord h;
    Smart_Data *sd = data;

    evas_object_geometry_get(sd->o_grid, NULL, NULL, NULL, &h);
    elm_gengrid_item_size_set(sd->o_grid, h / 4, h / 4);
}

static void
_item_activate(Elm_Gengrid_Item *item)
{
    Picture_Item *li;

    li = (Picture_Item*)elm_gengrid_item_data_get(item);
    if (li->func_activated)
            li->func_activated(li->data);
}

static void
_item_selected(void *data, Evas_Object *obj, void *event_info)
{
    Picture_Item *li = data;

    evas_object_smart_callback_call(obj, "hilight", li->data);
}

static void
_item_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Elm_Gengrid_Item *item = data;
    Evas_Event_Mouse_Up *ev = event_info;

    /* Don't activate when user is scrolling list */
    if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
        return;
    _item_activate(item);
}

static void
_item_realized_cb(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Gengrid_Item *item = event_info;
   Evas_Object *o_item;

   o_item = (Evas_Object*)elm_gengrid_item_object_get(item);
   evas_object_event_callback_add(o_item, EVAS_CALLBACK_MOUSE_UP,_item_click_cb, item);
}

/* externally accessible functions */

Evas_Object *
enna_wall_add(Evas_Object * parent)
{
    Smart_Data *sd;
    Ethumb_Client *client;

    elm_need_ethumb();

    sd = calloc(1, sizeof(Smart_Data));

    sd->o_grid = elm_gengrid_add(parent);
    elm_gengrid_horizontal_set(sd->o_grid, EINA_TRUE);
    elm_gengrid_multi_select_set(sd->o_grid, EINA_FALSE);
    elm_gengrid_align_set(sd->o_grid, 0, 0.5);
    elm_gengrid_bounce_set(sd->o_grid, EINA_TRUE, EINA_FALSE);

    evas_object_data_set(sd->o_grid, "sd", sd);
    elm_object_style_set(sd->o_grid, "enna");
    evas_object_smart_callback_add(sd->o_grid, "realized", _item_realized_cb, sd);
    evas_object_event_callback_add(sd->o_grid, EVAS_CALLBACK_DEL, _del_cb, sd);
    evas_object_event_callback_add(sd->o_grid, EVAS_CALLBACK_RESIZE, _resize_cb, sd);
    client = elm_thumb_ethumb_client_get();
    ethumb_client_aspect_set(client, ETHUMB_THUMB_CROP);
    ethumb_client_crop_align_set(client, 0.5, 0.5);

    sd->nav = enna_kbdnav_add();

    return sd->o_grid;
}

void
enna_wall_clear(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    Picture_Item *item;

    elm_gengrid_clear(obj);
    EINA_LIST_FREE(sd->items, item)
        _item_remove(obj, item);
}


static const Evas_Object *
_kbdnav_object_get(void *item_data, void *user_data)
{
  Picture_Item *pi = item_data;

  if (!pi)
    return NULL;

  return elm_gengrid_item_object_get(pi->item);
}

static void
_kbdnav_select_set(void *item_data, void *user_data)
{
  Picture_Item *pi = item_data;

  if (!pi)
    return;
  elm_gengrid_item_selected_set(pi->item, EINA_TRUE);
}

static void
_kbdnav_activate_set(void *item_data, void *user_data)
{
  Picture_Item *pi = item_data;

  if (!pi)
    return;
  _item_activate(pi->item);
}

static Enna_Kbdnav_Class ekc = {
  _kbdnav_object_get,
  _kbdnav_select_set,
  _kbdnav_activate_set
};

void
enna_wall_file_append(Evas_Object *obj, Enna_Vfs_File *file,
                      void (*func_activated) (void *data), void *data )
{
    Smart_Data *sd;
    Picture_Item *pi;

    sd = evas_object_data_get(obj, "sd");

    pi = ENNA_NEW(Picture_Item, 1);

    pi->func_activated = func_activated;
    pi->data = data;
    pi->file = file;
    pi->sd = sd;

    pi->item = elm_gengrid_item_append (obj, &gic, pi, _item_selected, pi);
    sd->items = eina_list_append(sd->items, pi);
    enna_kbdnav_item_add(sd->nav, pi, &ekc, NULL);
}

void
enna_wall_file_remove(Evas_Object *obj, Enna_File *file)
{
    Eina_List *l;
    Picture_Item *pi;
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, pi)
    {
        if (file == pi->file)
        {
            _item_remove(obj, pi);
            break;
        }
    }
}

Eina_Bool
enna_wall_input_feed(Evas_Object *obj, enna_input ev)
{ 
  Smart_Data *sd;

 sd = evas_object_data_get(obj, "sd");
 switch (ev)
    {
    case ENNA_INPUT_LEFT:
        enna_kbdnav_left(sd->nav);
        return ENNA_EVENT_BLOCK;
        break;
    case ENNA_INPUT_RIGHT:
       enna_kbdnav_right(sd->nav);
        return ENNA_EVENT_BLOCK;
        break;
    case ENNA_INPUT_UP:
       enna_kbdnav_up(sd->nav);
        return ENNA_EVENT_BLOCK;
        break;
    case ENNA_INPUT_DOWN:
       enna_kbdnav_down(sd->nav);
        return ENNA_EVENT_BLOCK;
        break;
    case ENNA_INPUT_OK:
        enna_kbdnav_activate(sd->nav);
        return ENNA_EVENT_BLOCK;
        break;
    default:
        break;
    }
    return ENNA_EVENT_CONTINUE;
}

void
enna_wall_select_nth(Evas_Object *obj, int col, int row)
{

}

void *
enna_wall_selected_data_get(Evas_Object *obj)
{    
    return NULL;
}

const char *
enna_wall_selected_filename_get(Evas_Object *obj)
{
    return NULL;
}

Eina_List*
enna_wall_files_get(Evas_Object* obj)
{
    return NULL;
}

int
enna_wall_jump_label(Evas_Object *obj, const char *label)
{
    return -1;
}

void
enna_wall_jump_ascii(Evas_Object *obj, char k)
{

}
