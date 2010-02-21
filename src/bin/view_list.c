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

#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "view_list.h"
#include "input.h"
#include "vfs.h"

#define SMART_NAME "enna_list"

typedef struct _Smart_Data Smart_Data;
typedef struct _List_Item List_Item;

struct _List_Item
{
    Enna_Vfs_File *file;
    void (*func_activated) (void *data);
    void *data;
    Elm_Genlist_Item *item;
};

struct _Smart_Data
{
    Evas_Object *obj;
    Eina_List *items;
};


static void
_item_activate(Elm_Genlist_Item *item)
{
    List_Item *li;

    li = (List_Item*)elm_genlist_item_data_get(item);
    if (li->func_activated)
            li->func_activated(li->data);
}

static void
_item_selected(void *data, Evas_Object *obj, void *event_info)
{
    List_Item *li = data;

    evas_object_smart_callback_call(obj, "hilight", li->data);
}

static void
_item_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Elm_Genlist_Item *item = data;

    /* Activate item only if it's already selected */
    if (elm_genlist_item_selected_get(item))
        _item_activate(item);
}

static void
_item_realized_cb(void *data, Evas_Object *obj, void *event_info)
{
   Elm_Genlist_Item *item = event_info;
   Evas_Object *o_item;

   o_item = (Evas_Object*)elm_genlist_item_object_get(item);
   evas_object_event_callback_add(o_item, EVAS_CALLBACK_MOUSE_UP,_item_click_cb, item);
}

static void
_item_remove(Evas_Object *obj, List_Item *item)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd || !item) return;

    sd->items = eina_list_remove(sd->items, item);
    ENNA_FREE(item);

    return;
}

/* List View */
static char *
_list_item_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const List_Item *li = data;

    if (!li || !li->file) return NULL;

    return strdup(li->file->label);
}

static Evas_Object *
_list_item_icon_get(const void *data, Evas_Object *obj, const char *part)
{
    List_Item *li = (List_Item*) data;

    if (!li) return NULL;

    if (!strcmp(part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        if (!li->file->is_menu)
            return NULL;

        ic = elm_icon_add(obj);
        if (li->file->icon && li->file->icon[0] == '/')
            elm_icon_file_set(ic, li->file->icon, NULL);
        else if (li->file->icon)
            elm_icon_file_set(ic, enna_config_theme_get(), li->file->icon);
        else
            return NULL;
        evas_object_size_hint_min_set(ic, 32, 32);
        evas_object_show(ic);
        return ic;
    }
    else if (!strcmp(part, "elm.swallow.end"))
    {
        Evas_Object *ic;

        if (!li->file->is_directory)
            return NULL;

        ic = elm_icon_add(obj);
        elm_icon_file_set(ic, enna_config_theme_get(), "icon/arrow_right");
        evas_object_size_hint_min_set(ic, 24, 24);
        evas_object_show(ic);
        return ic;
    }

    return NULL;
}

static void
_list_item_del(const void *data, Evas_Object *obj)
{
    Enna_Vfs_File *item = (void *) data;

    if (!item) return;
}

static Elm_Genlist_Item_Class itc_list = {
    "default",
    {
        _list_item_label_get,
        _list_item_icon_get,
        NULL,
        _list_item_del
    }
};


static void
_smart_select_item(Smart_Data *sd, int n)
{
    List_Item *it;

    it = eina_list_nth(sd->items, n);
    if (!it) return;

    elm_genlist_item_middle_bring_in(it->item);
    elm_genlist_item_selected_set(it->item, 1);
    evas_object_smart_callback_call(sd->obj, "hilight", it->data);
}

static void
list_set_item(Smart_Data *sd, int start, int up, int step)
{
    int n, ns;

    ns = start;
    n = start;

    int boundary = up ? eina_list_count(sd->items) - 1 : 0;

    if (n == boundary)
        n = ns;

    n = up ? n + step : n - step;

    if (n != ns)
        _smart_select_item(sd, n);
}

static void
_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    if (!sd)
        return;

    enna_list_clear(sd->obj);
    eina_list_free(sd->items);
    free(sd);
}

