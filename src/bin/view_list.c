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
#include "logs.h"
#include "mediaplayer.h"
#include "utils.h"

#define SMART_NAME "enna_list"

typedef struct _Smart_Data Smart_Data;
typedef struct _List_Item List_Item;

struct _List_Item
{
    Enna_File *file;
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
_item_click_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
    Elm_Genlist_Item *item = data;
    Evas_Event_Mouse_Up *ev = event_info;

    /* Don't activate when user is scrolling list */
    if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
        return;
    _item_activate(item);
}

static void
_item_realized_cb(void *data __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Elm_Genlist_Item *item = event_info;
   Evas_Object *o_item;

   o_item = (Evas_Object*)elm_genlist_item_object_get(item);
   evas_object_event_callback_add(o_item, EVAS_CALLBACK_MOUSE_UP,_item_click_cb, item);
}


static void
_file_meta_update(void *data, Enna_File *file __UNUSED__)
{
    List_Item *it = data;
    if (!it || !it->item)
        return;
    elm_genlist_item_update(it->item);
}


static void
_item_remove(Evas_Object *obj, List_Item *item)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd || !item) return;

    sd->items = eina_list_remove(sd->items, item);
    enna_file_meta_callback_del(item->file, _file_meta_update);
    enna_file_free(item->file);
    elm_genlist_item_del(item->item);
    free(item);

    return;
}

/* List View */
/* Default genlist items */
static char *
_list_item_default_label_get(void *data, Evas_Object *obj __UNUSED__, const char *part __UNUSED__)
{
    const List_Item *li = data;

    if (!li || !li->file) return NULL;

    return li->file->label ? strdup(li->file->label) : NULL;
}

static Evas_Object *
_list_item_default_icon_get(void *data, Evas_Object *obj, const char *part)
{
    List_Item *li = (List_Item*) data;

    if (!li) return NULL;

    if (!strcmp(part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        if (!li->file || (li->file->type != ENNA_FILE_MENU && li->file->type != ENNA_FILE_VOLUME) )
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

        if (!li->file || !ENNA_FILE_IS_BROWSABLE(li->file))
            return NULL;

        ic = elm_icon_add(obj);
        elm_icon_file_set(ic, enna_config_theme_get(), "icon/arrow_right");
        evas_object_size_hint_min_set(ic, 24, 24);
        evas_object_show(ic);
        return ic;
    }

    return NULL;
}

/* Tracks relative  genlist items */
static char *
_list_item_track_label_get(void *data, Evas_Object *obj __UNUSED__, const char *part)
{
    const List_Item *li = data;
    const char *title;
    const char *track;
    const char *duration;
    const char *sduration;
    char *tmp;

    if (!li || !li->file) return NULL;

    if (!strcmp(part, "elm.text.title"))
    {
        title = enna_file_meta_get(li->file, "title");
        if (!title || !title[0] || title[0] == ' ')
            return li->file->label ? strdup(li->file->label) : NULL;
        else
        {
            tmp = strdup(title);
            eina_stringshare_del(title);
            return tmp;
        }
    }
    else if (!strcmp(part, "elm.text.trackno"))
    {
        track = enna_file_meta_get(li->file, "track");
        if (!track)
            return NULL;
        else
        {
            track = eina_stringshare_printf("%02d.", atoi(track));
            tmp = strdup(track);
            eina_stringshare_del(track);
            return tmp;
        }
    }
    else if (!strcmp(part, "elm.text.length"))
    {
        duration = enna_file_meta_get(li->file, "duration");
        if (!duration)
            duration = enna_file_meta_get(li->file, "length");
        if (!duration)
            return NULL;
        sduration = enna_util_duration_to_string(duration);
        if (!sduration)
        {
            eina_stringshare_del(duration);
            return NULL;
        }
        tmp = strdup(sduration);
        eina_stringshare_del(sduration);
        return tmp;
    }

    return NULL;
}

static Evas_Object *
_list_item_track_icon_get(void *data, Evas_Object *obj, const char *part)
{
    List_Item *li = (List_Item*) data;

    if (!li) return NULL;

    if (!strcmp(part, "elm.swallow.starred"))
    {
        Evas_Object *ic;
        const char *starred;
        if (!li->file || ENNA_FILE_IS_BROWSABLE(li->file))
            return NULL;

        starred = enna_file_meta_get(li->file, "starred");
        printf("Starred : %s\n", starred);
        if (!starred)
            return NULL;
        ic = elm_icon_add(obj);
        elm_icon_file_set(ic, enna_config_theme_get(), "icon/favorite");
        evas_object_size_hint_min_set(ic, 24, 24);
        evas_object_show(ic);
        return ic;
    }
    else if (!strcmp(part, "elm.swallow.playing"))
    {
        Evas_Object *ic;
        const char *tmp;

        if (!li->file || ENNA_FILE_IS_BROWSABLE(li->file))
            return NULL;
        tmp = enna_mediaplayer_get_current_uri();
        if (!tmp)
            return NULL;

        else if (strcmp(li->file->mrl, tmp))
        {
            eina_stringshare_del(tmp);
            return NULL;
        }
        eina_stringshare_del(tmp);
        ic = elm_icon_add(obj);
        printf("PLAYING icon add\n");
        elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_play");
        evas_object_size_hint_min_set(ic, 24, 24);
        evas_object_show(ic);
        return ic;
    }


    return NULL;
}


static Elm_Genlist_Item_Class itc_list_default = {
    "default",
    {
        _list_item_default_label_get,
        _list_item_default_icon_get,
        NULL,
        NULL
    }
};

static Elm_Genlist_Item_Class itc_list_track = {
    "track",
    {
        _list_item_track_label_get,
        _list_item_track_icon_get,
        NULL,
        NULL
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
_smart_unselect_item(Smart_Data *sd, int n)
{
    List_Item *it;

    it = eina_list_nth(sd->items, n);
    if (!it) return;

    elm_genlist_item_selected_set(it->item, 0);
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
_del_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
    Smart_Data *sd = data;

    if (!sd)
        return;

    enna_list_clear(obj);
    eina_list_free(sd->items);

    free(sd);
}

Evas_Object *
enna_list_add(Evas_Object *parent)
{
    Evas_Object *obj;
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));

    obj = elm_genlist_add(parent);
    /* Don't let elm focused genlist object, keys are handle by enna */
    elm_object_focus_allow_set(obj, EINA_FALSE);
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
enna_list_file_append(Evas_Object *obj, Enna_File *file,
                      void (*func_activated) (void *data),  void *data)
{
    Smart_Data *sd;
    List_Item *it;

    sd = evas_object_data_get(obj, "sd");

    it = ENNA_NEW(List_Item, 1);

    it->func_activated = func_activated;
    it->data = data;
    it->file = enna_file_ref(file);

    if (file->type == ENNA_FILE_TRACK)
    {
        it->item = elm_genlist_item_append (obj, &itc_list_track, it,
                                            NULL, ELM_GENLIST_ITEM_NONE,
                                            _item_selected, it);

        /* Track file needs meta data update if any */
        enna_file_meta_callback_add(file, _file_meta_update, it);
    }
    else
    {
        it->item = elm_genlist_item_append (obj, &itc_list_default, it,
                                            NULL, ELM_GENLIST_ITEM_NONE,
                                            _item_selected, it);
    }
    sd->items = eina_list_append(sd->items, it);
    /* Select first item */
    if (eina_list_count(sd->items) == 1)
        enna_list_select_nth(obj, 0);
}

void
enna_list_file_remove(Evas_Object *obj, Enna_File *file)
{
    Smart_Data *sd;
    List_Item *it;
    Eina_List *l;

    sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->file == file)
        {
            _item_remove(obj, it);
            break;
        }
    }
}

