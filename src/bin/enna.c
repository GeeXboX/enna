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

#include "config.h"

#define _GNU_SOURCE
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>

#include "enna.h"
#include "enna_config.h"
#include "enna_config_main.h"
#include "enna_gui.h"
#include "utils.h"
#include "activity_priv.h"
#include "module_priv.h"
#include "exit.h"
#include "logs.h"
#include "metadata.h"
#include "mediaplayer.h"
#include "url_utils.h"
#include "geoip.h"
#include "gadgets.h"


struct cmdline_config_s
{
    const char *cfg_file;
    const char *profile;
    const char *theme;

    unsigned int app_width;
    unsigned int app_height;
    unsigned int app_x_off;
    unsigned int app_y_off;

    int fullscreen;
};

// FIXME: Set to ENNA_MSG_WARNING for production version
#define DEFAULT_MSG_LEVEL     (ENNA_MSG_INFO)


/* Global Variable Enna *enna*/
Enna *enna;

/* Functions */

/* Callbacks */
static Eina_Bool
_idle_timer_cb(void *data __UNUSED__)
{

    if (enna_exit_visible())
    {
        enna_log(ENNA_MSG_INFO, NULL, "gracetime is over, quitting enna.");
        ecore_main_loop_quit();
    }
    else
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
      else
      {
        enna_log(ENNA_MSG_INFO, NULL,
                 "enna seems to be idle, sending quit msg and waiting 20s");
        enna_input_event_emit(ENNA_INPUT_QUIT);
        enna->idle_timer = ecore_timer_add(20, _idle_timer_cb, NULL);
      }
    }
    return ECORE_CALLBACK_CANCEL;
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



static Eina_Bool
exit_signal(void *data __UNUSED__, int type __UNUSED__, void *e)
{
    Ecore_Event_Signal_Exit *event = e;

    fprintf(stderr,
            "Enna got exit signal [interrupt=%u, quit=%u, terminate=%u]\n",
            event->interrupt, event->quit, event->terminate);

    ecore_main_loop_quit();
    return 1;
}


static void usage(const char *binname)
{
    printf(_("Enna MediaCenter\n"));
    printf(_(" Usage: %s [options ...]\n"), binname);
    printf(_(" Available options:\n"));
    printf(_("  -c, (--config):  Specify configuration file to be used.\n"));
    printf(_("  -f, (--fs):      Force fullscreen mode.\n"));
    printf(_("  -h, (--help):    Display this help.\n"));
    printf(_("  -t, (--theme):   Specify theme name to be used.\n"));
    printf(_("  -g, (--geometry):Specify window geometry. (geometry=1280x720)\n"));
    printf(_("  -g, (--geometry):Specify window geometry and offset. (geometry=1280x720:10:20)\n"));
    printf(_("  -p, (--profile): Specify display profile (cannot be used with theme,geometry)\n"));
    printf(_("    Supported: "));
    list_profiles();
    printf(_("  -V, (--version): Display Enna version number.\n"));

    exit(EXIT_SUCCESS);
}

static void version(void)
{
    printf(PACKAGE_STRING"\n");
    exit(EXIT_SUCCESS);
}

static void _opt_geometry_parse(const char *optarg,
                                unsigned int *pw, unsigned int *ph, unsigned int *px, unsigned int *py)
{
    int w = 0, h = 0;
    int x = 0, y = 0;
    int ret;

    ret = sscanf(optarg, "%dx%d:%d:%d", &w, &h, &x, &y);

    if ( ret != 2 && ret != 4 )
        return;

    if (pw) *pw = w;
    if (ph) *ph = h;
    if (px) *px = x;
    if (py) *py = y;
}

static int parse_command_line(int argc, char **argv, struct cmdline_config_s *cmd_config)
{
    int c, index;
    char short_options[] = "Vhfc:t:b:g:p:";
    struct option long_options [] =
        {
            { "help",          no_argument,       0, 'h' },
            { "version",       no_argument,       0, 'V' },
            { "fs",            no_argument,       0, 'f' },
            { "config",        required_argument, 0, 'c' },
            { "theme",         required_argument, 0, 't' },
            { "geometry",      required_argument, 0, 'g' },
            { "profile",       required_argument, 0, 'p' },
            { 0,               0,                 0,  0  }
        };

    const char *progname = argv[0];

    // init_cmd_config
    memset(cmd_config, 0, sizeof(*cmd_config));

    int profile_mode = 0; // 0 = UNSET; 1=use theme,geom;  2=use profile

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
            usage(progname);
        return -1;

        case 'V':
            version();
            break;

        case 'f':
	    cmd_config->fullscreen = 1;
            break;

        case 'c':
	    cmd_config->cfg_file = strdup(optarg);
            break;

        case 't':
	    if (profile_mode == 2) { usage(progname); }
	    cmd_config->theme = strdup(optarg);
	    profile_mode = 1;
            break;

        case 'g':
	    if (profile_mode == 2) { usage(progname); }
            _opt_geometry_parse(optarg, &cmd_config->app_width, &cmd_config->app_height,
					&cmd_config->app_x_off, &cmd_config->app_y_off);
	    profile_mode = 1;
            break;

        case 'p':
	    if (profile_mode == 1) { usage(progname); }
	    cmd_config->profile = strdup(optarg);
	    profile_mode = 2;
	    if (!config_profile_parse(optarg, &cmd_config->theme,
		&cmd_config->app_width, &cmd_config->app_height))
			{ usage(progname); }
            break;

        default:
            usage(progname);
            return -1;
        }
    }

    return 0;
}

