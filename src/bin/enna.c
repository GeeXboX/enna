/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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

#define _GNU_SOURCE
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <Edje.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Str.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "mainmenu.h"
#include "exit.h"
#include "module.h"
#include "content.h"
#include "logs.h"
#include "volumes.h"
#include "metadata.h"
#include "mediaplayer.h"
#include "ipc.h"
#include "input.h"
#include "url_utils.h"

/* seconds after which the mouse pointer disappears*/
#define ENNA_MOUSE_IDLE_TIMEOUT 10


/* Global Variable Enna *enna*/
Enna *enna;

static char *conffile = NULL;
static char *theme_name = NULL;
static unsigned int app_w = 1280;
static unsigned int app_h = 720;
static int run_fullscreen = 0;

/* Functions */
static int _create_gui(void);

/* Callbacks */
static int _idle_timer_cb(void *data)
{
    if (enna_mediaplayer_state_get() == PLAYING)
    {
        enna_log(ENNA_MSG_INFO, NULL, "still playing, renewing idle timer");
        return ECORE_CALLBACK_RENEW;
    }
    else if (enna_activity_request_quit_all())
    {
        enna_log(ENNA_MSG_INFO, NULL,
                 "at least one activity's busy, renewing idle timer");
        return ECORE_CALLBACK_RENEW;
    }

    if (enna_mainmenu_exit_visible())
    {
        enna_log(ENNA_MSG_INFO, NULL, "gracetime is over, quitting enna.");
        ecore_main_loop_quit();
    }
    else
    {
        enna_log(ENNA_MSG_INFO, NULL,
                 "enna seems to be idle, sending quit msg and waiting 20s");
        evas_event_feed_key_down(enna->evas,
                                 "Escape",
                                 "Escape",
                                 "Escape",
                                 NULL,
                                 ecore_time_get(),
                                 NULL);
        ecore_timer_interval_set(enna->idle_timer, 20);
    }

    return ECORE_CALLBACK_RENEW;
}

static int _mouse_idle_timer_cb(void *data)
{
    Evas_Object *cursor = (Evas_Object*)data;
    edje_object_signal_emit(cursor, "cursor,hide", "enna");
    enna->cursor_is_shown=0;
    enna_log(ENNA_MSG_EVENT, NULL, "hiding cursor.");
    ENNA_TIMER_DEL(enna->mouse_idle_timer);
    return ECORE_CALLBACK_CANCEL;
}

void _mousemove_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    if (!enna->cursor_is_shown)
    {
        edje_object_signal_emit(obj, "cursor,show", "enna");
        enna->cursor_is_shown=1;
        enna_log(ENNA_MSG_EVENT, NULL, "unhiding cursor.",
                 evas_object_visible_get(obj));
        enna_idle_timer_renew();
    }
    ENNA_TIMER_DEL(enna->mouse_idle_timer);
    if (enna->mouse_idle_timer)
        ecore_timer_interval_set(enna->mouse_idle_timer,
                                 ENNA_MOUSE_IDLE_TIMEOUT);
    else
        enna->mouse_idle_timer = ecore_timer_add(ENNA_MOUSE_IDLE_TIMEOUT,
                                                 _mouse_idle_timer_cb,
                                                 obj);
}

static void
_set_scale(int h)
{
    double scale = 1.0;
    if (h)
        scale = (h / 720.0);
    if (scale >= 1.0)
        scale = 1.0;
    elm_scale_set(scale);

    enna_log(ENNA_MSG_INFO, NULL, "Scale: %3.3f", scale);
}


static void _window_delete_cb(void *data, Evas_Object *obj, void *event_info)
{
    ecore_main_loop_quit();
}

static void
_window_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Coord h;

    evas_object_geometry_get(enna->win, NULL, NULL, NULL, &h);
    _set_scale(h);
}

static void _list_engines(void)
{
    Eina_List  *lst;
    Eina_List  *n;
    const char *engine;

    enna_log(ENNA_MSG_INFO, NULL, "Supported engines:");

    lst = ecore_evas_engines_get();

    EINA_LIST_FOREACH(lst, n, engine)
    {
        if (strcmp(engine, "buffer") != 0)
            enna_log(ENNA_MSG_INFO, NULL, "\t* %s", engine);
    }

    ecore_evas_engines_free(lst);
}

/* Functions */

static const struct {
    const char *name;
    enna_msg_level_t lvl;
} msg_level_mapping[] = {
    { "none",       ENNA_MSG_NONE     },
    { "event",      ENNA_MSG_EVENT    },
    { "info",       ENNA_MSG_INFO     },
    { "warning",    ENNA_MSG_WARNING  },
    { "error",      ENNA_MSG_ERROR    },
    { "critical",   ENNA_MSG_CRITICAL },
    { NULL,         0                 }
};

