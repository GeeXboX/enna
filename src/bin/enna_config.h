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

typedef struct _Enna_Config Enna_Config;
extern Enna_Config *enna_config;


/* general access to enna config properties */
const char *enna_config_theme_get(void);
const char *enna_config_engine(void);
int enna_config_slideshow_delay(void);


/* configuration getters */
const char * enna_config_string_get (const char *section, const char *key);
Eina_List * enna_config_string_list_get (const char *section, const char *key);
int enna_config_int_get (const char *section, const char *key, int default_value, Eina_Bool *is_set);
Eina_Bool enna_config_bool_get (const char *section, const char *key, Eina_Bool default_value, Eina_Bool *is_set);

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

#endif /* ENNA_CONFIG_H */
