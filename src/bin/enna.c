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
#include "background.h"
#include "module.h"
#include "content.h"
#include "event_key.h"
#include "logs.h"
#include "volumes.h"
#include "metadata.h"
#include "mediaplayer.h"
#include "ipc.h"

#define ENNA_MOUSE_IDLE_TIMEOUT 10 //seconds after which the mouse pointer disappears

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
        enna_log(ENNA_MSG_INFO, NULL, "at least one activity's busy, renewing idle timer");
        return ECORE_CALLBACK_RENEW;
    }

    if (enna_mainmenu_exit_visible(enna->o_mainmenu))
    {
        enna_log(ENNA_MSG_INFO, NULL, "gracetime is over, quitting enna.");
        ecore_main_loop_quit();
    }
    else
    {
        enna_log(ENNA_MSG_INFO, NULL, "enna seems to be idle, sending quit msg and waiting 20s");
        evas_event_feed_key_down(enna->evas, "Escape", "Escape", "Escape", NULL, ecore_time_get(), NULL);
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
        enna_log(ENNA_MSG_EVENT, NULL, "unhiding cursor.", evas_object_visible_get(obj));
        enna_idle_timer_renew();
    }
    ENNA_TIMER_DEL(enna->mouse_idle_timer);
    if (enna->mouse_idle_timer)
      ecore_timer_interval_set(enna->mouse_idle_timer, ENNA_MOUSE_IDLE_TIMEOUT);
    else
      enna->mouse_idle_timer = ecore_timer_add(ENNA_MOUSE_IDLE_TIMEOUT, _mouse_idle_timer_cb, obj);
}

static void _event_bg_key_down_cb(void *data, Evas *e,
    Evas_Object *obj, void *event)
{
    Enna *enna;
    enna_key_t key;

    enna_idle_timer_renew();

    key = enna_get_key(event);

    enna = (Enna *) data;
    if (!enna)
        return;

    if (key == ENNA_KEY_FULLSCREEN)
    {
        run_fullscreen = ~run_fullscreen;
        ecore_evas_fullscreen_set(enna->ee, run_fullscreen);
    }

    if (enna_mainmenu_exit_visible(enna->o_mainmenu) || key == ENNA_KEY_QUIT)
    {
        enna_mainmenu_event_feed(enna->o_mainmenu, event);
    }
    else if (enna_mainmenu_visible(enna->o_mainmenu))
    {
        switch (key)
        {
        case ENNA_KEY_MENU:
        {
            enna_content_show();
            enna_mainmenu_hide(enna->o_mainmenu);
            edje_object_signal_emit(enna->o_edje, "mainmenu,hide", "enna");
            break;
        }
        default:
            enna_mainmenu_event_feed(enna->o_mainmenu, event);
            break;
        }
    }
    else
    {
        switch (key)
        {
        case ENNA_KEY_MENU:
        {
            enna_content_hide();
            enna_mainmenu_show(enna->o_mainmenu);
            break;
        }
        default:
            enna_activity_event(enna_mainmenu_selected_activity_get(enna->o_mainmenu), event);
            break;
        }
    }
}

static void _resize_viewport_cb(Ecore_Evas * ee)
{
    Evas_Coord w, h, x, y;

    if (!enna->ee)
        return;

    evas_output_viewport_get(enna->evas, &x, &y, &w, &h);
    evas_object_resize(enna->o_edje, w, h);
    evas_object_move(enna->o_edje, x, y);
    ecore_evas_resize(enna->ee, w, h);
}

static void _cb_delete(Ecore_Evas *ee)
{
    ecore_main_loop_quit();
}

