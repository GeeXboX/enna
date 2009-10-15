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

typedef enum {
    ENNA_UNKNOW,
    ENNA_BUTTON,
    ENNA_TOGGLE,     //TODO finish
    ENNA_CHECKBOX,
    ENNA_HOVERSEL,   //TODO implement
    ENNA_SPINNER,    //TODO implement
}list_control_type;

typedef struct _Item_Data Item_Data;
struct _Item_Data
{
    const char *label1;        /**< primary label (title) */
    const char *label2;        /**< secondary label (subtitle) */
    const char *icon;          /**< icon for the list item */
    void (*func) (void *data); /**< function to call when list item is selected */
    void *func_data;           /**< data to return-back */
    Eina_List *buttons;        /**< list of Item_Button pointers */
};

typedef struct _Item_Button Item_Button;
struct _Item_Button
{
    Evas_Object *obj;          /**< elementary button */
    list_control_type type;    /**< type of the control (button, toggle, etc) */
    const char *label;         /**< label for the button */
    const char *icon;          /**< icon for the button */
    Eina_Bool status;          /**< initial status for checks buttons */
    void (*func) (void *data); /**< function to call when button pressed */
    void *func_data;           /**< data to return-back */
};

/***   Local protos  ***/
static void _list_item_activate(Item_Data *id);
static void _list_button_activate(Item_Button *b);
static Evas_Object *_list_item_buttons_create_all(const Item_Data *id);

/***   Callbacks  ***/
/*static void // called on list item double-click
_list_item_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    Item_Data *id = data;
    printf("CLICK on %s\n", id->label);
    _list_item_activate(id);
}*/

static void // called on list item selection (higlight)
_list_item_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    //Elm_Genlist_Item *item = event_info;
    Item_Data *id = data;
    Eina_List *l;
    Item_Button *b;

    EINA_LIST_FOREACH(id->buttons, l, b)
        elm_object_disabled_set(b->obj, EINA_TRUE);
}

static void // called when one of the buttons is pressed
_list_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    Item_Button *b = data;

    _list_button_activate(b);
}

/***   Privates  ***/
static void
_enna_list_item_widget_add(Elm_Genlist_Item *item, const char *icon, const char *label,
                           void (*func) (void *data),
                           void *func_data,
                           list_control_type type,
                           Eina_Bool status)
{
    Item_Data *id = (Item_Data *)elm_genlist_item_data_get(item);
    Item_Button *b;

    if (!item) return;

    b = ENNA_NEW(Item_Button, 1);
    if (!b) return;

    b->icon = eina_stringshare_add(icon);
    b->label = eina_stringshare_add(label);
    b->func = func;
    b->func_data = func_data;
    b->type = type;
    b->status = status;

    id->buttons = eina_list_append(id->buttons, b);
}

static void
_list_item_activate(Item_Data *id)
{
    if (!id) return;

    // execute the item activate callback
    if (id->func) id->func(id->func_data);
}

static void
_list_button_activate(Item_Button *b)
{
    if (!b) return;

    // execute the button activate callback
    if (b->func) b->func(b->func_data);
}

static Evas_Object *
_list_item_buttons_create_all(const Item_Data *id)
{
    Evas_Object *box;
    Eina_List *l;
    Item_Button *b;

    if (!id->buttons) return NULL;

    box = elm_box_add(enna->layout);
    elm_box_horizontal_set(box, EINA_TRUE);

    EINA_LIST_FOREACH(id->buttons, l, b)
    {
        Evas_Object *o, *ic;

        o = ic = NULL;

        // create icon if needed
        if (b->type == ENNA_BUTTON)
        {
            ic = elm_icon_add(enna->layout);
            if (b->icon && b->icon[0] == '/')
                elm_icon_file_set(ic, b->icon, NULL);
            else if (b->icon)
                elm_icon_file_set(ic, enna_config_theme_get(), b->icon);
            evas_object_show(ic);
        }

        // create the widget of the given type
        switch (b->type)
        {
        case ENNA_BUTTON:
            o = elm_button_add(enna->layout);
            elm_object_style_set(o, "enna_list");
            elm_button_label_set(o, b->label);
            elm_button_icon_set(o, ic);
            evas_object_smart_callback_add(o, "clicked", _list_button_clicked_cb, b);
            break;
        case ENNA_TOGGLE:
            o = elm_toggle_add(enna->layout);
            break;
        case ENNA_CHECKBOX:
            o = elm_check_add(enna->layout);
            elm_object_style_set(o, "enna_list");
            elm_check_label_set(o, b->label);
            elm_check_icon_set(o, ic);
            elm_check_state_set(o, b->status);
            evas_object_smart_callback_add(o, "changed", _list_button_clicked_cb, b);
            break;
        default:
            break;
        }

        if (o)
        {
            evas_object_size_hint_weight_set(o, 1.0, 1.0);
            evas_object_size_hint_align_set(o, -1.0, -1.0);
            evas_object_show(o);
            elm_box_pack_end(box, o);
            b->obj = o;
        }
    }
    
    return box;
}

