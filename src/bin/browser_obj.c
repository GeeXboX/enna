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

#include "logs.h"
#include "vfs.h"
#include "input.h"
#include "view_wall.h"
#include "view_list.h"
#include "box.h"
#include "enna_config.h"
#include "search.h"
#include "browser.h"
#include "browser_obj.h"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *o_layout;
    Evas_Object *o_view;
    Enna_Browser_View_Type view_type;
    Ecore_Timer *hilight_timer;
    Enna_Browser *browser;
    Evas_Object *o_header;
    const char *root;
    struct
    {
        Evas_Object *(*view_add)(Smart_Data *sd);
        void (*view_append)(Evas_Object *view,
                            Enna_File *file,
                            void (*func_activated)(void *data),
                            void *data);
        void (*view_remove)(Evas_Object *view,
                            Enna_File *file);
        void *(*view_selected_data_get)(Evas_Object *view);
        int (*view_jump_label)(Evas_Object *view, const char *label);
        Eina_Bool (*view_key_down)(Evas_Object *view, enna_input event);
        void (*view_select_nth)(Evas_Object *obj, int nth);
        Eina_List *(*view_files_get)(Evas_Object *obj);
        void (*view_jump_ascii)(Evas_Object *obj, char k);
    } view_funcs;
};

static int
_view_delay_hilight_cb(void *data)
{
    Smart_Data *sd = data;
    Enna_File *file = sd->view_funcs.view_selected_data_get(sd->o_view);
    DBG(__FUNCTION__);
    sd->hilight_timer = NULL;
    evas_object_smart_callback_call (sd->o_layout, "delay,hilight", file);
    return 0;
}

static void
_view_hilight_cb (void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    DBG(__FUNCTION__);
    evas_object_smart_callback_call (sd->o_layout, "hilight", NULL);
    ENNA_TIMER_DEL(sd->hilight_timer);
    sd->hilight_timer = ecore_timer_add(0.2, _view_delay_hilight_cb, sd);
}


static Evas_Object *
_browser_view_list_add(Smart_Data *sd)
{
    Evas_Object *view;
    DBG(__FUNCTION__);
    if (!sd) return NULL;

    view = enna_list_add(sd->o_layout);

    elm_layout_content_set(sd->o_layout, "enna.swallow.content", view);
    evas_object_smart_callback_add(view, "hilight", _view_hilight_cb, sd);
    /* View */
    edje_object_signal_emit(view, "list,right,now", "enna");
    return view;
}


static Evas_Object *
_browser_box_add(Smart_Data *sd)
{
    Evas_Object *view;
    DBG(__FUNCTION__);
    if (!sd) return NULL;

    view = enna_box_add(enna->layout, NULL);

    elm_layout_content_set(sd->o_layout, "enna.swallow.content", view);
    evas_object_smart_callback_add(view, "hilight", _view_hilight_cb, sd);
    return view;
}

static Evas_Object *
_browser_view_wall_add(Smart_Data *sd)
{
    Evas_Object *view;
    DBG(__FUNCTION__);
    if (!sd) return NULL;

    view = enna_wall_add(enna->evas);

    elm_layout_content_set(sd->o_layout, "enna.swallow.content", view);
    evas_object_smart_callback_add(view, "hilight", _view_hilight_cb, sd);
    return view;
}

static void
_browser_view_wall_select_nth(Evas_Object *view, int nth)
{
    DBG(__FUNCTION__);
    enna_wall_select_nth(view, nth, 0);
}

void
_change_view(Smart_Data *sd, Enna_Browser_View_Type view_type)
{
    DBG(__FUNCTION__);
    sd->view_type = view_type;
    switch(sd->view_type)
    {
    case ENNA_BROWSER_VIEW_LIST:
        sd->view_funcs.view_add                 = _browser_view_list_add;
        sd->view_funcs.view_append              = enna_list_file_append;
        sd->view_funcs.view_remove              = enna_list_file_remove;
        sd->view_funcs.view_selected_data_get   = enna_list_selected_data_get;
        sd->view_funcs.view_jump_label          = enna_list_jump_label;
        sd->view_funcs.view_key_down            = enna_list_input_feed;
        sd->view_funcs.view_select_nth          = enna_list_select_nth;
        sd->view_funcs.view_files_get           = enna_list_files_get;
        sd->view_funcs.view_jump_ascii          = enna_list_jump_ascii;
        break;
    case ENNA_BROWSER_BOX:
        sd->view_funcs.view_add                 = _browser_box_add;
        sd->view_funcs.view_append              = enna_box_file_append;
//        sd->view_funcs.view_remove              = enna_box_file_remove;
        sd->view_funcs.view_selected_data_get   = enna_box_selected_data_get;
        sd->view_funcs.view_jump_label          = enna_box_jump_label;
        sd->view_funcs.view_key_down            = enna_box_input_feed;
        sd->view_funcs.view_select_nth          = enna_box_select_nth;
        sd->view_funcs.view_files_get           = enna_box_files_get;
        sd->view_funcs.view_jump_ascii          = enna_box_jump_ascii;
        break;
    case ENNA_BROWSER_VIEW_WALL:
        sd->view_funcs.view_add                 = _browser_view_wall_add;
        sd->view_funcs.view_append              = enna_wall_file_append;
//        sd->view_funcs.view_remove              = enna_wall_file_remove;
        sd->view_funcs.view_selected_data_get   = enna_wall_selected_data_get;
        sd->view_funcs.view_jump_label          = enna_wall_jump_label;
        sd->view_funcs.view_key_down            = enna_wall_input_feed;
        sd->view_funcs.view_select_nth          = _browser_view_wall_select_nth;
        sd->view_funcs.view_files_get           = enna_wall_files_get;
        sd->view_funcs.view_jump_ascii          = enna_wall_jump_ascii;
    default:
        break;
    }
}
static void
_add_cb(void *data, Enna_File *file)
{
    Smart_Data *sd = data;
    Evas_Object *icon = NULL;
    DBG(__FUNCTION__);

    if (!sd->o_view)
        sd->o_view = sd->view_funcs.view_add(sd);

    if (!file)
    {
        /* No File detected */

    }

    if (file->icon_file && file->icon_file[0] == '/')
    {
        icon = elm_icon_add(sd->o_view);
        elm_icon_file_set(icon, file->icon_file, NULL);
    }
    else
    {
        icon = elm_icon_add(sd->o_view);
        edje_object_file_set(icon, enna_config_theme_get(), file->icon);
    }

    sd->view_funcs.view_append(sd->o_view, file, NULL/*_browse*/, file);
}

