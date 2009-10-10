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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Eina.h>
#include <Ecore_File.h>
#include <Ecore_Str.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "logs.h"
#include "slideshow.h"

#define DEFAULT_FILE \
    "[enna]\n" \
    "#none,event,info,warning,error,critical\n" \
    "verbosity=info\n" \
    "log_file=/var/log/enna.log\n" \
    "\n" \
    "\n" \
    "#0,1\n" \
    "fullscreen=0\n" \
    "\n" \
    "#auto,top,middle,bottom\n" \
    "sub_align=auto\n" \
    "\n" \
    "#auto (value set to 100 for bottom),M..N where M and N are integer\n" \
    "sub_pos=auto\n" \
    "\n" \
    "#auto (value set to 5) ,M..N where M and N are integer\n" \
    "sub_scale=auto\n" \
    "\n" \
    "#auto,yes,no\n" \
    "sub_visibility=auto\n" \
    "\n" \
    "#no,soft,hard\n" \
    "framedrop=no\n" \
    "\n" \
    "slideshow_delay=5\n" \
    "\n" \
    "idle_timeout=0\n" \
    "\n" \
    "#/home/user/theme.edj for specific file or just 'default' if theme is\n" \
    "#located at /usr/share/enna/theme/default.edj\n" \
    "theme=default\n" \
    "\n" \
    "#software_x11,xrender_x11,opengl_x11,software_x11_16\n" \
    "engine=software_x11\n" \
    "\n" \
    "music_ext=3gp,aac,ape,apl,flac,m4a,mac,mka,mp2,mp3,mp4,mpc,ogg,ra,wav,wma\n" \
    "video_ext=asf,avi,divx,dvr-ms,evo,flc,fli,flv,m1v,m2v,m4p,m4v,mkv,mov,mp4,mp4v,mpe,mpeg,mpg,ogm,qt,rm,rmvb,swf,ts,vdr,vob,vro,wmv,y4m\n" \
    "photo_ext=jpg,jpeg,png,gif,tif,tiff,xpm\n" \
    "\n" \
    "[libplayer]\n" \
    "# Values by default are 'mplayer' type, 'auto' video_out, 'auto' audio_out\n" \
    "# and 'warning' verbosity. Change these parameters only if that is really\n" \
    "# necessary.\n" \
    "\n" \
    "#mplayer,xine\n" \
    "#type=\n" \
    "#dvd_type=\n" \
    "#tv_type=\n" \
    "\n" \
    "#auto,vdpau,x11,xv,gl,fb\n" \
    "#video_out=\n" \
    "\n" \
    "#auto,alsa,oss\n" \
    "#audio_out=\n" \
    "\n" \
    "#verbose,info,warning,error,critical,none\n" \
    "#verbosity=\n" \
    "\n" \
    "# Metadata module (video)\n" \
    "blacklist_keywords=0tv,1080p,2hd,720p,ac3,booya,caph,crimson,ctu,dimension,divx,dot,dsr,dvdrip,dvdscr,e7,etach,fov,fqm,hdq,hdtv,lol,mainevent,notv,pdtv,proper,pushercrew,repack,reseed,screencam,screener,sys,vtv,x264,xor,xvid\n" \
    "\n" \
    "[localfiles]\n" \
    "path_music=file:///path/to/Music,Music,icon/favorite\n" \
    "path_music=file:///path/to/server/Medias/Music,Server,icon/dev/nfs\n" \
    "path_video=file:///path/to/Videos,Videos,icon/favorite\n" \
    "path_video=file:///path/to/server/Medias/Videos,Server,icon/dev/nfs\n" \
    "path_photo=file:///path/to/Photos,Photos,icon/favorite\n" \
    "path_photo=file:///path/to/server/Medias/Photos,Server,icon/dev/nfs\n" \
    "\n" \
    "[netstreams]\n" \
    "stream_video=http://mafreebox.freebox.fr/freeboxtv/playlist.m3u,FreeboxTV,icon/freeboxtv\n" \
    "\n" \
    "[tv]\n" \
    "# Default: 'vdr:/' (autodetect local VDR)\n" \
    "# Local VDR: 'vdr://tmp/vdr-xine/stream#demux:mpeg_pes\n" \
    "# Remote VDR: 'netvdr://192.168.1.1:18701\n" \
    "#vdr_uri=\n" \
    "\n" \
    "# Default: 2001\n" \
    "#svdrp_port=\n" \
    "\n" \
    "# Default: 30\n" \
    "#svdrp_timeout=\n" \
    "\n" \
    "#verbose,info,warning,error,critical,none\n" \
    "#svdrp_verbosity=\n" \
    "\n" \
    "# Don't exit if a timer will fire before this value (in minutes)\n" \
    "# Default: 15\n" \
    "#timer_quit_threshold=\n" \
    "\n" \
    "[weather]\n" \
    "city=New York\n" \
    "\n" \
    "[valhalla]\n" \
    "path=file:///path/to/Music\n" \
    "path=file:///path/to/server/Medias/Music\n" \
    "verbosity=info\n" \
    "parser_number=2\n" \
    "commit_interval=128\n" \
    "# <=0 for infinite\n" \
    "scan_loop=-1\n" \
    "# time [sec] for sleeping between loops\n" \
    "scan_sleep=900\n" \
    "# 0: normal, -20: higher, 19 lower\n" \
    "scan_priority=19\n" \
    "\n" \
    "[lms]\n" \
    "path=file:///path/to/Music\n" \
    "path=file:///path/to/server/Medias/Music\n" \
    "parser=mp3,ogg,flac,rm,dummy\n" \
    "slave_timeout=1000\n" \
    "commit_interval=100\n" \


