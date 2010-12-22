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

#include <Eina.h>
#include <Edje.h>
#include <Elementary.h>

#include "browser.h"
#include "enna.h"
#include "enna_config.h"
#include "mainmenu.h"
#include "content.h"
#include "input.h"
#include "logs.h"
#include "box.h"
#include "image.h"
#include "exit.h"
#include "volume_notification.h"
#include "mediaplayer.h"
#include "gadgets.h"

typedef struct _Smart_Data Smart_Data;
typedef struct _Activated_Cb_Data Activated_Cb_Data;

struct _Activated_Cb_Data
{
	Smart_Data *sd;
	Enna_File *file;
};

struct _Smart_Data
{
    Evas_Object *o_menu;
    Evas_Object *o_background;
    Evas_Object *o_weather;
    Evas_Object *o_volume;
    Enna_Vfs_File *selected;
    Input_Listener *listener;
    Ecore_Event_Handler *act_handler;
    Eina_Bool visible;
    Eina_Bool exit_visible;
    Eina_List *backgrounds;
    Enna_Browser *browser;
};

/* Local subsystem functions */

static void
_add_cb(void *data, Enna_Vfs_File *file)
{
    Smart_Data *sd = data;
    enna_mainmenu_append(sd->o_menu, file);
}

static void
_enna_mainmenu_load_from_activities(Smart_Data *sd)
{
    enna_box_clear(sd->o_menu);
    sd->browser = enna_browser_add(_add_cb, sd, NULL, NULL, NULL, NULL, "/");
    enna_browser_browse(sd->browser);
}

static void
_enna_mainmenu_item_focus(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    Activated_Cb_Data *cb_data = event_info;

    if (!cb_data || !cb_data->file)
        return;

    enna_mainmenu_background_select(sd->o_menu, cb_data->file);
}

static void
_enna_mainmenu_item_activate(void *data)
{
	Activated_Cb_Data *cb_data = data;

    enna_mainmenu_hide(cb_data->sd->o_menu);

    // run the activity_show cb. that is responsable of showing the
    // content using enna_content_select("name")
    enna_activity_show(cb_data->file->name);
    cb_data->sd->selected = cb_data->file;
}

/* Local subsystem callbacks */


static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    Smart_Data *sd = data;

    if (!sd) return ENNA_EVENT_CONTINUE;

    if (event == ENNA_INPUT_FULLSCREEN)
    {
        enna->run_fullscreen = ~enna->run_fullscreen;
        elm_win_fullscreen_set(enna->win, enna->run_fullscreen);
        return ENNA_EVENT_BLOCK;
    }

    /* check for volume control bindings */
    if (event == ENNA_INPUT_MUTE)
    {
      enna_mediaplayer_mute();
      enna_volume_notification_show(sd->o_volume);
      return ENNA_EVENT_BLOCK;
    }
    else if (event == ENNA_INPUT_VOLPLUS)
    {
      enna_mediaplayer_default_increase_volume();
      enna_volume_notification_show(sd->o_volume);
      return ENNA_EVENT_BLOCK;
    }
    else if (event == ENNA_INPUT_VOLMINUS)
    {
      enna_mediaplayer_default_decrease_volume();
      enna_volume_notification_show(sd->o_volume);
      return ENNA_EVENT_BLOCK;
    }

    if (sd->visible)
    {
        switch (event)
        {
            case ENNA_INPUT_RIGHT:
            case ENNA_INPUT_LEFT:
            case ENNA_INPUT_UP:
            case ENNA_INPUT_DOWN:
            case ENNA_INPUT_OK:
                enna_box_input_feed(sd->o_menu, event);
                return ENNA_EVENT_BLOCK;
                break;
            case ENNA_INPUT_BACK:
                return ENNA_EVENT_BLOCK;
            default:
                break;
        }
    }
    else if (event == ENNA_INPUT_HOME)
    {
        enna_content_hide();
        enna_mainmenu_show(sd->o_menu);
        return ENNA_EVENT_BLOCK;
    }
    if (!sd->visible)
    {
        Enna_Vfs_File *f = enna_mainmenu_selected_activity_get(sd->o_menu);
        enna_activity_event(enna_activity_get(f->name), event);
    }

    return ENNA_EVENT_CONTINUE;
}

