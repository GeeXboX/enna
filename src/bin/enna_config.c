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

#include <Eina.h>
#include <Ecore_File.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "logs.h"
#include "buffer.h"
#include "ini_parser.h"
#include "utils.h"
#include "xdg.h"

static Eina_List *cfg_parsers = NULL;
static ini_t *cfg_ini = NULL;

/****************************************************************************/
/*                       Config File Main Section                           */
/****************************************************************************/

#define SLIDESHOW_DEFAULT_TIMER 5.0

#define FILTER_DEFAULT_MUSIC \
    "3gp,aac,ape,apl,flac,m4a,mac,mka,mp2,mp3,mp4,mpc,oga,ogg,ra,wav,wma"

#define FILTER_DEFAULT_VIDEO \
    "asf,avi,divx,dvr-ms,evo,flc,fli,flv,m1v,m2v,m4p,m4v,mkv,mov,mp4,mp4v,mpe,mpeg,mpg,ogm,ogv,qt,rm,rmvb,swf,ts,vdr,vob,vro,wmv,y4m"

#define FILTER_DEFAULT_PHOTO \
    "jpg,jpeg,png,gif,tif,tiff,xpm"

static void
cfg_main_section_set_default (void)
{

    enna_config->idle_timeout    = 0;
    enna_config->fullscreen      = 0;
    enna_config->slideshow_delay = SLIDESHOW_DEFAULT_TIMER;

    enna_config->theme     = strdup("default");
    enna_config->engine    = strdup("software_x11");
    enna_config->verbosity = strdup("warning");
    enna_config->log_file  = NULL;

    enna_config->music_filters =
        enna_util_tuple_get(FILTER_DEFAULT_MUSIC, ",");
    enna_config->video_filters =
        enna_util_tuple_get(FILTER_DEFAULT_VIDEO, ",");
    enna_config->photo_filters =
        enna_util_tuple_get(FILTER_DEFAULT_PHOTO, ",");

    if (!enna_config->theme)
        enna_config->theme = strdup("default");
    enna_config->theme_file = (char *)
        enna_config_theme_file_get(enna_config->theme);

    enna_log(ENNA_MSG_INFO, NULL, "Theme Name: %s", enna_config->theme);
    enna_log(ENNA_MSG_INFO, NULL, "Theme File: %s", enna_config->theme_file);

    if (enna_config->theme_file)
        elm_theme_overlay_add(enna_config->theme_file);
    else
        enna_log(ENNA_MSG_CRITICAL, NULL, "couldn't load theme file!");
}