/***   Genlist Class Implementation  ***/
static char *
_list_item_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Item_Data *id = data;

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
    else if (!strcmp(part, "elm.swallow.end"))
    {
        return _list_item_buttons_create_all(id);
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
    Item_Button *b;

    if (!id) return;
    EINA_LIST_FREE(id->buttons, b)
    {
        ENNA_STRINGSHARE_DEL(b->label);
        ENNA_STRINGSHARE_DEL(b->icon);
        ENNA_FREE(b);
    }
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

Elm_Genlist_Item *
enna_list2_append(Evas_Object *obj, const char *label1, const char *label2,
                  const char *icon,
                  void (*func)(void *data), void *func_data)
{
    Elm_Genlist_Item *item;
    Item_Data *id;

    id = ENNA_NEW(Item_Data, 1);
    if (!id) return NULL;
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

    return item;
}

void
enna_list2_file_append(Evas_Object *obj, Enna_Vfs_File *file,
                       void (*func)(void *data), void *func_data)
{
    enna_list2_append(obj, file->label, NULL, file->icon, func, func_data);
    //TODO the caller expect I will free the Vfs_File ??
}

void
enna_list2_item_button_add(Elm_Genlist_Item *item, const char *icon, const char *label,
                           void (*func) (void *data), void *func_data)
{
    _enna_list_item_widget_add(item, icon, label, func, func_data, ENNA_BUTTON, EINA_FALSE);
}

void //TODO to finish
enna_list2_item_toggle_add(Elm_Genlist_Item *item, const char *icon, const char *label,
                           void (*func) (void *data), void *func_data)
{
    _enna_list_item_widget_add(item, icon, label, func, func_data, ENNA_TOGGLE, EINA_FALSE);
}

void
enna_list2_item_check_add(Elm_Genlist_Item *item, const char *icon, const char *label,
                          Eina_Bool status, void (*func) (void *data), void *func_data)
{
    _enna_list_item_widget_add(item, icon, label, func, func_data, ENNA_CHECKBOX, status);
}

/* Events input */
static Item_Button *
_list_item_button_focus_next(Elm_Genlist_Item *item, Item_Button *cur)
{
    Item_Data *id = (Item_Data *)elm_genlist_item_data_get(item);
    Item_Button *next;

    if (!item || !id) return NULL;

    // item currently selected, select the next one if exists
    if (cur)
    {
        Eina_List *l;

        l = eina_list_next(eina_list_data_find_list(id->buttons, cur));
        if (l)
        {
            next = l->data;
            elm_object_disabled_set(next->obj, EINA_FALSE);
            elm_object_disabled_set(cur->obj, EINA_TRUE);
            return next;
        }
    }
    // none selected, select the first
    else if (id->buttons)
    {
        next = (id->buttons->data);
        elm_object_disabled_set(next->obj, EINA_FALSE);
        return next;
    }

    return cur;
}

static Item_Button *
_list_item_button_focus_prev(Elm_Genlist_Item *item, Item_Button *cur)
{
    Item_Data *id = (Item_Data *)elm_genlist_item_data_get(item);
    Item_Button *prev;

    if (!item || !id) return NULL;

    if (cur)
    {
        Eina_List *l;

        l = eina_list_prev(eina_list_data_find_list(id->buttons, cur));
        if (l)
        {
            prev = l->data;
            elm_object_disabled_set(prev->obj, EINA_FALSE);
            elm_object_disabled_set(cur->obj, EINA_TRUE);
            return prev;
        }
        elm_object_disabled_set(cur->obj, EINA_TRUE);
        return NULL;
    }

    return NULL;
}

Eina_Bool
enna_list2_input_feed(Evas_Object *obj, enna_input event)
{
    Elm_Genlist_Item *item, *prev, *next;
    static Item_Button *focused = NULL;

    //printf("INPUT.. to list2 %d\n", event);
    if (!obj) return ENNA_EVENT_CONTINUE;

    item = elm_genlist_selected_item_get(obj);
    if (!item) return ENNA_EVENT_CONTINUE;
    

    switch (event)
    {
        case ENNA_INPUT_RIGHT:
            focused = _list_item_button_focus_next(item, focused);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_LEFT:
            focused = _list_item_button_focus_prev(item, focused);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_UP:
            prev = elm_genlist_item_prev_get(item);
            if (prev)
            {
                elm_genlist_item_selected_set(prev, EINA_TRUE);
                elm_genlist_item_bring_in(prev);
                focused = NULL;
                return ENNA_EVENT_BLOCK;
            }
            break;
        case ENNA_INPUT_DOWN:
            next = elm_genlist_item_next_get(item);
            if (next)
            {
                elm_genlist_item_selected_set(next, EINA_TRUE);
                elm_genlist_item_bring_in(next);
                focused = NULL;
                return ENNA_EVENT_BLOCK;
            }
            break;
        case ENNA_INPUT_OK:
            if (focused)
            {
                if(focused->type == ENNA_CHECKBOX)
                    elm_check_state_set(focused->obj, !elm_check_state_get(focused->obj));

                _list_button_activate(focused);
            }
            else
            {
                _list_item_activate((Item_Data*)elm_genlist_item_data_get(item));
            }
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