static int _enna_init(void)
{
    char tmp[PATH_MAX];
    int i;
    Eina_List *l;
    Enna_Class_Activity *a;

    enna->lvl = ENNA_MSG_INFO;

    enna_module_init();

    snprintf(tmp, sizeof(tmp), "%s/.enna", enna_util_user_home_get());

    if (!ecore_file_exists(tmp))
        ecore_file_mkdir(tmp);

    snprintf(tmp, sizeof(tmp), "%s/.enna/covers", enna_util_user_home_get());
    if (!ecore_file_exists(tmp))
        ecore_file_mkdir(tmp);

    if (enna_config->verbosity)
    {
        for (i = 0; msg_level_mapping[i].name; i++)
            if (!strcmp (enna_config->verbosity, msg_level_mapping[i].name))
            {
                enna->lvl = msg_level_mapping[i].lvl;
                break;
            }
    }

    enna->slideshow_delay = enna_config->slideshow_delay;


    /* Create ecore events (we should put here ALL the event_type_new) */
    ENNA_EVENT_ACTIVITIES_CHANGED = ecore_event_type_new();


    if (!_create_gui())
        return 0;

    /* Init various stuff */
    enna_metadata_init ();

    if (!enna_mediaplayer_init())
        return 0;

    /* Load available modules */
    enna_module_load_all();

    /* Dinamically init activities */
    EINA_LIST_FOREACH(enna_activities_get(), l, a)
        enna_activity_init(a->name);

    /* Show mainmenu */
    //~ enna_mainmenu_select_nth(0);
    enna_mainmenu_show();

    enna->idle_timer = NULL;
    enna_idle_timer_renew();

    enna_ipc_init();

    return 1;
}

static int _create_gui(void)
{
    // set custom elementary theme
    elm_theme_extension_add(enna_config_theme_get());

    // show supported engines
    _list_engines();
    enna_log(ENNA_MSG_INFO, NULL, "Using engine: %s", enna_config->engine); //FIXME
    //~ ENNA_FREE(enna_config->engine);
    //~ enna_config->engine=strdup(ecore_evas_engine_name_get(enna->ee));

    // main window
    enna->win = elm_win_add(NULL, "enna", ELM_WIN_BASIC);
    if (!enna->win)
      return 0;
    elm_win_title_set(enna->win, "Enna MediaCenter");
    enna->run_fullscreen = enna_config->fullscreen | run_fullscreen;
    elm_win_fullscreen_set(enna->win, enna->run_fullscreen);
    evas_object_smart_callback_add(enna->win, "delete-request",
                                   _window_delete_cb, NULL);
    evas_object_event_callback_add(enna->win, EVAS_CALLBACK_RESIZE,
                                   _window_resize_cb, NULL);
    //~ ecore_evas_shaped_set(enna->ee, 1);  //TODO why this ???
    enna->ee_winid = elm_win_xwindow_get(enna->win);
    enna->evas = evas_object_evas_get(enna->win);

    // main layout widget
    enna->layout = elm_layout_add(enna->win);
    elm_layout_file_set(enna->layout, enna_config_theme_get(), "main_layout");
    evas_object_size_hint_weight_set(enna->layout, 1.0, 1.0);
    elm_win_resize_object_add(enna->win, enna->layout);
    evas_object_show(enna->layout);

    // mainmenu
    enna_mainmenu_init();

    // exit dialog
    elm_layout_content_set(enna->layout, "enna.exit.swallow",
                           enna_exit_add(enna->evas));

    // mouse pointer
    enna->o_cursor = edje_object_add(enna->evas);
    edje_object_file_set(enna->o_cursor,
                         enna_config_theme_get(),
                         "enna/cursor");
    // hot_x/hot_y are about 4px/3px in original image which is scaled by 1.5
    /* Comment until Dave patch elementary */
    /*elm_win_cursor_set(enna->win, enna->o_cursor, 9, 6);*/
    evas_object_show(enna->o_cursor);
    enna->mouse_idle_timer = ecore_timer_add(ENNA_MOUSE_IDLE_TIMEOUT,
                                             _mouse_idle_timer_cb,
                                             enna->o_cursor);
    evas_object_event_callback_add(enna->o_cursor,
                                   EVAS_CALLBACK_MOVE,
                                   _mousemove_cb,
                                   NULL);
    enna->cursor_is_shown=1;
    
    // show all
    evas_object_resize(enna->win, app_w, app_h);
    _set_scale(app_h);
    evas_object_show(enna->win);

    return 1;
}

