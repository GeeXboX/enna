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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "logs.h"
#include "content.h"
#include "mainmenu.h"
#include "mediaplayer.h"

#ifdef BUILD_LIBSVDRP
#include <time.h>
#include "utils.h"
#endif /* BUILD_LIBSVDRP */

#define ENNA_MODULE_NAME "tv"

typedef struct _Enna_Module_Tv
{
    Evas *e;
    Evas_Object *o_layout;
    Evas_Object *edje;
    Enna_Playlist *enna_playlist;
    Enna_Module *em;
#ifdef BUILD_LIBSVDRP
    svdrp_t *svdrp;
    int timer_threshold;
#endif /* BUILD_LIBSVDRP */
} Enna_Module_Tv;

typedef struct tv_cfg_s
{
    char *vdr_uri;
#ifdef BUILD_LIBSVDRP
    char *svdrp_host;
    int svdrp_port;
    int svdrp_timeout;
    svdrp_verbosity_level_t svdrp_verb;
    int timer_threshold;
#endif /* BUILD_LIBSVDRP */
} tv_cfg_t;

static tv_cfg_t tv_cfg;
static Enna_Module_Tv *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _class_init(void)
{
    mod->o_layout = elm_layout_add(enna->layout);
    elm_layout_file_set(mod->o_layout, enna_config_theme_get (), "activity/tv");
    enna_content_append (ENNA_MODULE_NAME, mod->o_layout);
}

static const char *
_class_quit_request(void)
{
#ifdef BUILD_LIBSVDRP
    int ret;
    int timer_id;
    time_t start, now, diff;

    ret = svdrp_next_timer_event(mod->svdrp, &timer_id, &start);

    if (ret != SVDRP_OK)
        return NULL;

    if (time(&now) == ((time_t) -1))
        return NULL;

    diff = (start - now) / 60;

    if (diff <= mod->timer_threshold) /* recording in progess */
    {
        svdrp_timer_t timer;

        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "recording in progress");

        if (svdrp_get_timer(mod->svdrp, timer_id, &timer) == SVDRP_OK)
        {
            const char *format =
                diff > 0 ? ngettext("Timer '%s' is due to start in %i minute",
                                    "Timer '%s' is due to start in %i minutes",
                                    diff)
                         : _("Currently recording '%s'");
            size_t len = strlen(format) + strlen(timer.file) - 1;
            char *msg = malloc (len);

            snprintf(msg, len, format, timer.file, diff);

            return msg;
        }
        else
            return strdup (_("Recording in progress"));
    }
#endif /* BUILD_LIBSVDRP */

    return NULL;
}

static void
_class_show(void)
{
    enna_content_select(ENNA_MODULE_NAME);
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "starting playback");
    enna_mediaplayer_play(mod->enna_playlist);
    edje_object_signal_emit(elm_layout_edje_get(mod->o_layout),
                            "tv,show", "enna");
}

static void
_class_hide(void)
{
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "stopping playback");
    enna_mediaplayer_stop();
    edje_object_signal_emit(elm_layout_edje_get(mod->o_layout),
                            "tv,hide", "enna");
}

static void
_class_event(enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_HOME:
    case ENNA_INPUT_QUIT:
        enna_content_hide();
        break;
    default:
        enna_mediaplayer_send_input(event);
        break;
    }
}

static Enna_Class_Activity class =
{
    ENNA_MODULE_NAME,
    2,
    N_("Television"),
    NULL,
    "icon/dev/tv",
    "background/tv",
    ENNA_CAPS_NONE,
    {
    _class_init,
    _class_quit_request,
    NULL,
    _class_show,
    _class_hide,
    _class_event
    }
};

#ifdef BUILD_LIBSVDRP
static const struct {
    const char *name;
    svdrp_verbosity_level_t verb;
} map_svdrp_verbosity[] = {
    { "none",        SVDRP_MSG_NONE        },
    { "verbose",     SVDRP_MSG_VERBOSE     },
    { "info",        SVDRP_MSG_INFO        },
    { "warning",     SVDRP_MSG_WARNING     },
    { "error",       SVDRP_MSG_ERROR       },
    { "critical",    SVDRP_MSG_CRITICAL    },
    { NULL,          SVDRP_MSG_NONE        }
};
#endif /* BUILD_LIBSVDRP */

static void
cfg_tv_section_load(const char *section)
{
    const char *value = NULL;
#ifdef BUILD_LIBSVDRP
    int v;
#endif /* BUILD_LIBSVDRP */

    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "parameters:");

    value = enna_config_string_get(section, "vdr_uri");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, " * vdr_uri: %s", value);
        ENNA_FREE(tv_cfg.vdr_uri);
        tv_cfg.vdr_uri = strdup(value);
    }