#define GET_STRING(v)                                                   \
    value = enna_config_string_get(section, #v);                        \
    if (value)                                                          \
    {                                                                   \
        ENNA_FREE(enna_config->v);                                      \
        enna_config->v = strdup(value);                                 \
    }

#define GET_INT(v)                                                      \
    i = enna_config_int_get(section, #v);                               \
    if (i)                                                              \
        enna_config->v = i;                                             \

#define GET_TUPLE(f,v)                                                  \
    value = enna_config_string_get(section, v);                         \
    if (value)                                                          \
        enna_config->f = enna_util_tuple_get(value, ",");

static void
cfg_main_section_free (void)
{
    char *c;

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

    GET_STRING(theme);
    GET_STRING(engine);
    GET_STRING(verbosity);
    GET_STRING(log_file);

    GET_INT(idle_timeout);
    GET_INT(fullscreen);
    GET_INT(slideshow_delay);

    GET_TUPLE(music_filters, "music_ext");
    GET_TUPLE(video_filters, "video_ext");
    GET_TUPLE(photo_filters, "photo_ext");
}

#define SET_STRING(v)                                                   \
    enna_config_string_set(section, #v, enna_config->v);

#define SET_INT(v)                                                      \
    enna_config_int_set(section, #v, enna_config->v);

#define SET_TUPLE(f,v)                                                  \
    filters = enna_util_tuple_set(enna_config->f, ",");                 \
    enna_config_string_set(section, v, filters);                        \
    ENNA_FREE(filters);

static void
cfg_main_section_save (const char *section)
{
    char *filters;

    SET_STRING(theme);
    SET_STRING(engine);
    SET_STRING(verbosity);
    SET_STRING(log_file);

    SET_INT(idle_timeout);
    SET_INT(fullscreen);
    SET_INT(slideshow_delay);

    SET_TUPLE(music_filters, "music_ext");
    SET_TUPLE(video_filters, "video_ext");
    SET_TUPLE(photo_filters, "photo_ext");
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
}

void
enna_config_section_parser_unregister (Enna_Config_Section_Parser *parser)
{
    if (!parser)
        return;

    cfg_parsers = eina_list_remove(cfg_parsers, parser);
}

void
enna_config_init (const char *file)
{
    char filename[4096];

    enna_config = calloc(1, sizeof(Enna_Config));
    if (file)
        snprintf(filename, sizeof(filename), "%s", file);
    else
        snprintf(filename, sizeof(filename), "%s/enna.cfg",
                 enna_config_home_get());

    enna_config->cfg_file = strdup(filename);
    enna_log(ENNA_MSG_INFO, NULL, "using config file: %s", filename);

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
}

void
enna_config_set_default (void)
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
enna_config_load (void)
{
    Eina_List *l;
    Enna_Config_Section_Parser *p;

    ini_parse(cfg_ini);

    EINA_LIST_FOREACH(cfg_parsers, l, p)
    {
        if (p->load)
            p->load(p->section);
    }
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
enna_config_int_get (const char *section, const char *key)
{
    return ini_get_int(cfg_ini, section, key);
}

Eina_Bool
enna_config_bool_get (const char *section, const char *key)
{
    return ini_get_bool(cfg_ini, section, key);
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
enna_config_theme_get()
{
    return enna_config->theme_file;
}

const char *
enna_config_theme_file_get(const char *s)
{
    char tmp[4096];
    memset(tmp, 0, sizeof(tmp));

    if (!s)
        return NULL;
    if (s[0]=='/')
        snprintf(tmp, sizeof(tmp), "%s", s);

    if (!ecore_file_exists(tmp))
        snprintf(tmp, sizeof(tmp), PACKAGE_DATA_DIR "/enna/theme/%s.edj", s);
    if (!ecore_file_exists(tmp))
        snprintf(tmp, sizeof(tmp), "%s", PACKAGE_DATA_DIR "/enna/theme/default.edj");

    if (ecore_file_exists(tmp))
        return strdup(tmp);
    else
        return NULL;
}

/****************************************************************************/
/*                        Config Panel Stuff                                */
/****************************************************************************/

Eina_List *_config_panels = NULL;

/**
 * @brief Register a new configuration panel
 * @param label The label to show, with locale applied
 * @param icon The name of the icon to use
 * @param create_cb This is the function to call when the panel need to be showed
 * @param destroy_cb This is the function to call when the panel need to be destroyed
 * @param data This is the user data pointer, use as you like
 * @return The object handler (needed to unregister)
 *
 * With this function modules (and not) can register item to be showed in
 * the configuration panel. The use is quite simple: give in a label and the
 * icon that rapresent your panel.
 * You need to implement 2 functions:
 *
 *  Evas_Object *create_cb(void *data)
 *   This function must return the main Evas_Object of your panel
 *   The data pointer is the data you set in the register function
 *
 *  void *destroy_cb(void *data)
 *   In this function you must hide your panel, and possiby free some resource
 *
 */
Enna_Config_Panel *
enna_config_panel_register(const char *label, const char *icon,
                           Evas_Object *(*create_cb)(void *data),
                           void (*destroy_cb)(void *data),
                           void *data)
{
    Enna_Config_Panel *ecp;

    if (!label) return NULL;

    ecp = ENNA_NEW(Enna_Config_Panel, 1);
    if (!ecp) return EINA_FALSE;
    ecp->label = eina_stringshare_add(label);
    ecp->icon = eina_stringshare_add(icon);
    ecp->create_cb = create_cb;
    ecp->destroy_cb = destroy_cb;
    ecp->data = data;

    _config_panels = eina_list_append(_config_panels, ecp);
    //TODO here emit an event like ENNA_CONFIG_PANEL_CHANGED
    return ecp;
}

/**
 * @brief UnRegister a configuration panel
 * @param destroy_cb This is the function to call when the panel need to be destroyed
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * When you dont need the entry in the configuration panel use this function.
 * You should do this at least on shoutdown
 */
Eina_Bool
enna_config_panel_unregister(Enna_Config_Panel *ecp)
{
    if (!ecp) return EINA_FALSE;

    _config_panels = eina_list_remove(_config_panels, ecp);
    if (ecp->label) eina_stringshare_del(ecp->label);
    if (ecp->icon) eina_stringshare_del(ecp->icon);
    ENNA_FREE(ecp);
    //TODO here emit an event like ENNA_CONFIG_PANEL_CHANGED
    return EINA_TRUE;
}

Eina_List *
enna_config_panel_list_get(void)
{
    return _config_panels;
}
