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

#ifndef ENNA_CONFIG_H
#define ENNA_CONFIG_H

#include <Elementary.h>

typedef struct _Enna_Config Enna_Config;

struct _Enna_Config
{
    char *cfg_file;
    Elm_Theme *eth;
    char *theme;
    char *theme_file;
    int idle_timeout;
    int fullscreen;
    int slideshow_delay;
    Eina_Bool display_mouse;
    char *engine;
    char *verbosity;
    Eina_List *music_local_root_directories;
    Eina_List *music_filters;
    Eina_List *video_filters;
    Eina_List *photo_filters;
    char *log_file;
};

Enna_Config *enna_config;

void enna_config_load_theme (void);
const char *enna_config_theme_get(void);
const char *enna_config_theme_file_get(const char *s);

/*********************************/

void enna_main_cfg_register (void);

void enna_config_init (const char *file);
void enna_config_shutdown(void);

void enna_config_set_default(void);
void enna_config_load (void);
void enna_config_save (void);

/* configuration getters */
const char * enna_config_string_get (const char *section, const char *key);
Eina_List * enna_config_string_list_get (const char *section, const char *key);
int enna_config_int_get (const char *section, const char *key);
Eina_Bool enna_config_bool_get (const char *section, const char *key);

/* configuration setters */
void enna_config_string_set (const char *section,
                             const char *key, const char *value);
void enna_config_string_list_set (const char *section,
                                  const char *key, Eina_List *value);
void enna_config_int_set (const char *section, const char *key, int value);
void enna_config_bool_set (const char *section,
                           const char *key, Eina_Bool value);

/* Config Section Parser */

typedef struct _Enna_Config_Section_Parser Enna_Config_Section_Parser;
struct _Enna_Config_Section_Parser {
    const char *section;
    void (*load) (const char *section);
    void (*save) (const char *section);
    void (*set_default) (void);
    void (*free) (void);
};

void enna_config_section_parser_register   (Enna_Config_Section_Parser *parser);
void enna_config_section_parser_unregister (Enna_Config_Section_Parser *parser);

/****************************************************************************/
/*                        Config Panel Stuff                                */
/****************************************************************************/

typedef struct _Enna_Config_Panel Enna_Config_Panel;
struct _Enna_Config_Panel
{
    const char *label; /**< Label to show. with locale applied */
    const char *icon;  /**< Name of the icon */
    Evas_Object *(*create_cb)(void *data);  /**< Function to show/create a panel */
    void (*destroy_cb)(void *data);  /**< Function to hide/destroy a panel */
    void *data; /**< User data pointer */
};

Enna_Config_Panel *enna_config_panel_register(const char *label, const char *icon,
                                        Evas_Object *(*create_cb)(void *data),
                                        void (*destroy_cb)(void *data), void *data);
Eina_Bool          enna_config_panel_unregister(Enna_Config_Panel *ecp);
Eina_List         *enna_config_panel_list_get(void);


#endif /* ENNA_CONFIG_H */
