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
#include "view_list2.h"
#include "enna_config.h"


typedef struct _Item_Data Item_Data;
struct _Item_Data
{
    void (*func) (void *data);
    void *func_data;
    const char *label1;
    const char *label2;
    const char *icon;
};

/***   Local Globals  ***/



/***   Privates  ***/
static void
_list_item_activate(Item_Data *id)
{
    if (!id) return;
    //printf("ACTIVATE %s\n", id->label);

    // execute the activate function
    if (id->func) id->func(id->func_data);
}

/***   Callbacks  ***/
//~ static void // called on item double-click
//~ _list_item_clicked_cb(void *data, Evas_Object *obj, void *event_info)
//~ {
    //~ Item_Data *id = data;
    //~ printf("CLICK on %s\n", id->label);
    //~ _list_item_activate(id);
//~ }

static void // called on item selection (higlight)
_list_item_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    Item_Data *id = data;
    //Elm_Genlist_Item *item = event_info;
    
    printf("SEL %s\n", id->label1);
}

/***   Genlist Class   ***/
static char *
_list_item_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Item_Data *id = data;

    printf(" * label_get(,,%s)\n", part);
    if (!id) return NULL;
    
    if (!strcmp(part, "elm.text") && id->label1)
        return strdup(id->label1);
    if (!strcmp(part, "elm.text.sub") && id->label2)
        return strdup(id->label2);
    return NULL;
}

static Evas_Object *
_list_item_icon_get(const void *data, Evas_Object *obj, const char *part)
{
    const Item_Data *id = data;

    //printf(" * icon_get()\n");
    if (!id) return NULL;

    if (!strcmp(part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        ic = elm_icon_add(obj);
        if (id->icon && id->icon[0] == '/')
            elm_icon_file_set(ic, id->icon, NULL);
        else
            elm_icon_file_set(ic, enna_config_theme_get(), id->icon);
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
    Item_Data *id = (Item_Data *)data;
    printf("DEL\n");

    if (!id) return;
    ENNA_STRINGSHARE_DEL(id->label1);
    ENNA_STRINGSHARE_DEL(id->label2);
    ENNA_STRINGSHARE_DEL(id->icon);
    ENNA_FREE(id);
}

static Elm_Genlist_Item_Class itc_single_label = {
    "enna_list",
    {
        _list_item_label_get,
        _list_item_icon_get,
        _list_item_state_get,
        _list_item_del
    }
};

/***   Public API  ***/
Evas_Object *
enna_list2_add(Evas *evas) // evas is unused
{
    Evas_Object *obj;

    obj = elm_genlist_add(enna->layout);
    evas_object_size_hint_align_set(obj, -1.0, -1.0);
    evas_object_size_hint_weight_set(obj, 1.0, 1.0);
    elm_genlist_horizontal_mode_set(obj, ELM_LIST_LIMIT);
    //~ evas_object_smart_callback_add(obj, "clicked", _list_item_clicked_cb, NULL);
    evas_object_show(obj);

    return obj;
}

void
enna_list2_append(Evas_Object *obj, const char *label1, const char *label2,
                  const char *icon,
                  void (*func)(void *data), void *func_data)
{
    Elm_Genlist_Item *item;
    Item_Data *id;

    id = ENNA_NEW(Item_Data, 1);
    if (!id) return;
    id->func = func;
    id->func_data = func_data;
    id->label1 = eina_stringshare_add(label1);
    id->label2 = eina_stringshare_add(label2);
    id->icon = eina_stringshare_add(icon);

    item = elm_genlist_item_append(obj, &itc_single_label, id, NULL,
                                   ELM_GENLIST_ITEM_NONE,
                                   _list_item_selected_cb, id);

    if (!elm_genlist_selected_item_get(obj))
        elm_genlist_item_selected_set(item, EINA_TRUE);
}

void
enna_list2_file_append(Evas_Object *obj, Enna_Vfs_File *file,
                       void (*func)(void *data), void *func_data)
{
    enna_list2_append(obj, file->label, NULL, file->icon, func, func_data);
    //TODO the caller expect I will free the Vfs_File ??
}

Eina_Bool
enna_list2_input_feed(Evas_Object *obj, enna_input event)
{
    Elm_Genlist_Item *item, *prev, *next;

    //printf("INPUT.. to list2 %d\n", event);
    if (!obj) return ENNA_EVENT_CONTINUE;

    item = elm_genlist_selected_item_get(obj);
    if (!item) return ENNA_EVENT_CONTINUE;

    switch (event)
    {
        case ENNA_INPUT_UP:
            prev = elm_genlist_item_prev_get(item);
            if (prev)
            {
                elm_genlist_item_selected_set(prev, EINA_TRUE);
                elm_genlist_item_bring_in(prev);
                return ENNA_EVENT_BLOCK;
            }
            break;
        case ENNA_INPUT_DOWN:
            next = elm_genlist_item_next_get(item);
            if (next)
            {
                elm_genlist_item_selected_set(next, EINA_TRUE);
                elm_genlist_item_bring_in(next);
                return ENNA_EVENT_BLOCK;
            }
            break;
        case ENNA_INPUT_OK:
            _list_item_activate((Item_Data*)elm_genlist_item_data_get(item));
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_HOME:
            prev = elm_genlist_first_item_get(obj);
            elm_genlist_item_selected_set(prev, EINA_TRUE);
            elm_genlist_item_bring_in(prev);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_END:
            next = elm_genlist_last_item_get(obj);
            elm_genlist_item_selected_set(next, EINA_TRUE);
            elm_genlist_item_bring_in(next);
            return ENNA_EVENT_BLOCK;
            break;
        //~ case ENNA_INPUT_PREV:
            //~ list_set_item(sd, ns, 0, 5);
            //~ return ENNA_EVENT_BLOCK;
            //~ break;
        //~ case ENNA_INPUT_NEXT:
            //~ list_set_item(sd, ns, 1, 5);
            //~ return ENNA_EVENT_BLOCK;
            //~ break;
        default:
            break;
    }

    return ENNA_EVENT_CONTINUE;
}

