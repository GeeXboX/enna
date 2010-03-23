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

#include <dirent.h>

#include <Ecore_File.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "module.h"
#include "logs.h"
#include "view_wall.h"
#include "content.h"
#include "mainmenu.h"
#include "enna_config.h"
#include "input.h"
#include "image.h"
#include "games.h"
#include "games_sys.h"
#include "games_mame.h"

#define ENNA_MODULE_NAME "games"


typedef enum _GAMES_STATE {
    MENU_VIEW,
    SERVICE_VIEW
} GAMES_STATE;

typedef struct _Enna_Module_Games {
    Evas_Object  *o_edje;
    Evas_Object  *o_menu;
    Evas_Object  *o_image;
    Evas_Object  *o_bg;
    GAMES_STATE   state;
    Eina_Bool     was_fs;
    Ecore_Event_Handler *exe_handler;
    Enna_Module  *em;
    Games_Service *current;
    Eina_List     *services;
} Enna_Module_Games;



static Enna_Module_Games *mod;

/*****************************************************************************/
/*                              Games Helpers                                */
/*****************************************************************************/

static void
_game_service_set_bg(const char *bg)
{
    if (bg)
    {
        Evas_Object *old;

        old = mod->o_bg;
        mod->o_bg = edje_object_add(evas_object_evas_get(mod->o_edje));
        edje_object_file_set(mod->o_bg, enna_config_theme_get(), bg);
        edje_object_part_swallow(mod->o_edje,
                                 "service.bg.swallow", mod->o_bg);
        evas_object_show(mod->o_bg);
        evas_object_del(old);
    }
    else
    {
        evas_object_hide(mod->o_bg);
        edje_object_part_swallow(mod->o_edje, "service.bg.swallow", NULL);
    }
}

static Eina_Bool
_game_service_init(Games_Service *s)
{
    if (!s)
        return EINA_FALSE;

    if (s->init)
        return (s->init)();
    else
        return EINA_TRUE;
}

static Eina_Bool
_game_service_shutdown(Games_Service *s)
{
    if (!s)
        return EINA_FALSE;

    if (s->shutdown)
        return (s->shutdown)();
    else
        return EINA_TRUE;
}

static void
_game_service_show(Games_Service *s)
{
    if (!s)
        return;

    if (s->show)
        (s->show)(mod->o_edje);

    _game_service_set_bg(s->bg);

    edje_object_signal_emit(mod->o_edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "service,show", "enna");

    mod->state = SERVICE_VIEW;
    mod->current = s;
}

static void
_game_service_hide(Games_Service *s)
{
    if (!s)
        return;

    if (s && s->hide)
        (s->hide)(mod->o_edje);

    mod->current = NULL;
    mod->state = MENU_VIEW;

    edje_object_signal_emit(mod->o_edje, "service,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
}

static void
_menu_item_cb_selected(void *data)
{
    Games_Service *s = data;

    _game_service_show(s);
}

static void
_menu_add(Games_Service *s)
{
    Enna_Vfs_File *f;

    if (!s)
        return;

    f          = calloc (1, sizeof(Enna_Vfs_File));
    f->icon    = (char *) eina_stringshare_add(s->icon);
    f->label   = (char *) eina_stringshare_add(s->label);
    f->is_menu = 1;

    enna_wall_file_append(mod->o_menu, f, _menu_item_cb_selected, s);
}

/*****************************************************************************/
/*                         Games Public API                                  */
/*****************************************************************************/
void
games_service_image_show(const char *file)
{
    Evas_Object *old;

    edje_object_signal_emit(mod->o_edje, "image,hide", "enna");

    if (!file)
    {
        edje_object_signal_emit(mod->o_edje, "image,hide", "enna");
        evas_object_del(mod->o_image);
        return;
    }

    old = mod->o_image;
    mod->o_image = enna_image_add(enna->evas);
    enna_image_fill_inside_set(mod->o_image, 1);
    enna_image_file_set(mod->o_image, file, NULL);

    edje_object_part_swallow(mod->o_edje,
                             "service.games.image.swallow", mod->o_image);
    edje_object_signal_emit(mod->o_edje, "image,show", "enna");
    evas_object_del(old);
    evas_object_show(mod->o_image);
}

void
games_service_title_show(const char *title)
{
    edje_object_part_text_set(mod->o_edje, "service.games.name.str", title);
}

void
games_service_total_show(int tot)
{
    char buf[128];

    if (tot == 0)
        snprintf(buf, sizeof(buf), _("no games found"));
    else if (tot == 1)
        snprintf(buf, sizeof(buf), _("one game"));
    else
        snprintf(buf, sizeof(buf), _("%d games"), tot);
    edje_object_part_text_set(mod->o_edje, "service.games.counter.str", buf);
}

static int
_games_service_exec_exit_cb(void *data, int ev_type, void *ev)
{
    // Ecore_Exe_Event_Del *e = ev;
    Evas_Object *o_msg = data;

    /* Delete messagge */
    ENNA_OBJECT_DEL(o_msg);
    
    ENNA_EVENT_HANDLER_DEL(mod->exe_handler);

    //~ enna_input_event_thaw();

    /* Restore fullscreen state */
    if (mod->was_fs)
        elm_win_fullscreen_set(enna->win, EINA_TRUE);

   return 1;
}

void
games_service_exec(const char *cmd, const char *text)
{
    Evas_Object *o_msg;

    //~ enna_input_event_freeze();

    /* This hack to prevent gnome (at least) to get confused on having
     * two fullscreen windows (enna + mame) */
    mod->was_fs = elm_win_fullscreen_get(enna->win);
    elm_win_fullscreen_set(enna->win, EINA_FALSE);

    /* Exec Command */
    o_msg = enna_util_message_show(text);
    ecore_exe_run(cmd, NULL);
    mod->exe_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                            _games_service_exec_exit_cb, o_msg);
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "Game started: %s", cmd);
}
/**
 * TODO move this functions in a proper file
 **/