void resolve_config_overrides(struct cmdline_config_s *cmd_config)
{
    enna_config->fullscreen |= cmd_config->fullscreen;

    // profile -> theme, width, height
    // OR we have theme,wid,height but no profile

    if (cmd_config->profile)
    {
        ENNA_FREE(enna_config->profile);
        enna_config->profile = strdup(cmd_config->profile);
    }

    if (cmd_config->theme)
    {
        ENNA_FREE(enna_config->theme);
        enna_config->theme = strdup(cmd_config->theme);
    }

    if (cmd_config->app_width >0) enna_config->app_width = cmd_config->app_width;
    if (cmd_config->app_height>0) enna_config->app_height= cmd_config->app_height;
    if (cmd_config->app_x_off >0) enna_config->app_x_off = cmd_config->app_x_off;
    if (cmd_config->app_y_off >0) enna_config->app_y_off = cmd_config->app_y_off;

    enna_log(ENNA_MSG_INFO, NULL, "enna profile : %s", enna_config->profile);
    enna_log(ENNA_MSG_INFO, NULL, "enna theme:%s, width=%u, height=%u, x_off=%u, y_off=%u",
	enna_config->theme, enna_config->app_width, enna_config->app_height,
	enna_config->app_x_off, enna_config->app_y_off);
}


static void
enna_load_theme(void)
{
    if (!enna_config->theme)
        goto err_theme;

    assert(enna_config->theme_file == NULL);

    enna_config->theme_file = (char *)
        enna_config_resolve_theme_file(enna_config->theme);

    if (!enna_config->theme_file)
        goto err_theme;

    enna_log(ENNA_MSG_INFO, NULL, "enna theme file : %s", enna_config->theme_file);

    enna_config->eth = elm_theme_new();
    assert(enna_config->eth);

    elm_theme_overlay_add(enna_config->eth, enna_config->theme_file);

    // set custom elementary theme
    elm_theme_extension_add(enna_config->eth, enna_config->theme_file);

    return;

err_theme:
      enna_log(ENNA_MSG_CRITICAL, NULL, "couldn't load theme file!");
}


static int _enna_init(int argc, char **argv)
{
    struct cmdline_config_s cmd_config;

    // Note: all logging up to enna_log_init() will go to stderr

    if (parse_command_line(argc, argv, &cmd_config) < 0)
        return EXIT_SUCCESS;

    enna = calloc(1, sizeof(Enna));
    enna->lvl = DEFAULT_MSG_LEVEL;

    enna_util_init();

    enna_config_load(cmd_config.cfg_file);
    resolve_config_overrides(&cmd_config);

    enna_main_cfg_register(); // triggers default + parsing for this section

    // enna_config valid

    enna_log(ENNA_MSG_INFO, NULL, "enna log file : %s", enna_config->log_file);
    enna_log_init(enna_config->log_file);
    enna->lvl = enna_util_get_log_level(enna_config->verbosity);

    /* ecore events of event_type_new should go about here */

    url_global_init(); // should be done before modules load and geoip
    enna_get_geo_by_ip(); /* try to geolocate */
    enna_browser_init();
    enna_metadata_init();
    if (!enna_mediaplayer_init())
        return 0;

    enna_module_init();
    enna_module_load_all();  /* Load available modules */
    enna_gadgets_init();

    enna_load_theme();

    ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_signal, enna);

    Evas_Object *mainwindow = enna_gui_init();
    if (!mainwindow)
	return 0;

    enna_create_gui(mainwindow);

    enna_activity_init_all();
    enna_gadgets_show();

    if (enna_config->display_mouse == EINA_TRUE)
    {
        enna->idle_timer = NULL;
        enna_idle_timer_renew();
    }

    return 1;
}


static void _enna_shutdown(void)
{
    ENNA_TIMER_DEL(enna->idle_timer);

#ifdef BUILD_ECORE_X
    // KRL: put into gui shutdown !?
    ENNA_TIMER_DEL(enna->mouse_idle_timer);
    ENNA_EVENT_HANDLER_DEL(enna->mouse_handler);
#endif
    enna_geo_free(enna->geo_loc);

    enna_activity_del_all();
    enna_gadgets_shutdown();
    enna_module_shutdown();
    enna_metadata_shutdown();
    enna_mediaplayer_shutdown();

    // KRL: FIXME, has enna_weather_shutdown() been called !?

    url_global_uninit();

    evas_object_del(enna->o_background);
    evas_object_del(enna->o_menu);
    evas_object_del(enna->o_content);

    enna_exit_shutdown();
    elm_theme_free(enna_config->eth);
    enna_util_shutdown();
    enna_log(ENNA_MSG_INFO, NULL, "Bye Bye !");
    enna_log_shutdown();
    enna_config_shutdown();

    ENNA_FREE(enna);
}


int main(int argc, char **argv)
{
    int res = EXIT_FAILURE;

    init_locale();
    eina_init();
    ecore_init();
    elm_init(argc, argv);

    if (_enna_init(argc, argv))
    {
        ecore_main_loop_begin();
        res = EXIT_SUCCESS;
    }

    _enna_shutdown();

    elm_shutdown();
    ecore_shutdown();
    eina_shutdown();

    return res;
}
