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

#include <string.h>

#include <Eina.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "mainmenu.h"
#include "content.h"
#include "input.h"
#include "logs.h"
#include "view_cover.h"
#include "image.h"

typedef struct _Background_Item Background_Item;

struct _Background_Item
{
    const char *name;
    const char *filename;
    const char *key;
};

typedef struct _Menu_Data Menu_Data;

struct _Menu_Data
{
    Evas_Object *o_menu;
    Evas_Object *o_background;
    Enna_Class_Activity *selected;
    Input_Listener *listener;
    Ecore_Event_Handler *act_handler;
    Eina_Bool visible;
    Eina_Bool exit_visible;
    Eina_List *backgrounds;
};

static Menu_Data *sd = NULL;


/* Local subsystem functions */
static void
_enna_mainmenu_load_from_activities(void)
{
    Enna_Class_Activity *act;
    Eina_List *l;

    enna_view_cover_clear(sd->o_menu);

    EINA_LIST_FOREACH(enna_activities_get(), l, act)
    {
        enna_mainmenu_append(act);
    }
}

static void
_enna_mainmenu_item_focus(void *data)
{
    Enna_Class_Activity *act = data;
    if (!act)
        return;

    enna_mainmenu_background_select(act->name);
}

static void
_enna_mainmenu_item_activate(void *data)
{
    Enna_Class_Activity *act = data;

    enna_mainmenu_hide();

    // run the activity_show cb. that is responsable of showing the
    // content using enna_content_select("name")
    enna_activity_show(act->name);
    sd->selected = act;
}

/* Local subsystem callbacks */
static int
_activities_changed_cb(void *data, int type, void *event)
{
    _enna_mainmenu_load_from_activities();
    // TODO reselect selected activities
    return ECORE_CALLBACK_RENEW;
}

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

    if (sd->visible)
    {
        switch (event)
        {
            case ENNA_INPUT_RIGHT:
            case ENNA_INPUT_LEFT:
            case ENNA_INPUT_UP:
            case ENNA_INPUT_DOWN:
            case ENNA_INPUT_OK:
                enna_view_cover_input_feed(sd->o_menu, event);
                return ENNA_EVENT_BLOCK;
                break;
            default:
                break;
        }
    }
    else if (event == ENNA_INPUT_MENU)
    {
        enna_content_hide();
        enna_mainmenu_show();
        return ENNA_EVENT_BLOCK;
    }
    if (!sd->visible)
    {
        enna_activity_event(enna_mainmenu_selected_activity_get(), event);
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
    sd->o_menu = enna_view_cover_add(evas_object_evas_get(enna->layout), 0);
    elm_layout_content_set(enna->layout, "enna.mainmenu.swallow", sd->o_menu);
    evas_object_size_hint_weight_set(sd->o_menu, -1.0, -1.0);

    /* Add  background*/
    sd->o_background = NULL;
    sd->backgrounds = NULL;

    /* connect to the input signal */
    sd->listener = enna_input_listener_add("mainmenu", _input_events_cb, NULL);

    sd->act_handler = ecore_event_handler_add(ENNA_EVENT_ACTIVITIES_CHANGED,
                                          _activities_changed_cb, NULL);

    return sd->o_menu;
}

void
enna_mainmenu_shutdown(void)
{
    Eina_List *l;
    Background_Item *it;

    ENNA_EVENT_HANDLER_DEL(sd->act_handler);

    enna_input_listener_del(sd->listener);

    ENNA_OBJECT_DEL(sd->o_menu);
    ENNA_OBJECT_DEL(sd->o_background);
    EINA_LIST_FOREACH(sd->backgrounds, l, it)
    {
        sd->backgrounds = eina_list_remove(sd->backgrounds, it);
        if (it->name) eina_stringshare_del(it->name);
        if (it->filename) eina_stringshare_del(it->filename);
        if (it->key) eina_stringshare_del(it->key);
        ENNA_FREE(it);
    }
    ENNA_FREE(sd);
}

void
enna_mainmenu_append(Enna_Class_Activity *act)
{
    Enna_Vfs_File *f;

    if (!act) return;

    f = calloc(1, sizeof(Enna_Vfs_File));
    f->label = (char*)eina_stringshare_add(act->label);
    f->icon = (char*)eina_stringshare_add(act->icon);
    enna_view_cover_file_append(sd->o_menu, f, _enna_mainmenu_item_activate, _enna_mainmenu_item_focus, act);

    /*if (act->background && act->background[0] == '/')
        enna_mainmenu_background_add(act->name, act->background, NULL);
    else
        enna_mainmenu_background_add(act->name, enna_config_theme_get(), act->background);
    */
    enna_mainmenu_background_add(act->name, enna_config_theme_get(), act->icon);
}

Enna_Class_Activity *
enna_mainmenu_selected_activity_get(void)
{
    if (!sd) return 0;
    return sd->selected;
}

void
enna_mainmenu_show(void)
{
    if (!sd) return;
    sd->visible = 1;
    sd->selected = NULL;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,show", "enna");
}

void
enna_mainmenu_hide(void)
{
    if (!sd) return;
    sd->visible = 0;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,hide", "enna");
}

void
enna_mainmenu_background_add(const char *name, const char *filename, const char *key)
{
    Background_Item *item;

    if (!name)
        return;

    item = ENNA_NEW(Background_Item, 1);
    item->name = name ? eina_stringshare_add(name) : NULL;
    item->filename = filename ? eina_stringshare_add(filename) : NULL;
    item->key = key ? eina_stringshare_add(key) : NULL;

    sd->backgrounds = eina_list_append(sd->backgrounds, item);
}

void
enna_mainmenu_background_select(const char *name)
{
    Background_Item *it;
    Eina_List *l;

    EINA_LIST_FOREACH(sd->backgrounds, l, it)
    {
        if (!strcmp(name, it->name))
        {
            ENNA_OBJECT_DEL(sd->o_background);
            sd->o_background = elm_image_add(enna->layout);
            elm_image_file_set(sd->o_background, it->filename, it->key);
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

Eina_Bool
enna_mainmenu_exit_visible(void)
{
    if (!sd) return 0;
    return sd->exit_visible;
}

/* local subsystem functions */