static void
_enna_util_message_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Input_Listener *listener = data;

    enna_input_listener_del(listener);
}

static Eina_Bool
_enna_util_message_events_cb(void *data, enna_input event)
{
    Evas_Object *inwin = data;

    switch(event)
    {
        case ENNA_INPUT_QUIT:
        case ENNA_INPUT_BACK:
        case ENNA_INPUT_OK:
            ENNA_OBJECT_DEL(inwin);
            break;
        default:
            break;
    }
    return ENNA_EVENT_BLOCK;
}

Evas_Object *
enna_util_message_show(const char *text)
{
    Evas_Object *inwin;
    Evas_Object *label;
    Input_Listener *listener;

    inwin = elm_win_inwin_add(enna->win);
    elm_object_style_set(inwin, "enna_minimal");
    
    label = elm_label_add(inwin);
    elm_object_style_set(label, enna);
    elm_label_label_set(label, text);
    evas_object_show(label);
    
    elm_win_inwin_content_set(inwin, label);
    elm_win_inwin_activate(inwin);

    listener = enna_input_listener_add("enna_msg",
                                       _enna_util_message_events_cb, inwin);
    evas_object_event_callback_add(inwin, EVAS_CALLBACK_DEL,
                                   _enna_util_message_del_cb, listener);
    return inwin;
}

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void
_class_show(int dummy)
{
    /* create the activity content once for all */
    if (!mod->o_edje)
    {
        mod->o_edje = edje_object_add(enna->evas);
        edje_object_file_set(mod->o_edje, enna_config_theme_get(),
                             "activity/games");
        enna_content_append(ENNA_MODULE_NAME, mod->o_edje);
    }

    /* create the menu, once for all */
    if (!mod->o_menu)
    {
        Eina_List *l;
        Games_Service *s;
    
        mod->o_menu = enna_wall_add(enna->evas);

        EINA_LIST_FOREACH(mod->services, l, s)
            _menu_add(s);

        enna_wall_select_nth(mod->o_menu, 0, 0);
        edje_object_part_swallow(mod->o_edje, "menu.swallow", mod->o_menu);
        mod->state = MENU_VIEW;
    }

    /* show module */
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
}

static void
_class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "service,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
    _game_service_hide(mod->current);
}

static void
_class_event(enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed Games : %d", event);

    switch (mod->state)
    {
    /* Menu View */
    case MENU_VIEW:
    {
        if (event == ENNA_INPUT_BACK)
        {
            enna_content_hide();
            enna_mainmenu_show();
        }
        else
            enna_wall_input_feed(mod->o_menu, event);
        break;
    }
    /* Service View */
    case SERVICE_VIEW:
    {
        Eina_Bool b = ENNA_EVENT_BLOCK;
        if (mod->current && mod->current->event)
            b = (mod->current->event)(mod->o_edje, event);

        if ((b == ENNA_EVENT_CONTINUE) && (event == ENNA_INPUT_BACK))
            _game_service_hide(mod->current);
        break;
    }
    default:
        break;
    }
}

static Enna_Class_Activity
class =
{
    ENNA_MODULE_NAME,
    9,
    N_("Games"),
    NULL,
    "icon/games",
    "background/games",
    {
        NULL,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_games
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = ENNA_NEW(Enna_Module_Games, 1);
    mod->em = em;
    em->mod = mod;

    /* Add activity */
    enna_activity_add(&class);

    if (_game_service_init(&games_sys) == EINA_TRUE)
        mod->services = eina_list_append(mod->services, &games_sys);

    if (_game_service_init(&games_mame) == EINA_TRUE)
        mod->services = eina_list_append(mod->services, &games_mame);
}

static void
module_shutdown(Enna_Module *em)
{
    Games_Service *s;

    EINA_LIST_FREE(mod->services, s)
        _game_service_shutdown(s);

    enna_activity_del(ENNA_MODULE_NAME);
    ENNA_OBJECT_DEL(mod->o_menu);
    ENNA_OBJECT_DEL(mod->o_image);
    ENNA_OBJECT_DEL(mod->o_bg);
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_FREE(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_games",
    N_("Games"),
    "icon/games",
    N_("Play all your games directly from Enna"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