void
enna_list_file_update(Evas_Object *obj, Enna_File *file)
{
    Smart_Data *sd;
    List_Item *it;
    Eina_List *l;

    sd = evas_object_data_get(obj, "sd");

    EINA_LIST_FOREACH(sd->items, l, it)
    {
        DBG("%s == %s", it->file->name, file->name);
        if (it->file == file)
        {
            DBG("Update genlist item");
            if (it->item)
                elm_genlist_item_update(it->item);
            break;
        }
    }
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

#define LIST_SEEK_OFFSET 5

static Eina_Bool
view_list_item_select (Evas_Object *obj, int down, int cycle, int range)
{
    int ns, total, start, end;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    ns    = enna_list_selected_get(obj);
    total = eina_list_count(sd->items);
    start = down ? total - 1 : 0;
    end   = down ? 0 : total - 1;

    if (ns == start)
    {
        _smart_unselect_item(sd, start);
        return ENNA_EVENT_CONTINUE;
    }
    else if (cycle)
    {
        if (!down && (ns - range < 0))
            _smart_select_item(sd, 0);
        else if (down && (ns + range > total - 1))
            _smart_select_item(sd, total - 1);
        else
            list_set_item(sd, ns, down, range);
    }
    else
    {
        if (ns == -1 && !down)
            list_set_item(sd, end + 1, down, range);
        else
            list_set_item(sd, ns, down, range);
    }

    return ENNA_EVENT_BLOCK;
}

Eina_Bool
enna_list_input_feed(Evas_Object *obj, enna_input event)
{
    int total;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd)
        return ENNA_EVENT_CONTINUE;

    total = eina_list_count(sd->items);

    switch (event)
    {
        case ENNA_INPUT_UP:
            return view_list_item_select(obj, 0, 0, 1);
        case ENNA_INPUT_PREV:
            return view_list_item_select(obj, 0, 1, LIST_SEEK_OFFSET);
        case ENNA_INPUT_DOWN:
            return view_list_item_select(obj, 1, 0, 1);
        case ENNA_INPUT_NEXT:
            return view_list_item_select(obj, 1, 1, LIST_SEEK_OFFSET);
        case ENNA_INPUT_FIRST:
            _smart_select_item(sd, 0);
            return ENNA_EVENT_BLOCK;
            break;
        case ENNA_INPUT_LAST:
            _smart_select_item(sd, total - 1);
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

    EINA_LIST_FOREACH_SAFE(sd->items, l, l_next, item)
    {
        _item_remove(obj, item);
    }
    elm_genlist_clear(obj);
}