Evas_Object *
enna_list_add(Evas *evas)
{
    Evas_Object *obj;
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));

    obj = elm_genlist_add(enna->layout);
    evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_genlist_horizontal_mode_set(obj, ELM_LIST_COMPRESS);
    evas_object_show(obj);
    sd->obj = obj;

    evas_object_data_set(obj, "sd", sd);

    evas_object_smart_callback_add(obj, "realized", _item_realized_cb, sd);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _del_cb, sd);

    return obj;
}

void
enna_list_file_append(Evas_Object *obj, Enna_Vfs_File *file,
                      void (*func_activated) (void *data),  void *data)
{
    Smart_Data *sd;
    List_Item *it;

    sd = evas_object_data_get(obj, "sd");

    it = ENNA_NEW(List_Item, 1);
    it->item = elm_genlist_item_append (obj, &itc_list, it,
        NULL, ELM_GENLIST_ITEM_NONE, _item_selected, it);

    it->func_activated = func_activated;
    it->data = data;
    it->file = file;

    sd->items = eina_list_append(sd->items, it);
}

void
enna_list_select_nth(Evas_Object *obj, int nth)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    _smart_select_item(sd, nth);
}

Eina_List *
enna_list_files_get(Evas_Object* obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    Eina_List *files = NULL;
    Eina_List *l;
    List_Item *it;

    EINA_LIST_FOREACH(sd->items, l, it)
        files = eina_list_append(files, it->file);

    return files;
}

int
enna_list_jump_label(Evas_Object *obj, const char *label)
{
    List_Item *it = NULL;
    Eina_List *l;
    int i = 0;

    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd || !label) return -1;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
         if (it->file->label && !strcmp(it->file->label, label))
        {
            _smart_select_item(sd, i);
              return i;
        }
        i++;
    }

    return -1;
}

int
enna_list_selected_get(Evas_Object *obj)
{
    Eina_List *l;
    List_Item *it;
    int i = 0;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd->items) return -1;
    EINA_LIST_FOREACH(sd->items,l, it)
    {
        if ( elm_genlist_item_selected_get (it->item))
        {
            return i;
        }
        i++;
    }
    return -1;
}

void *
enna_list_selected_data_get(Evas_Object *obj)
{
    Eina_List *l;
    List_Item *it;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd->items) return NULL;

    EINA_LIST_FOREACH(sd->items,l, it)
    {
        if ( elm_genlist_item_selected_get (it->item))
        {
            return it->data;
        }
    }
    return NULL;
}

void
enna_list_jump_ascii(Evas_Object *obj, char k)
{
    List_Item *it;
    Eina_List *l;
    int i = 0;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->file->label[0] == k || it->file->label[0] == k - 32)
        {
            _smart_select_item(sd, i);
            return;
        }
        i++;
    }
}

Eina_Bool
enna_list_input_feed(Evas_Object *obj, enna_input event)
{
    int ns;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    ns = enna_list_selected_get(obj);

    switch (event)
    {
        case ENNA_INPUT_UP:
            list_set_item(sd, ns, 0, 1);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_PREV:
            list_set_item(sd, ns, 0, 5);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_DOWN:
            list_set_item(sd, ns, 1, 1);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_NEXT:
            list_set_item(sd, ns, 1, 5);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_FIRST:
            list_set_item(sd, -1, 1, 1);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_LAST:
            list_set_item(sd, eina_list_count(sd->items), 0, 1);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_OK:
        {
            List_Item *it = eina_list_nth(sd->items, enna_list_selected_get(obj));
            if (it)
            {
                if (it->func_activated)
                    it->func_activated(it->data);
            }
        }
            return ENNA_EVENT_BLOCK;
            break;
       default:
            break;
    }
    return ENNA_EVENT_CONTINUE;
}

void
enna_list_clear(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    List_Item *item;
    Eina_List *l, *l_next;

    elm_genlist_clear(obj);
    EINA_LIST_FOREACH_SAFE(sd->items, l, l_next, item)
    {
        _item_remove(obj, item);
    }
}
