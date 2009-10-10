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