static Eina_Hash *hash_config;

static Eina_Bool _hash_foreach(const Eina_Hash *hash, const void *key,
        void *data, void *fdata);
static Eina_Hash *_config_load_conf_file(char *filename);
static Eina_Hash *_config_load_conf(char *conffile, int size);

const char * enna_config_theme_get()
{
    return enna_config->theme_file;
}

const char * enna_config_theme_file_get(const char *s)
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

Enna_Config_Video * enna_config_video_get()
{
    Enna_Config_Video *config_video = calloc(1, sizeof(Enna_Config_Video));;

    config_video->sub_align = enna_config->sub_align;
    config_video->sub_pos = enna_config->sub_pos;
    config_video->sub_scale = enna_config->sub_scale;
    config_video->sub_visibility = enna_config->sub_visibility;
    config_video->framedrop = enna_config->framedrop;

    return config_video;
}

void enna_config_value_store(void *var, char *section,
        ENNA_CONFIG_TYPE type, Config_Pair *pair)
{
    if (!strcmp(pair->key, section))
    {
        switch (type)
        {
            case ENNA_CONFIG_INT:
            {
                int *value = var;
                *value = atoi(pair->value);
                break;
            }
            case ENNA_CONFIG_STRING:
            {
                char **value = var;
                *value = strdup(pair->value);
                break;
            }
            case ENNA_CONFIG_STRING_LIST:
            {
                Eina_List *list;
                Eina_List **value = var;
                char **clist;
                char *string;
                int i;

                list = NULL;
                clist = ecore_str_split(pair->value, ",", 0);

                for (i = 0; (string = clist[i]); i++)
                {
                    if (!string)
                        break;
                    list = eina_list_append(list, string);
                }
                *value = list;
            }
            default:
                break;
        }
    }
}

Enna_Config_Data *
enna_config_module_pair_get(const char *module_name)
{
    if(!hash_config || !module_name)
    return NULL;

    return eina_hash_find(hash_config, module_name);
}

void enna_config_init(const char* conffile)
{
    char filename[4096];

    enna_config = calloc(1, sizeof(Enna_Config));
    if (conffile)
        snprintf(filename, sizeof(filename), "%s", conffile);
    else
        snprintf(filename, sizeof(filename), "%s/.enna/enna.cfg",
            enna_util_user_home_get());

    hash_config = _config_load_conf_file(filename);
    if (hash_config) {
        eina_hash_foreach(hash_config, _hash_foreach, NULL);
        enna_log(ENNA_MSG_INFO, NULL, "using config file: %s.", filename);
    }
    else enna_log(ENNA_MSG_WARNING, NULL, "couldn't load enna config file: %s.", filename);

    if (!enna_config->theme) 
        enna_config->theme=strdup("default");
    enna_config->theme_file
        = enna_config_theme_file_get(enna_config->theme);

    enna_log(ENNA_MSG_INFO, NULL, "Theme Name: %s", enna_config->theme);
    enna_log(ENNA_MSG_INFO, NULL, "Theme File: %s", enna_config->theme_file);
    if (enna_config->theme_file)
        elm_theme_overlay_add(enna_config->theme_file);
    else
        enna_log(ENNA_MSG_CRITICAL, NULL, "couldn't load theme file!");

}

void enna_config_shutdown(void)
{

}