#ifdef BUILD_LIBSVDRP
    v = enna_config_int_get(section, "svdrp_port");
    if (v)
        tv_cfg.svdrp_port = v;

    v = enna_config_int_get(section, "svdrp_timeout");
    if (v)
        tv_cfg.svdrp_timeout = v;

    value = enna_config_string_get(section, "svdrp_verbosity");
    if (value)
    {
        int i;

        for (i = 0; map_svdrp_verbosity[i].name; i++)
            if (!strcmp(value, map_svdrp_verbosity[i].name))
            {
                tv_cfg.svdrp_verb = map_svdrp_verbosity[i].verb;
                break;
            }
    }

    v = enna_config_int_get(section, "timer_quit_threshold");
    if (v)
        tv_cfg.timer_threshold = v;
#endif /* BUILD_LIBSVDRP */

    if (!value)
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 " * use all parameters by default");
}

static void
cfg_tv_section_save(const char *section)
{
#ifdef BUILD_LIBSVDRP
    int i;

    enna_config_int_set(section, "svdrp_port", tv_cfg.svdrp_port);
    enna_config_int_set(section, "svdrp_timeout", tv_cfg.svdrp_timeout);

    for (i = 0; map_svdrp_verbosity[i].name; i++)
        if (tv_cfg.svdrp_verb == map_svdrp_verbosity[i].verb)
        {
            enna_config_string_set(section, "svdrp_verbosity",
                                   map_svdrp_verbosity[i].name);
            break;
        }

    enna_config_int_set(section,
                        "timer_quit_threshold", tv_cfg.timer_threshold);
#endif /* BUILD_LIBSVDRP */

    enna_config_string_set(section, "vdr_uri", tv_cfg.vdr_uri);
}

static void
cfg_tv_free(void)
{
    ENNA_FREE(tv_cfg.vdr_uri);
#ifdef BUILD_LIBSVDRP
    ENNA_FREE(tv_cfg.svdrp_host);
#endif /* BUILD_LIBSVDRP */
}

static void
cfg_tv_section_set_default(void)
{
    cfg_tv_free();

    tv_cfg.vdr_uri         = strdup("vdr:/");
#ifdef BUILD_LIBSVDRP
    tv_cfg.svdrp_host      = NULL;
    tv_cfg.svdrp_port      = SVDRP_DEFAULT_PORT;
    tv_cfg.svdrp_timeout   = SVDRP_DEFAULT_TIMEOUT;
    tv_cfg.svdrp_verb      = SVDRP_MSG_WARNING;
    tv_cfg.timer_threshold = 15;
#endif /* BUILD_LIBSVDRP */
}

static Enna_Config_Section_Parser cfg_tv = {
    "tv",
    cfg_tv_section_load,
    cfg_tv_section_save,
    cfg_tv_section_set_default,
    cfg_tv_free,
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_tv
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    Enna_File *it;

    if (!em)
        return;

    if (!enna_mediaplayer_supported_uri_type(ENNA_MP_URI_TYPE_VDR) ||
        !enna_mediaplayer_supported_uri_type(ENNA_MP_URI_TYPE_NETVDR))
        return;

    enna_config_section_parser_register(&cfg_tv);
//    cfg_tv_section_set_default();
//    cfg_tv_section_load(cfg_tv.section);

    mod = calloc(1, sizeof(Enna_Module_Tv));
    mod->em = em;
    em->mod = mod;

    enna_activity_register(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();

    it = calloc (1, sizeof(Enna_File));
    it->uri = (char*) eina_stringshare_add (tv_cfg.vdr_uri);
    it->label = (char*) eina_stringshare_add ("vdr");

    enna_mediaplayer_file_append(mod->enna_playlist, it);

#ifdef BUILD_LIBSVDRP
    if (strstr(tv_cfg.vdr_uri, "vdr:/"))
        tv_cfg.svdrp_host = strdup("localhost");
    else if (strstr(tv_cfg.vdr_uri, "netvdr:/"))
    {
        /* TODO needs testing */
        char *p = tv_cfg.vdr_uri + strlen("netvdr://");
        char *q = strstr(tv_cfg.vdr_uri, ":");

        tv_cfg.svdrp_host = malloc (q - p);
        strncpy(tv_cfg.svdrp_host, p, q - p);
    }
    else
        enna_log(ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                 " * unknown vdr uri '%s'", tv_cfg.vdr_uri);

    mod->timer_threshold = tv_cfg.timer_threshold;

    if (tv_cfg.svdrp_host)
        mod->svdrp = enna_svdrp_init(tv_cfg.svdrp_host,
                                     tv_cfg.svdrp_port,
                                     tv_cfg.svdrp_timeout, tv_cfg.svdrp_verb);
#endif /* BUILD_LIBSVDRP */
}

static void
module_shutdown(Enna_Module *em)
{
    if (!mod)
        return;

    enna_activity_unregister(&class);
    ENNA_OBJECT_DEL (mod->o_layout);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
#ifdef BUILD_LIBSVDRP
    enna_svdrp_uninit();
#endif /* BUILD_LIBSVDRP */
    free(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_tv",
    N_("Television"),
    "icon/dev/tv",
    N_("View TV channels"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
