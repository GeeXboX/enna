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



typedef struct _Menu_Data Menu_Data;

struct _Menu_Data
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

static Menu_Data *sd = NULL;


/* Local subsystem functions */

static void
_add_cb(void *data, Enna_Vfs_File *file)
{
    enna_mainmenu_append(file);
}

static void
_enna_mainmenu_load_from_activities(void)
{
    enna_box_clear(sd->o_menu);
    sd->browser = enna_browser_add(_add_cb, NULL, NULL, NULL, "/");
    enna_browser_browse(sd->browser);
}

static void
_enna_mainmenu_item_focus(void *data, Evas_Object *obj, void *event_info)
{
    Enna_Vfs_File *f = event_info;
    if (!f)
        return;

    enna_mainmenu_background_select(f->icon_file);
}

static void
_enna_mainmenu_item_activate(void *data)
{
    Enna_Vfs_File *file = data;
    enna_mainmenu_hide();

    // run the activity_show cb. that is responsable of showing the
    // content using enna_content_select("name")
    enna_activity_show(file->name);
    sd->selected = file;
}

/* Local subsystem callbacks */


static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
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
        enna_mainmenu_show();
        return ENNA_EVENT_BLOCK;
    }
    if (!sd->visible)
    {
        Enna_Vfs_File *f = enna_mainmenu_selected_activity_get();
        enna_activity_event(enna_activity_get(f->name), event);
    }

    return ENNA_EVENT_CONTINUE;
}

/* externally accessible functions */
Evas_Object *
enna_mainmenu_init(void)
{
    if (sd) return sd->o_menu;

    sd = ENNA_NEW(Menu_Data, 1);
    if (!sd) return NULL;

    /* cover view */
    sd->o_menu = enna_box_add(enna->layout, "list");
    elm_layout_content_set(enna->layout, "enna.mainmenu.swallow", sd->o_menu);
    evas_object_size_hint_align_set(sd->o_menu, -1.0, -1.0);
    evas_object_size_hint_weight_set(sd->o_menu, 1.0, 1.0);
    evas_object_show(sd->o_menu);
    /* Add  background*/
    sd->o_background = NULL;
    sd->backgrounds = NULL;

    /* Volume widget */
    sd->o_volume =
        enna_volume_notification_smart_add(evas_object_evas_get(enna->layout));
    elm_layout_content_set(enna->layout,
                           "enna.volume.swallow", sd->o_volume);

    /* connect to the input signal */
    sd->listener = enna_input_listener_add("mainmenu", _input_events_cb, NULL);
    _enna_mainmenu_load_from_activities();

    return sd->o_menu;
}

void
enna_mainmenu_shutdown(void)
{
    Eina_List *l;
    const char *name;
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
enna_mainmenu_append(Enna_Vfs_File *f)
{
    if (!f) return;
    
    enna_box_file_append(sd->o_menu, f, _enna_mainmenu_item_activate, f);
    evas_object_smart_callback_add(sd->o_menu, "hilight", _enna_mainmenu_item_focus, f);
    enna_mainmenu_background_add(f->icon_file);
    if (!enna_box_selected_data_get(sd->o_menu))
    {
        enna_box_select_nth(sd->o_menu, 0);
    }
}

Enna_Vfs_File *
enna_mainmenu_selected_activity_get(void)
{
    if (!sd) return 0;
    return sd->selected;
}

void
enna_mainmenu_show(void)
{
    Evas_Object *ic;

    if (!sd) return;
    sd->visible = 1;
    sd->selected = NULL;
    enna_gadgets_show();
    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,show", "enna");
    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "gadgets,show", "enna");

    ic = elm_icon_add(enna->layout);
    elm_icon_file_set(ic, enna_config_theme_get(), "ctrl/shutdown");
    elm_button_icon_set(enna->o_button_back, ic);

}

void
enna_mainmenu_hide(void)
{ 
    Evas_Object *ic;
    if (!sd) return;
    sd->visible = 0;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,hide", "enna");
    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "gadgets,hide", "enna");

    ic = elm_icon_add(enna->layout);
    elm_icon_file_set(ic, enna_config_theme_get(), "icon/arrow_left");
    elm_button_icon_set(enna->o_button_back, ic);
    enna_gadgets_hide();
}

void
enna_mainmenu_background_add(const char *name)
{
    if (!name)
        return;
    sd->backgrounds = eina_list_append(sd->backgrounds, name);
}

void
enna_mainmenu_background_select(const char *name)
{
    const char *n;
    Eina_List *l;
 
    EINA_LIST_FOREACH(sd->backgrounds, l, n)
    {
        if (!strcmp(name, n))
        {
            ENNA_OBJECT_DEL(sd->o_background);
            sd->o_background = elm_image_add(enna->layout);
            elm_image_file_set(sd->o_background, enna_config_theme_get(), name);
            elm_layout_content_set(enna->layout, "enna.background.swallow", sd->o_background);
            break;
        }
    }
}

Eina_Bool
enna_mainmenu_visible(void)
{
    if (!sd) return EINA_FALSE;
    return sd->visible;
}
