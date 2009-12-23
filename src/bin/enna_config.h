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

#ifndef ENNA_CONFIG_H
#define ENNA_CONFIG_H

typedef struct _Enna_Config Enna_Config;
typedef struct _Enna_Config_Data Enna_Config_Data;
typedef struct _Enna_Config_Video Enna_Config_Video;

typedef enum _ENNA_CONFIG_TYPE ENNA_CONFIG_TYPE;

typedef struct _Config_Pair Config_Pair;

enum _ENNA_CONFIG_TYPE
{
    ENNA_CONFIG_STRING,
    ENNA_CONFIG_STRING_LIST,
    ENNA_CONFIG_INT
};

struct _Enna_Config
{
    /* Theme */
    const char *theme;
    const char *theme_file;
    int idle_timeout;
    int fullscreen;
    int slideshow_delay;
    char *engine;
    const char *verbosity;
    Eina_List *music_local_root_directories;
    Eina_List *music_filters;
    Eina_List *video_filters;
    Eina_List *photo_filters;
    const char *log_file;
    Enna_Config_Video *cfg_video;
};

struct _Enna_Config_Data
{
    char *section;
    Eina_List *pair;
};

struct _Enna_Config_Video
{
    char *sub_align;
    char *sub_pos;
    char *sub_scale;
    char *sub_visibility;
    char *framedrop;
};

struct _Config_Pair
{
    char *key;
    char *value;
};

Enna_Config *enna_config;

const char *enna_config_theme_get(void);
const char *enna_config_theme_file_get(const char *s);
void enna_config_value_store(void *var, char *section,
        ENNA_CONFIG_TYPE type, Config_Pair *pair);
Enna_Config_Data *enna_config_module_pair_get(const char *module_name);
void enna_config_init(const char* conffile);
void enna_config_shutdown(void);

/****************************************************************************/
/*                        Default values                                    */
/****************************************************************************/

#define SLIDESHOW_DEFAULT_TIMER 5.0

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
