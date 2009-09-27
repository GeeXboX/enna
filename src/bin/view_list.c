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
    void *data;
    void (*func) (void *data);
    Elm_Genlist_Item *item;
    const char *label;
    Enna_Vfs_File *file;
};

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_smart;
    Evas_Object *o_edje;
    Evas_Object *o_list;
    Eina_List *items;
    Elm_Genlist_Item_Class *item_class;
};

static void _smart_init(void);
static void _smart_add(Evas_Object *obj);
static void _smart_del(Evas_Object *obj);
static void _smart_show(Evas_Object *obj);
static void _smart_hide(Evas_Object *obj);
static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _smart_clip_unset(Evas_Object *obj);
static void _smart_reconfigure(Smart_Data *sd);
static void _smart_select_item(Smart_Data *sd, int n);

static Evas_Smart *_e_smart = NULL;

Evas_Object *
enna_list_add(Evas *evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _e_smart);
}

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
            if (it->func) it->func(it->data);
            return;
        }
    }
}

void enna_list_file_append(Evas_Object *obj, Enna_Vfs_File *file, void (*func) (void *data),  void *data)
{

    List_Item *it;
    API_ENTRY return;

    it = calloc(1, sizeof(List_Item));
    it->item = elm_genlist_item_append (sd->o_list, sd->item_class, file,
        NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL );

    it->func = func;
    it->data = data;
    it->label = eina_stringshare_add(file->label);
    it->file = file;
    
    sd->items = eina_list_append(sd->items, it);
}

void enna_list_select_nth(Evas_Object *obj, int nth)
{
    API_ENTRY return;
    _smart_select_item(sd, nth);
}

Eina_List* enna_list_files_get(Evas_Object* obj)
{
    API_ENTRY return NULL;
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

    API_ENTRY return -1;

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

    API_ENTRY return -1;

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

    API_ENTRY return NULL;

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
    API_ENTRY return;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label[0] == k || it->label[0] == k - 32)
        {
            _smart_select_item(sd, i);
            return;
        }
        i++;
    }
}

/* SMART FUNCTIONS */
static void _smart_init(void)
{
    if (_e_smart)
        return;
    {
        static const Evas_Smart_Class sc =
        {
            SMART_NAME,
            EVAS_SMART_CLASS_VERSION,
            _smart_add,
            _smart_del,
            _smart_move,
            _smart_resize,
            _smart_show,
            _smart_hide,
            _smart_color_set,
            _smart_clip_set,
            _smart_clip_unset,
            NULL,
            NULL
        };
        _e_smart = evas_smart_class_new(&sc);
    }
}

/* List View */
static char *_view_list_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Enna_Vfs_File *item = data;

    if (!item) return NULL;

    return strdup(item->label);
}

static Evas_Object *_view_list_icon_get(const void *data, Evas_Object *obj, const char *part)
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
//	item->ic = ic;
        return ic;
    }

    return NULL;
}

static Eina_Bool _view_list_state_get(const void *data, Evas_Object *obj, const char *part)
{
    return EINA_FALSE;
}

static void _view_list_del(const void *data, Evas_Object *obj)
{
    Enna_Vfs_File *item = (void *) data;

    if (!item) return;

    //ENNA_OBJECT_DEL(item->ic);
}

static void _smart_add(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    evas_object_smart_data_set(obj, sd);

    sd->o_smart = obj;
    sd->x = sd->y = sd->w = sd->h = 0;

    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "enna/list");

    sd->o_list = elm_genlist_add(obj);
    evas_object_size_hint_align_set(sd->o_list, -1.0, -1.0);
    evas_object_size_hint_weight_set(sd->o_list, 1.0, 1.0);
    elm_genlist_horizontal_mode_set(sd->o_list, ELM_LIST_LIMIT);
    evas_object_show(sd->o_list);

    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", sd->o_list);

    sd->item_class = calloc(1, sizeof(Elm_Genlist_Item_Class));

    sd->item_class->item_style     = "default";
    sd->item_class->func.label_get = _view_list_label_get;
    sd->item_class->func.icon_get  = _view_list_icon_get;
    sd->item_class->func.state_get = _view_list_state_get;
    sd->item_class->func.del       = _view_list_del;

    evas_object_smart_callback_add(sd->o_list, "clicked", _item_activated, sd);

    evas_object_smart_member_add(sd->o_edje, obj);

    evas_object_propagate_events_set(obj, 0);
}

static void _smart_del(Evas_Object *obj)
{
    Eina_List *list = NULL;
    Eina_List *l;
    Eina_List *l_prev;
    List_Item *it;

    INTERNAL_ENTRY;
    
    evas_object_del(sd->o_edje);
    EINA_LIST_REVERSE_FOREACH_SAFE(sd->items, l, l_prev, it)
    {
	elm_genlist_item_del(it->item);
        free(it);
        list = eina_list_remove_list(list, l);
    }
    elm_genlist_clear(sd->o_list);
    evas_object_del(sd->o_list);
    free(sd);
}

static void _smart_show(Evas_Object *obj)
{
    INTERNAL_ENTRY ;
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object *obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;
    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;
    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;

    _smart_reconfigure(sd);
}

static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object *obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_edje);
}

static void _smart_reconfigure(Smart_Data *sd)
{
    evas_object_move(sd->o_edje, sd->x, sd->y);
    evas_object_resize(sd->o_edje, sd->w, sd->h);
}

static void _smart_select_item(Smart_Data *sd, int n)
{
    List_Item *it;

    it = eina_list_nth(sd->items, n);
    if (!it) return;

    elm_genlist_item_show(it->item);
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

Eina_Bool
enna_list_input_feed(Evas_Object *obj, enna_input event)
{
    int ns;
    API_ENTRY return ENNA_EVENT_CONTINUE;

    printf("INPUT.. to list %d\n", event);
    ns = enna_list_selected_get(sd->o_smart);

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
            List_Item *it = eina_list_nth(sd->items, enna_list_selected_get(sd->o_smart));
            if (it)
            {
                if (it->func)
                    it->func(it->data);
            }
        }
            return ENNA_EVENT_BLOCK;
            break;
       default:
            break;
    }
    return ENNA_EVENT_CONTINUE;
}