static Eina_Bool _hash_foreach(const Eina_Hash *hash, const void *key,
        void *data, void *fdata)
{
    Enna_Config_Data *config_data;
    Eina_List *l;
    if (!strcmp(key, "enna"))
    {
        config_data = data;

        enna_config->slideshow_delay = SLIDESHOW_DEFAULT_TIMER;

        enna_config->sub_align = "auto";
        enna_config->sub_pos = "auto";
        enna_config->sub_scale = "auto";
        enna_config->sub_visibility = "auto";
        enna_config->framedrop = "no";

        for (l = config_data->pair; l; l = l->next)
        {
            Config_Pair *pair = l->data;
            enna_config_value_store(&enna_config->theme, "theme",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->idle_timeout, "idle_timeout",
                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&enna_config->fullscreen, "fullscreen",
                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&enna_config->sub_align, "sub_align",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->sub_pos, "sub_pos",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->sub_scale, "sub_scale",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->sub_visibility,
	                            "sub_visibility",
                                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->framedrop, "framedrop",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->slideshow_delay,
                                    "slideshow_delay",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&enna_config->engine, "engine",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->verbosity, "verbosity",
                    ENNA_CONFIG_STRING, pair);
            enna_config_value_store(&enna_config->music_filters, "music_ext",
                    ENNA_CONFIG_STRING_LIST, pair);
            enna_config_value_store(&enna_config->video_filters, "video_ext",
                    ENNA_CONFIG_STRING_LIST, pair);
            enna_config_value_store(&enna_config->photo_filters, "photo_ext",
                    ENNA_CONFIG_STRING_LIST, pair);
            enna_config_value_store(&enna_config->log_file, "log_file",
                    ENNA_CONFIG_STRING, pair);
        }
    }

    return 1;
}

static Eina_Hash * _config_load_conf_file(char *filename)
{
    int fd;
    FILE *f;
    struct stat st;
    char tmp[4096];
    char *conffile;
    int ret;

    if (stat(filename, &st))
    {
        enna_log(ENNA_MSG_WARNING, NULL, "Cannot stat file %s", filename);
        snprintf(tmp, sizeof(tmp), "%s/.enna", enna_util_user_home_get());
        if (!ecore_file_is_dir(tmp))
            ecore_file_mkdir(tmp);

        if (!(f = fopen(filename, "w")))
            return NULL;
        else
        {
            fprintf(f, DEFAULT_FILE);
            fclose(f);
        }

    }

    if (stat(filename, &st))
    {
        enna_log(ENNA_MSG_ERROR, NULL, "Cannot stat file %s", filename);
        return NULL;
    }

    conffile = malloc(st.st_size);

    if ((fd = open(filename, O_RDONLY)) < 0)
    {
        enna_log(ENNA_MSG_ERROR, NULL, "Cannot open file");
        return NULL;
    }

    ret = read(fd, conffile, st.st_size);

    if (ret != st.st_size)
    {
        enna_log(ENNA_MSG_ERROR, NULL,
                "Cannot read conf file entirely, read only %d bytes", ret);
        return NULL;
    }

    return _config_load_conf(conffile, st.st_size);
}

static Eina_Hash * _config_load_conf(char *conffile, int size)
{
    char *current_section = NULL;
    char *current_line = conffile;
    Eina_Hash *config = NULL;
    Enna_Config_Data *config_data;

    config = eina_hash_string_superfast_new (NULL);

    while (current_line < conffile + size)
    {
        char *eol = strchr(current_line, '\n');
        Config_Pair *pair;
        char *key;
        char *value;
        if (eol)
            *eol = 0;
        else
            // Maybe the end of file
            eol = conffile + size;

        // Removing the leading spaces
        while (*current_line && *current_line == ' ')
            current_line++;

        // Maybe an empty line
        if (!(*current_line))
        {
            current_line = eol + 1;
            continue;
        }

        // Maybe a comment line
        if (*current_line == '#')
        {
            current_line = eol + 1;
            continue;
        }

        // We are at a section definition
        if (*current_line == '[')
        {
            // ']' must be the last char of this line
            char *end_of_section_name = strchr(current_line + 1, ']');

            if (end_of_section_name[1] != 0)
            {
                enna_log(ENNA_MSG_WARNING, NULL, "malformed section name %s",
                        current_line);
                return NULL;
            }
            current_line++;
            *end_of_section_name = '\0';

            // Building the section
            if (current_section)
                free(current_section);
            current_section = strdup(current_line);
            config_data = calloc(1, sizeof(Enna_Config_Data));
            config_data->section = current_section;
            config_data->pair = NULL;
            eina_hash_add(config, current_section, config_data);
            current_line = eol + 1;
            continue;

        }

        // Must be in a section to provide a key/value pair
        if (!current_section)
        {
            enna_log(ENNA_MSG_WARNING, NULL, "No section for this line %s",
                    current_line);
            /* FIXME : free hash and confile*/
            return NULL;
        }

        // Building the key/value string pair
        key = current_line;
        value = strchr(current_line, '=');
        if (!value)
        {
            enna_log(ENNA_MSG_WARNING, NULL, "Malformed line %s", current_line);
            /* FIXME : free hash and confile*/
            return NULL;
        }
        *value = '\0';
        value++;
        pair = calloc(1, sizeof(Config_Pair));
        pair->key = strdup(key);
        pair->value = strdup(value);
        config_data = eina_hash_find(config, current_section);
        if (config_data)
        {
            config_data->pair = eina_list_append(config_data->pair, pair);
            /* Need this ? */
            /*eina_hash_modify(hash, current_section, config_data);*/
        }

        current_line = eol + 1;
    }
    free(conffile);
    return config;
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