static void _enna_shutdown(void)
{
    ENNA_TIMER_DEL(enna->idle_timer);
    ENNA_TIMER_DEL(enna->mouse_idle_timer);

    enna_activity_del_all();
    enna_config_shutdown();
    enna_module_shutdown();
    enna_metadata_shutdown();
    enna_mediaplayer_shutdown();

    evas_object_del(enna->o_background);
    enna_mainmenu_shutdown();
    evas_object_del(enna->o_content);

    enna_ipc_shutdown();
    elm_shutdown();
    enna_log(ENNA_MSG_INFO, NULL, "Bye Bye !");
    enna_log_shutdown();
    ENNA_FREE(enna);
}

static void _opt_geometry_parse(const char *optarg,
                                unsigned int *pw, unsigned int *ph)
{
    int w = 0, h = 0;

    if (sscanf(optarg, "%dx%d", &w, &h) != 2)
        return;

    if (pw) *pw = w;
    if (ph) *ph = h;
}

void enna_idle_timer_renew(void)
{
    if (enna_config->idle_timeout)
    {
        if (enna->idle_timer)
        {
            ENNA_TIMER_DEL(enna->idle_timer)
        }
        else
        {
            enna_log(ENNA_MSG_INFO, NULL,
                     "setting up idle timer to %i minutes",
                     enna_config->idle_timeout);
        }
        if (!(enna->idle_timer = ecore_timer_add(enna_config->idle_timeout*60,
                                                 _idle_timer_cb, NULL)))
        {
            enna_log(ENNA_MSG_CRITICAL, NULL, "adding timer failed!");
        }
    }
}

static int exit_signal(void *data, int type, void *e)
{
    Ecore_Event_Signal_Exit *event = e;

    fprintf(stderr,
            "Enna got exit signal [interrupt=%u, quit=%u, terminate=%u]\n",
            event->interrupt, event->quit, event->terminate);

    ecore_main_loop_quit();
    return 1;
}


static void usage(char *binname)
{
    printf(_("Enna MediaCenter\n"));
    printf(_(" Usage: %s [options ...]\n"), binname);
    printf(_(" Available Options:\n"));
    printf(_("  -c, (--config):  Specify configuration file to be used.\n"));
    printf(_("  -f, (--fs):      Force Fullscreen mode.\n"));
    printf(_("  -h, (--help):    Display this help.\n"));
    printf(_("  -t, (--theme):   Specify theme name to be used.\n"));
    printf(_("  -g, (--geometry):Specify window geometry. (geometry=1280x720)\n"));
    printf(_("  -V, (--version): Display Enna version number.\n"));
    exit(EXIT_SUCCESS);
}

static int parse_command_line(int argc, char **argv)
{
    int c, index;
    char short_options[] = "Vhfc:t:b:g:";
    struct option long_options [] =
        {
            { "help",          no_argument,       0, 'h' },
            { "version",       no_argument,       0, 'V' },
            { "fs",            no_argument,       0, 'f' },
            { "config",        required_argument, 0, 'c' },
            { "theme",         required_argument, 0, 't' },
            { "geometry",      required_argument, 0, 'g' },
            { 0,               0,                 0,  0  }
        };

    /* command line argument processing */
    while (1)
    {
        c = getopt_long(argc, argv, short_options, long_options, &index);

        if (c == EOF)
            break;

        switch (c)
        {
        case 0:
            /* opt = long_options[index].name; */
            break;

        case '?':
        case 'h':
            usage(argv[0]);
        return -1;

        case 'V':
            break;

        case 'f':
            run_fullscreen = 1;
            break;

        case 'c':
            conffile = strdup(optarg);
            break;

        case 't':
            theme_name = strdup(optarg);
            break;
        case 'g':
            _opt_geometry_parse(optarg, &app_w, &app_h);
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    int res = EXIT_FAILURE;

    init_locale();

    if (parse_command_line(argc, argv) < 0)
        return EXIT_SUCCESS;

    url_global_init();
    eina_init();

    /* Must be called first */
    enna_config_init(conffile);
    ENNA_FREE(conffile);
    
    setenv("ELM_ENGINE", enna_config->engine, 1);
    elm_init(argc, argv);
    
    enna_log(ENNA_MSG_INFO, NULL, "enna log file : %s\n",
             enna_config->log_file);
    enna_log_init(enna_config->log_file);
    enna = calloc(1, sizeof(Enna));

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_signal, enna);

    if (!_enna_init())
        goto out;

    ecore_main_loop_begin();

    _enna_shutdown();
    res = EXIT_SUCCESS;

 out:
    url_global_uninit();
    return res;
}