static void
_add_header(Smart_Data *sd, const char *uri)
{
    Evas_Object *o_layout;
    Evas_Object *o_edje;
    Evas_Object *o_search_bar;
    Evas_Object *o_back_btn;
    Evas_Object *o_ic;

    ENNA_OBJECT_DEL(sd->o_header);
    
    o_layout = elm_layout_add(sd->o_layout);
    elm_layout_file_set(o_layout, enna_config_theme_get(), "enna/browser/header");

    o_back_btn = elm_button_add(sd->o_layout);
    o_ic = elm_icon_add(o_back_btn);
    elm_icon_file_set(o_ic, enna_config_theme_get(), "icon/back");
    elm_button_icon_set(o_back_btn, o_ic);
    elm_object_style_set(o_back_btn, "mediaplayer");
    
    elm_layout_content_set(o_layout, "enna.swallow.back", o_back_btn);
    o_edje = elm_layout_edje_get(o_layout);
    edje_object_part_text_set(o_layout, "enna.text.current", ecore_file_file_get(uri));

    o_search_bar = enna_search_add(o_layout);
    elm_layout_content_set(o_layout, "enna.swallow.search", o_search_bar);

    elm_layout_content_set(sd->o_layout, "enna.swallow.header", o_layout);
    
    sd->o_header = o_layout;
}

static void
_browse(Smart_Data *sd, Enna_File *file)
{
    const char *uri;

    if (!sd)
        return;

    if (file)
        uri = eina_stringshare_add(file->uri);
    else
        uri = sd->root;

    ENNA_OBJECT_DEL(sd->o_view);
    enna_browser_del(sd->browser);
    if (sd->hilight_timer)
        ecore_timer_del(sd->hilight_timer);
    sd->hilight_timer = NULL;
    sd->o_view = NULL;
    DBG("browse uri : %s\n", uri);
    sd->browser = enna_browser_add(_add_cb, sd, NULL, NULL, uri);

    _add_header(sd, uri);
    
    enna_browser_browse(sd->browser);
    eina_stringshare_del(uri);
}

static void
_browse_back(Smart_Data *sd)
{
    const char *uri = eina_stringshare_add(enna_browser_uri_get(sd->browser));

    ENNA_OBJECT_DEL(sd->o_view);
    enna_browser_del(sd->browser);

    sd->o_view = NULL;
    DBG("browse uri : %s\n", uri);

    if ( uri == sd->root)
    {
        sd->browser = enna_browser_add(_add_cb, sd, NULL, NULL, uri);
        enna_browser_browse(sd->browser);
        evas_object_smart_callback_call (sd->o_layout, "root", NULL);
    }
    else
    {
        char *parent_uri = ecore_file_dir_get(uri);
        sd->browser = enna_browser_add(_add_cb, sd, NULL, NULL, parent_uri);
        if (parent_uri)
            free(parent_uri);
        enna_browser_browse(sd->browser);
    }
    eina_stringshare_del(uri);
}

Eina_List *
enna_browser_obj_files_get(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!sd->browser)
        return NULL;
    else return
        enna_browser_files_get(sd->browser);
}