static void _list_engines(void)
{
    Eina_List  *lst;
    Eina_List  *n;
    const char *engine;

    enna_log(ENNA_MSG_CRITICAL, NULL, "supported engines:");

    lst = ecore_evas_engines_get();

    EINA_LIST_FOREACH(lst, n, engine)
    {
        if (strcmp(engine, "buffer") != 0)
            enna_log(ENNA_MSG_CRITICAL, NULL, "\t*%s", engine);
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


static void
pipe_grabber_read (void *data, void *buf, unsigned int nbyte)
{
  Enna_Class_Activity *a = NULL;


  a = enna_mainmenu_selected_activity_get(enna->o_mainmenu);
  if (!a)
    return;

  if (a->func.class_grabbing_finished)
    a->func.class_grabbing_finished(buf);

}



static int _enna_init(void)
{
    char tmp[PATH_MAX];
    int i;

    enna->lvl = ENNA_MSG_INFO;
    enna->home = enna_util_user_home_get();

    enna_module_init();

    snprintf(tmp, sizeof(tmp), "%s/.enna", enna->home);

    if (!ecore_file_exists(tmp))
        ecore_file_mkdir(tmp);

    snprintf(tmp, sizeof(tmp), "%s/.enna/covers", enna->home);
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

    enna->ee = ecore_evas_new(enna_config->engine, 0, 0, 1, 1, NULL);

    if (!enna->ee)
    {
        enna_log(ENNA_MSG_CRITICAL, NULL,
            "Can not create Ecore Evas with %s engine!",
            enna_config->engine);
        _list_engines();
        return 0;
    }

    if (ecore_str_has_extension(enna_config->engine, "_x11"))
        enna->ee_winid = (Ecore_X_Window) ecore_evas_window_get(enna->ee);

    enna->use_network = enna_config->use_network;
    enna->use_covers = enna_config->use_covers;
    enna->use_snapshots = enna_config->use_snapshots;
    enna->metadata_cache = enna_config->metadata_cache;

    ecore_evas_fullscreen_set(enna->ee, enna_config->fullscreen
        | run_fullscreen);

    ecore_evas_title_set(enna->ee, _("enna HTPC"));
    ecore_evas_name_class_set(enna->ee, "enna", "enna");
    ecore_evas_borderless_set(enna->ee, 0);
    ecore_evas_shaped_set(enna->ee, 1);
    enna->evas = ecore_evas_get(enna->ee);

    evas_data_attach_set(enna->evas, enna);

    if (!_create_gui())
        return 0;

    ecore_evas_show(enna->ee);
    enna_input_init();
    enna_ipc_init();
    grabber_start_thread();
    enna->pipe_grabber = ecore_pipe_add (pipe_grabber_read, NULL);

    return 1;
}


static int _create_gui(void)
{
    Evas_Object *o;

    o = edje_object_add(enna->evas);
    edje_object_file_set(o, enna_config_theme_get(), "enna");
    elm_theme_extension_add(enna_config_theme_get());
    evas_object_resize(o, app_w, app_h);
    evas_object_move(o, 0, 0);
    evas_object_show(o);
    ecore_evas_resize(enna->ee, app_w, app_h);

    enna->o_edje = o;

    /* Create Background Object */
    o = enna_background_add(enna->evas);
    edje_object_part_swallow(enna->o_edje, "enna.swallow.background", o);
    enna->o_background = o;
    /* Create Mainmenu Object */
    o = enna_mainmenu_add(enna->evas);
    edje_object_part_swallow(enna->o_edje, "enna.swallow.mainmenu", o);
    enna->o_mainmenu = o;

    edje_object_signal_emit(enna->o_edje, "mainmenu,show", "enna");
    evas_object_focus_set(enna->o_edje, 1);

    evas_object_event_callback_add(enna->o_edje, EVAS_CALLBACK_KEY_DOWN, _event_bg_key_down_cb, enna);

    ecore_evas_callback_resize_set(enna->ee, _resize_viewport_cb);
    ecore_evas_callback_delete_request_set(enna->ee, _cb_delete);
    /* Create Content Object */
    o = enna_content_add(enna->evas);
    edje_object_part_swallow(enna->o_edje, "enna.swallow.module", o);
    enna->o_content = o;

    /* Init various stuff */
    enna_volumes_init();
    enna_metadata_init ();
    if (!enna_mediaplayer_init())
        return 0;

    /* Load available modules */
    enna_module_load_all(enna->evas);

    /* Load mainmenu items */
#ifdef BUILD_ACTIVITY_INFOS
    enna_activity_init("infos");
#endif
#ifdef BUILD_ACTIVITY_MUSIC
    enna_activity_init("music");
#endif
#if defined(BUILD_ACTIVITY_TV)
    enna_activity_init("tv");
#endif
#ifdef BUILD_ACTIVITY_VIDEO
    enna_activity_init("video");
#endif
#ifdef BUILD_ACTIVITY_PHOTO
    enna_activity_init("photo");
#endif
#if defined(BUILD_ACTIVITY_GAMES) && defined(BUILD_EFREET)
    enna_activity_init("games");
#endif
#ifdef BUILD_ACTIVITY_WEATHER
    enna_activity_init("weather");
#endif

    enna_mainmenu_load_from_activities(enna->o_mainmenu);
    enna_mainmenu_select_nth(enna->o_mainmenu, 0);

    enna_content_hide();
    enna_mainmenu_show(enna->o_mainmenu);

    ecore_evas_show(enna->ee);

    enna->idle_timer = NULL;
    enna_idle_timer_renew();

    o = edje_object_add(enna->evas);
    edje_object_file_set(o, enna_config_theme_get(), "enna/mainmenu/cursor");
    // hot_x/hot_y are about 4px/3px in original image which is scaled by 1.5
    ecore_evas_object_cursor_set(enna->ee, o, 1, 9, 6);
    evas_object_show(o);
    enna->mouse_idle_timer = ecore_timer_add(ENNA_MOUSE_IDLE_TIMEOUT, _mouse_idle_timer_cb, o);
    evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _mousemove_cb, NULL);
    enna->cursor_is_shown=1;
    enna->o_cursor = o;

    return 1;
}

static void _enna_shutdown(void)
{
    ENNA_TIMER_DEL(enna->idle_timer);
    ENNA_TIMER_DEL(enna->mouse_idle_timer);

    enna_activity_del_all ();
    enna_input_shutdown();
    enna_config_shutdown();
    enna_module_shutdown();
    enna_mediaplayer_shutdown();
    evas_object_del(enna->o_background);
    evas_object_del(enna->o_edje);
    evas_object_del(enna->o_mainmenu);
    edje_shutdown();
    ecore_file_shutdown();
    ecore_evas_shutdown();
    enna_ipc_shutdown();
    ENNA_FREE(enna->home);
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
        if (enna->idle_timer) { ENNA_TIMER_DEL(enna->idle_timer) }
        else
            enna_log(ENNA_MSG_INFO, NULL, "setting up idle timer to %i minutes", enna_config->idle_timeout);
        if (!(enna->idle_timer = ecore_timer_add(enna_config->idle_timeout*60, _idle_timer_cb, NULL)))
            enna_log(ENNA_MSG_CRITICAL, NULL, "adding timer failed!");
    }
}

static int exit_signal(void *data, int type, void *e)
{
    Ecore_Event_Signal_Exit *event = e;

    fprintf(stderr, "Enna got exit signal [interrupt=%u, quit=%u, terminate=%u]\n",
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
    init_locale();

    if (parse_command_line(argc, argv) < 0)
        return EXIT_SUCCESS;

    elm_init(argc, argv);

    /* Must be called first */
    enna_config_init();
    enna_log(ENNA_MSG_INFO, NULL, "enna log file : %s\n", enna_config->log_file);
    enna_log_init(enna_config->log_file);
    enna = calloc(1, sizeof(Enna));

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_signal, enna);

    if (!_enna_init())
        return EXIT_FAILURE;

    ecore_main_loop_begin();

    _enna_shutdown();
    enna_log(ENNA_MSG_INFO, NULL, "Bye Bye !\n");
    enna_log_shutdown();
    return EXIT_SUCCESS;
}

