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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <Ecore_File.h>

#include "enna.h"
#include "enna_config.h"
#include "enna_config_main.h"
#include "platform.h"
#include "utils.h"
#include "logs.h"
#include "buffer.h"
#include "ini_parser.h"

static Eina_List *cfg_parsers = NULL;
static ini_t *cfg_ini = NULL;
Enna_Config *enna_config;


// These GET_ macros only change the value if set

#define GET_STRING(section, v)                                          \
    do {								\
        value = enna_config_string_get(section, #v);                    \
	if (value)                                                      \
	{                                                               \
	    ENNA_FREE(enna_config->v);                                  \
	    enna_config->v = strdup(value);                             \
	}							        \
    } while (0)

#define GET_INT(section, v)                                             \
    do {								\
	i = enna_config_int_get(section, #v, 0, &is_set);               \
	if (is_set)							\
	    enna_config->v = i;                                         \
    } while (0)

#define GET_BOOL(section, v)                                            \
    do {								\
	i = enna_config_bool_get(section, #v, 0, &is_set);              \
	if (is_set)                                                     \
	    enna_config->v = i;                                         \
    } while (0)		

#define GET_TUPLE(section, f, v)                                        \
    do {								\
	value = enna_config_string_get(section, v);                     \
	if (value)                                                      \
	    enna_config->f = enna_util_tuple_get(value, ",");		\
    } while (0)


#define SET_STRING(section, v)                                          \
    enna_config_string_set(section, #v, enna_config->v)

#define SET_INT(section, v)                                             \
    enna_config_int_set(section, #v, enna_config->v)

#define SET_BOOL(section, v)                                            \
    enna_config_bool_set(section, #v, enna_config->v)

#define SET_TUPLE(section, f,v)                                         \
    do {								\
	filters = enna_util_tuple_set(enna_config->f, ",");             \
	enna_config_string_set(section, v, filters);                    \
	ENNA_FREE(filters);						\
    } while (0)


/****************************************************************************/
/*                       Config File Main Section                           */
/****************************************************************************/

#define DEFAULT_IDLE_TIMEOUT	0
#define SLIDESHOW_DEFAULT_TIMER 5.0

#define FILTER_DEFAULT_MUSIC \
    "3gp,aac,ape,apl,flac,m4a,mac,mka,mp2,mp3,mp4,mpc,oga,ogg,ra,wav,wma"

#define FILTER_DEFAULT_VIDEO \
    "asf,avi,divx,dvr-ms,evo,flc,fli,flv,m1v,m2v,m4p,m4v,mkv,mov,mp4,mp4v,mpe,mpeg,mpg,ogm,ogv,qt,rm,rmvb,swf,ts,vdr,vob,vro,wmv,y4m"

#define FILTER_DEFAULT_PHOTO \
    "jpg,jpeg,png,gif,tif,tiff,xpm"

#define DEFAULT_PROFILE		"720p"

#define PACKAGE_THEME_DIR	PACKAGE_DATA_DIR "/enna/theme"


static const struct {
    const char *name;
    const char *theme;
    int width;
    int height;
} profile_resolution_mapping[] = {
    { "480p",       "default",   640,  480 },
    { "576p",       "default",   720,  576 },
    { "720p",       "default",  1280,  720 },
    { "1080p",      "default",  1920, 1080 },
    { "vga",        "default",   640,  480 },
    { "ntsc",       "default",   720,  480 },
    { "pal",        "default",   768,  576 },
    { "wvga",       "default",   800,  480 },
    { "svga",       "default",   800,  600 },
    { "touchbook","touchbook",  1024,  600 },
    { "netbook",    "default",  1024,  600 },
    { "hdready",    "default",  1366,  768 },
    { "fullhd",     "default",  1920, 1080 },
    { NULL,              NULL,     0,    0 }
};



int config_profile_parse (const char *profile, const char **pt,
                    unsigned int *pw, unsigned int *ph)
{
    // NOTE: don't touch return values if profile is invalid

    int i;

    if (!profile)
        return 0;

    for (i = 0; profile_resolution_mapping[i].name; i++)
        if (!strcasecmp(profile, profile_resolution_mapping[i].name))
        {
            *pt = profile_resolution_mapping[i].theme;
            *pw = profile_resolution_mapping[i].width;
            *ph = profile_resolution_mapping[i].height;
            return 1;
        }

    return 0;
}

void list_profiles (void)
{
    int i;
    for (i = 0; profile_resolution_mapping[i].name; i++)
    {
	printf ("%s ", profile_resolution_mapping[i].name);
    }
    printf("\n");
}


/****************************************************************************/
/*                       Config Main Section Parser                         */
/****************************************************************************/


static void
cfg_main_section_set_default (void)
{
    enna_config->profile         = strdup(DEFAULT_PROFILE);
    const char *theme = NULL;
    config_profile_parse(enna_config->profile, &theme,
                   &enna_config->app_width, &enna_config->app_height);

    assert(theme);
    enna_config->theme     	 = strdup(theme);

    enna_config->idle_timeout    = DEFAULT_IDLE_TIMEOUT;
    enna_config->fullscreen      = 0;
    enna_config->slideshow_delay = SLIDESHOW_DEFAULT_TIMER;
    enna_config->display_mouse   = EINA_TRUE;
    enna_config->engine    = strdup(FAILSAFE_ENGINE);
    enna_config->verbosity = strdup("warning");
    enna_config->log_file  = NULL;

    enna_config->music_filters =
        enna_util_tuple_get(FILTER_DEFAULT_MUSIC, ",");
    enna_config->video_filters =
        enna_util_tuple_get(FILTER_DEFAULT_VIDEO, ",");
    enna_config->photo_filters =
        enna_util_tuple_get(FILTER_DEFAULT_PHOTO, ",");
}


static void
cfg_main_section_free (void)
{
    char *c;

    ENNA_FREE(enna_config->profile);
    ENNA_FREE(enna_config->theme);
    ENNA_FREE(enna_config->engine);
    ENNA_FREE(enna_config->verbosity);
    ENNA_FREE(enna_config->log_file);

    EINA_LIST_FREE(enna_config->music_filters, c)
        ENNA_FREE(c);

    EINA_LIST_FREE(enna_config->video_filters, c)
        ENNA_FREE(c);

    EINA_LIST_FREE(enna_config->photo_filters, c)
        ENNA_FREE(c);
}

static void
cfg_main_section_load (const char *section)
{
    const char *value;
    int i;
    Eina_Bool is_set;

    enna_log(ENNA_MSG_INFO, NULL, "main section load: %s", section);

    GET_STRING(section, engine);
    GET_STRING(section, verbosity);
    GET_STRING(section, log_file);

    GET_INT(section, idle_timeout);

    GET_STRING(section, profile);
    if (value)
    {
        const char *theme = NULL;
        if (!config_profile_parse(enna_config->profile, &theme,
                   &enna_config->app_width, &enna_config->app_height))
	{
    	    enna_log(ENNA_MSG_CRITICAL, NULL, "invalid profile: %s", value);

	    // enna_config->theme, app_width, app_height will still be default values
	    // so use those
	}
	else
 	{
	    // valid profile, width, height set, now set theme
    	    ENNA_FREE(enna_config->theme);
            enna_config->theme  = strdup(theme);
	}
        enna_log(ENNA_MSG_INFO, NULL, "read profile: %s, width=%d, height=%d, theme=%s",
		value, enna_config->app_width, enna_config->app_height, enna_config->theme);
    }
    else
    {
	// only use these if profile not defined, or FIXME should we permit overrides ?
        //GET_STRING(section, theme);
        GET_INT(section, app_width);
        GET_INT(section, app_height);
        enna_log(ENNA_MSG_INFO, NULL, "DID NOT read profile, reading individual width=%d, height=%d,theme=%s",
		enna_config->app_width, enna_config->app_height, enna_config->theme);
    }

    // allow explicit theme to override default profile
    GET_STRING(section, theme);


    // get rest of properties
    GET_INT(section, fullscreen);
    GET_INT(section, slideshow_delay);
    GET_BOOL(section, display_mouse);

    GET_TUPLE(section, music_filters, "music_ext");
    GET_TUPLE(section, video_filters, "video_ext");
    GET_TUPLE(section, photo_filters, "photo_ext");
}

static void
cfg_main_section_save (const char *section)
{
    char *filters;

    SET_STRING(section, engine);
    SET_STRING(section, verbosity);
    SET_STRING(section, log_file);

// KRL: SET_STRING(section, profile) (need to resolve with theme, width, height)
// Any point setting profile, can't change at runtime !?
//    SET_STRING(section, theme);
// Only way values get changed is via cmdline override (which are temp)
// Don't save changed values back to ini file (cfg_ini has orig value to save)
//    SET_INT(section, app_width);
//    SET_INT(section, app_height);
//    SET_INT(section, fullscreen);

    SET_INT(section, idle_timeout);
    SET_INT(section, slideshow_delay);

    SET_BOOL(section, display_mouse);

    SET_TUPLE(section, music_filters, "music_ext");
    SET_TUPLE(section, video_filters, "video_ext");
    SET_TUPLE(section, photo_filters, "photo_ext");
}


static Enna_Config_Section_Parser cfg_main_section = {
    "enna",
    cfg_main_section_load,
    cfg_main_section_save,
    cfg_main_section_set_default,
    cfg_main_section_free,
};

void
enna_main_cfg_register (void)
{
    enna_config_section_parser_register(&cfg_main_section);
}


/****************************************************************************/
/*                       Config File Reader/Writer                          */
/****************************************************************************/

void
enna_config_section_parser_register (Enna_Config_Section_Parser *parser)
{
    if (!parser)
        return;

    cfg_parsers = eina_list_append(cfg_parsers, parser);

    // we guarantee config file will have been loaded at this point so we call the loader
    if (parser->set_default) parser->set_default();
    if (parser->load) parser->load(parser->section);
}

void
enna_config_section_parser_unregister (Enna_Config_Section_Parser *parser)
{
    if (!parser)
        return;

    cfg_parsers = eina_list_remove(cfg_parsers, parser);
}

void
enna_config_load (const char *file)
{
    // file is optional (ie fallback exists)
    // Note: loads and parses file internally but does not call app. specific section parsers

    char filename[1024];

    enna_config = calloc(1, sizeof(Enna_Config));
    if (file)
        snprintf(filename, sizeof(filename), "%s", file);
    else
        snprintf(filename, sizeof(filename), "%s/enna.cfg",
                 enna_util_config_home_get());

    enna_config->cfg_file = strdup(filename);
    enna_log(ENNA_MSG_INFO, NULL, "Using config file: %s", filename);

    if (!cfg_ini)
        cfg_ini = ini_new(filename);

    ini_parse(cfg_ini);
}

void
enna_config_shutdown (void)
{
    Eina_List *l;
    Enna_Config_Section_Parser *p;

    /* save current configuration to file */
    enna_config_save();

    EINA_LIST_FOREACH(cfg_parsers, l, p)
    {
        if (p->free)
            p->free();
        enna_config_section_parser_unregister(p);
    }

    if (cfg_ini)
        ini_free(cfg_ini);
    cfg_ini = NULL;

    // KRL: FIXME: we should FREE enna_config in this file somewhere
    ENNA_FREE(enna_config);
}

#if 0
void
enna_config_set_default_all_sections (void)
{
    Eina_List *l;
    Enna_Config_Section_Parser *p;

    EINA_LIST_FOREACH(cfg_parsers, l, p)
    {
        if (p->set_default)
            p->set_default();
    }
}

void
enna_config_parse_all_sections (void)
{
    Eina_List *l;
    Enna_Config_Section_Parser *p;

//    ini_parse(cfg_ini);

    EINA_LIST_FOREACH(cfg_parsers, l, p)
    {
	enna_config_parse_section(p, EINA_FALSE);
    }
}
#endif

void
enna_config_parse_section(Enna_Config_Section_Parser *p, Eina_Bool set_default)
{
    enna_log(ENNA_MSG_INFO, NULL, "Config Load: %s", p->section);

    if (set_default && p->set_default)
	p->set_default();

    if (p->load)
	p->load(p->section);
}

void
enna_config_save (void)
{
    Eina_List *l;
    Enna_Config_Section_Parser *p;

    EINA_LIST_FOREACH(cfg_parsers, l, p)
    {
        if (p->save)
            p->save(p->section);
    }

    ini_dump(cfg_ini);
}

const char *
enna_config_string_get (const char *section, const char *key)
{
    return ini_get_string(cfg_ini, section, key);
}

Eina_List *
enna_config_string_list_get (const char *section, const char *key)
{
    return ini_get_string_list(cfg_ini, section, key);
}

int
enna_config_int_get (const char *section, const char *key, int default_value, Eina_Bool *p_is_set)
{
    return ini_get_int(cfg_ini, section, key, default_value, p_is_set);
}

Eina_Bool
enna_config_bool_get (const char *section, const char *key, Eina_Bool default_value, Eina_Bool *p_is_set)
{
    return ini_get_bool(cfg_ini, section, key, default_value, p_is_set);
}

void
enna_config_string_set (const char *section,
                        const char *key, const char *value)
{
    ini_set_string(cfg_ini, section, key, value);
}

void
enna_config_string_list_set (const char *section,
                             const char *key, Eina_List *value)
{
    ini_set_string_list(cfg_ini, section, key, value);
}

void
enna_config_int_set (const char *section, const char *key, int value)
{
    ini_set_int(cfg_ini, section, key, value);
}

void
enna_config_bool_set (const char *section, const char *key, Eina_Bool value)
{
    ini_set_bool(cfg_ini, section, key, value);
}

/****************************************************************************/
/*                                Theme                                     */
/****************************************************************************/

const char *
enna_config_resolve_theme_file(const char *s)
{
    // caller to free returned string

    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));

    if (!s)
        return NULL;

    if (s[0]=='/')
    {
        snprintf(tmp, sizeof(tmp), "%s", s); // KRL FIXME append .edj !?
	if (ecore_file_exists(tmp))
	    return strdup(tmp);
    }

    snprintf(tmp, sizeof(tmp), PACKAGE_THEME_DIR "/%s.edj", s);
    if (ecore_file_exists(tmp))
	return strdup(tmp);

    snprintf(tmp, sizeof(tmp), "%s", PACKAGE_THEME_DIR "/default.edj");
    if (ecore_file_exists(tmp))
	return strdup(tmp);

    return NULL; // not found
}

/****************************************************************************/
/*                       Config Accessor Functions                          */
/****************************************************************************/

const char *
enna_config_theme_get()
{
    return enna_config->theme_file; // NOTE: returns theme_file (not theme name)
}

const char *
enna_config_engine()
{
    return enna_config->engine;
}

int
enna_config_slideshow_delay()
{
    return enna_config->slideshow_delay;
}