void
enna_browser_obj_input_feed(Evas_Object *obj, enna_input event)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    DBG(__FUNCTION__);
    switch (event)
    {
    case ENNA_INPUT_BACK:
        _browse_back(sd);
        break;
    case ENNA_INPUT_OK:
    {
        Enna_File *file = sd->view_funcs.view_selected_data_get(sd->o_view);
        if (!file)
            break;
        if (file->is_directory || file->is_menu)
            _browse(sd, file);
        else
            evas_object_smart_callback_call (sd->o_layout, "selected", file);
        break;
    }
    case ENNA_INPUT_UP:
    case ENNA_INPUT_DOWN:
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_LEFT:
    case ENNA_INPUT_NEXT:
    case ENNA_INPUT_PREV:
    case ENNA_INPUT_FIRST:
    case ENNA_INPUT_LAST:
        if (sd->view_funcs.view_key_down)
            sd->view_funcs.view_key_down(sd->o_view, event);
        break;

//     case ENNA_INPUT_KEY_0: _browser_letter_show(sd, "0"); break;
//     case ENNA_INPUT_KEY_1: _browser_letter_show(sd, "1"); break;
//     case ENNA_INPUT_KEY_2: _browser_letter_show(sd, "2"); break;
//     case ENNA_INPUT_KEY_3: _browser_letter_show(sd, "3"); break;
//     case ENNA_INPUT_KEY_4: _browser_letter_show(sd, "4"); break;
//     case ENNA_INPUT_KEY_5: _browser_letter_show(sd, "5"); break;
//     case ENNA_INPUT_KEY_6: _browser_letter_show(sd, "6"); break;
//     case ENNA_INPUT_KEY_7: _browser_letter_show(sd, "7"); break;
//     case ENNA_INPUT_KEY_8: _browser_letter_show(sd, "8"); break;
//     case ENNA_INPUT_KEY_9: _browser_letter_show(sd, "9"); break;
// 
//     case ENNA_INPUT_KEY_A: _browser_letter_show(sd, "a"); break;
//     case ENNA_INPUT_KEY_B: _browser_letter_show(sd, "b"); break;
//     case ENNA_INPUT_KEY_C: _browser_letter_show(sd, "c"); break;
//     case ENNA_INPUT_KEY_D: _browser_letter_show(sd, "d"); break;
//     case ENNA_INPUT_KEY_E: _browser_letter_show(sd, "e"); break;
//     case ENNA_INPUT_KEY_F: _browser_letter_show(sd, "f"); break;
//     case ENNA_INPUT_KEY_G: _browser_letter_show(sd, "g"); break;
//     case ENNA_INPUT_KEY_H: _browser_letter_show(sd, "h"); break;
//     case ENNA_INPUT_KEY_I: _browser_letter_show(sd, "i"); break;
//     case ENNA_INPUT_KEY_J: _browser_letter_show(sd, "j"); break;
//     case ENNA_INPUT_KEY_K: _browser_letter_show(sd, "k"); break;
//     case ENNA_INPUT_KEY_L: _browser_letter_show(sd, "l"); break;
//     case ENNA_INPUT_KEY_M: _browser_letter_show(sd, "m"); break;
//     case ENNA_INPUT_KEY_N: _browser_letter_show(sd, "n"); break;
//     case ENNA_INPUT_KEY_O: _browser_letter_show(sd, "o"); break;
//     case ENNA_INPUT_KEY_P: _browser_letter_show(sd, "p"); break;
//     case ENNA_INPUT_KEY_Q: _browser_letter_show(sd, "q"); break;
//     case ENNA_INPUT_KEY_R: _browser_letter_show(sd, "r"); break;
//     case ENNA_INPUT_KEY_S: _browser_letter_show(sd, "s"); break;
//     case ENNA_INPUT_KEY_T: _browser_letter_show(sd, "t"); break;
//     case ENNA_INPUT_KEY_U: _browser_letter_show(sd, "u"); break;
//     case ENNA_INPUT_KEY_V: _browser_letter_show(sd, "v"); break;
//     case ENNA_INPUT_KEY_W: _browser_letter_show(sd, "w"); break;
//     case ENNA_INPUT_KEY_Z: _browser_letter_show(sd, "z"); break;

    default:
        break;
    }
}

void
enna_browser_obj_view_type_set(Evas_Object *obj,
                               Enna_Browser_View_Type view_type)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    DBG(__FUNCTION__);
    _change_view(sd, view_type);
}

void
enna_browser_obj_root_set(Evas_Object *obj, const char *uri)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");
    if (sd->root)
        eina_stringshare_del(sd->root);
    sd->root = eina_stringshare_add(uri);
    _browse(sd, NULL);
}

Evas_Object *
enna_browser_obj_add(Evas_Object *parent)
{
    Smart_Data *sd;
    DBG(__FUNCTION__);
    sd = calloc(1, sizeof(Smart_Data));

    sd->o_layout = elm_layout_add(parent);
    evas_object_size_hint_weight_set(sd->o_layout, -1.0, -1.0);
    evas_object_size_hint_align_set(sd->o_layout, 1.0, 1.0);
    elm_layout_file_set(sd->o_layout, enna_config_theme_get(), "enna/browser");

    evas_object_data_set(sd->o_layout, "sd", sd);

    enna_browser_obj_view_type_set(sd->o_layout, ENNA_BROWSER_VIEW_LIST);

    return sd->o_layout;
}