/* externally accessible functions */
Evas_Object *
enna_mainmenu_add(Evas_Object *parent)
{
    Smart_Data *sd;
    sd = ENNA_NEW(Smart_Data, 1);
    if (!sd) return NULL;

    /* cover view */
    sd->o_menu = enna_box_add(parent, "list");
    evas_object_data_set(sd->o_menu, "mainmenu_data", sd);

    //elm_layout_content_set(enna->layout, "enna.mainmenu.swallow", sd->o_menu);
    evas_object_size_hint_align_set(sd->o_menu, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(sd->o_menu, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    evas_object_show(sd->o_menu);
    /* Add  background*/
    sd->o_background = elm_slideshow_add(parent);
    elm_layout_content_set(enna->layout, "enna.background.swallow", sd->o_background);
    elm_slideshow_loop_set(sd->o_background, EINA_TRUE);
    evas_object_size_hint_weight_set(sd->o_background, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(sd->o_background);
    elm_slideshow_transition_set(sd->o_background, "fade");
    sd->backgrounds = NULL;
    evas_object_smart_callback_add(sd->o_menu, "hilight", _enna_mainmenu_item_focus, sd);
    /* Volume widget */
    sd->o_volume =
        enna_volume_notification_smart_add(evas_object_evas_get(enna->layout));
    elm_layout_content_set(enna->layout,
                           "enna.volume.swallow", sd->o_volume);

    /* connect to the input signal */
    sd->listener = enna_input_listener_add("mainmenu", _input_events_cb, sd);
    _enna_mainmenu_load_from_activities(sd);

    return sd->o_menu;
}

void
enna_mainmenu_shutdown(Evas_Object *obj)
{
    Eina_List *l;
    const char *name;
    Smart_Data *sd;

    sd =evas_object_data_get(obj, "mainmenu_data");
    ENNA_EVENT_HANDLER_DEL(sd->act_handler);

    enna_input_listener_del(sd->listener);

    ENNA_OBJECT_DEL(sd->o_menu);
    ENNA_OBJECT_DEL(sd->o_background);
    ENNA_OBJECT_DEL(sd->o_weather);
    ENNA_OBJECT_DEL(sd->o_volume);
    EINA_LIST_FOREACH(sd->backgrounds, l, name)
    {
        sd->backgrounds = eina_list_remove(sd->backgrounds, name);
        if (name) eina_stringshare_del(name);
    }
    enna_browser_del(sd->browser);
    ENNA_FREE(sd);
}

void
enna_mainmenu_append(Evas_Object *obj, Enna_Vfs_File *f)
{
    Smart_Data *sd;
    Activated_Cb_Data *cb_data;

    sd = evas_object_data_get(obj, "mainmenu_data");

    if (!f) return;

    cb_data = malloc(sizeof(Activated_Cb_Data));
    cb_data->sd = sd;
    cb_data->file = f;

    enna_box_file_append(sd->o_menu, f, _enna_mainmenu_item_activate, cb_data);

    enna_mainmenu_background_add(sd->o_menu, f);
    if (!enna_box_selected_data_get(sd->o_menu))
    {
        enna_box_select_nth(sd->o_menu, 0);
    }
}

Enna_Vfs_File *
enna_mainmenu_selected_activity_get(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "mainmenu_data");

    if (!sd) return 0;
    return sd->selected;
}

void
enna_mainmenu_show(Evas_Object *obj)
{
    Evas_Object *ic;
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "mainmenu_data");

    if (!sd) return;
    sd->visible = 1;
    sd->selected = NULL;
    enna_gadgets_show();
    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,show", "enna");
    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "gadgets,show", "enna");

    ic = elm_icon_add(enna->o_button_back);
    elm_icon_file_set(ic, enna_config_theme_get(), "ctrl/shutdown");
    elm_button_icon_set(enna->o_button_back, ic);

}

void
enna_mainmenu_hide(Evas_Object *obj)
{
    Smart_Data *sd;
    Evas_Object *ic;

    sd = evas_object_data_get(obj, "mainmenu_data");

    if (!sd) return;
    sd->visible = 0;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,hide", "enna");
    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "gadgets,hide", "enna");

    ic = elm_icon_add(enna->o_button_back);
    elm_icon_file_set(ic, enna_config_theme_get(), "icon/arrow_left");
    elm_button_icon_set(enna->o_button_back, ic);
    enna_gadgets_hide();
}

typedef struct _Bg_Data
{
    Enna_File *file;
    Elm_Slideshow_Item *it;
}Bg_Data;

static Evas_Object *
_background_get(void *data, Evas_Object *obj)
{
    Evas_Object *o_bg = elm_icon_add(obj);
    elm_icon_file_set(o_bg, enna_config_theme_get(), data);
    return o_bg;
}

static Elm_Slideshow_Item_Class itc = {
    {
        _background_get,
        NULL
    }
};


void
enna_mainmenu_background_add(Evas_Object *obj, Enna_File *file)
{
    Smart_Data *sd;
    Elm_Slideshow_Item *it;
    Bg_Data *bg;


    if (!file)
        return;

    sd = evas_object_data_get(obj, "mainmenu_data");
    bg = calloc(1, sizeof(Bg_Data));

    it = elm_slideshow_item_add(sd->o_background, &itc, file->icon_file);
    bg->it = it;
    bg->file = file;
    sd->backgrounds = eina_list_append(sd->backgrounds, bg);
}

void
enna_mainmenu_background_select(Evas_Object *obj, Enna_File *file)
{
    Bg_Data *bg;
    Eina_List *l;
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "mainmenu_data");

    EINA_LIST_FOREACH(sd->backgrounds, l, bg)
    {
        if (bg->file == file)
        {
            elm_slideshow_show(bg->it);
            break;
        }
    }
}

Eina_Bool
enna_mainmenu_visible(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "mainmenu_data");

    if (!sd) return EINA_FALSE;
    return sd->visible;
}
