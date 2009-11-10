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
    void (*func_selected) (void *data);
    void *data;
    Elm_Genlist_Item *item;
};

struct _Smart_Data
{
    Eina_List *items;
};


void _item_activated(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    Elm_Genlist_Item *item = event_info;
    List_Item *it = NULL;
    Eina_List *l;

    if (!sd) return;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->item == item)
        {
            if (it->func_activated) it->func_activated(it->data);
            return;
        }
    }
}

/* List View */
static char *
_list_item_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Enna_Vfs_File *item = data;

    if (!item) return NULL;

    return strdup(item->label);
}

static Evas_Object *
_list_item_icon_get(const void *data, Evas_Object *obj, const char *part)
{
    Enna_Vfs_File *item = (Enna_Vfs_File *) data;

    if (!item) return NULL;

    if (!strcmp(part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        ic = elm_icon_add(obj);
        if (item->icon && item->icon[0] == '/')
            elm_icon_file_set(ic, item->icon, NULL);
        else
            elm_icon_file_set(ic, enna_config_theme_get(), item->icon);
        evas_object_size_hint_min_set(ic, 64, 64);
        evas_object_show(ic);
        return ic;
    }

    return NULL;
}

static Eina_Bool
_list_item_state_get(const void *data, Evas_Object *obj, const char *part)
{
    return EINA_FALSE;
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
        _list_item_state_get,
        _list_item_del
    }
};


static void _smart_select_item(Smart_Data *sd, int n)
{
    List_Item *it;

    it = eina_list_nth(sd->items, n);
    if (!it) return;

    elm_genlist_item_bring_in(it->item);
    elm_genlist_item_selected_set(it->item, 1);
}

static void list_set_item(Smart_Data *sd, int start, int up, int step)
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

    evas_object_data_set(obj, "sd", sd);

    evas_object_smart_callback_add(obj, "clicked", _item_activated, sd);

    return obj;
}

void enna_list_file_append(Evas_Object *obj, Enna_Vfs_File *file, void (*func_activated) (void *data),  void (*func_selected) (void *data), void *data)
{
    Smart_Data *sd;
    List_Item *it;

    sd = evas_object_data_get(obj, "sd");

    it = ENNA_NEW(List_Item, 1);
    it->item = elm_genlist_item_append (obj, &itc_list, file,
        NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL );

    it->func_activated = func_activated;
    it->func_selected = func_selected;
    it->data = data;
    it->file = file;

    sd->items = eina_list_append(sd->items, it);
}

void enna_list_select_nth(Evas_Object *obj, int nth)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    _smart_select_item(sd, nth);
}

Eina_List* enna_list_files_get(Evas_Object* obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    Eina_List *files = NULL;
    Eina_List *l;
    List_Item *it;

    EINA_LIST_FOREACH(sd->items, l, it)
        files = eina_list_append(files, it->file);

    return files;
}

int enna_list_jump_label(Evas_Object *obj, const char *label)
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

int enna_list_selected_get(Evas_Object *obj)
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

void *enna_list_selected_data_get(Evas_Object *obj)
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

void enna_list_jump_ascii(Evas_Object *obj, char k)
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
        case ENNA_INPUT_HOME:
            list_set_item(sd, -1, 1, 1);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_END:
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
