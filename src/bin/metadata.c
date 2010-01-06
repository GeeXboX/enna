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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include <Eina.h>
#include <Ecore_File.h>

#include <valhalla.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "logs.h"
#include "utils.h"
#include "buffer.h"

#define MODULE_NAME "enna"

#define ENNA_METADATA_DB_NAME                        "media.db"
#define ENNA_METADATA_DEFAULT_PARSER_NUMBER           2
#define ENNA_METADATA_DEFAULT_GRABBER_NUMBER          4
#define ENNA_METADATA_DEFAULT_COMMIT_INTERVAL         128
#define ENNA_METADATA_DEFAULT_SCAN_LOOP              -1
#define ENNA_METADATA_DEFAULT_SCAN_SLEEP              900
#define ENNA_METADATA_DEFAULT_SCAN_PRIORITY           19

#define PATH_FANARTS            "fanarts"
#define PATH_COVERS             "covers"

#define PATH_BUFFER 4096

static valhalla_t *vh = NULL;

#define SUFFIX_ADD(type)                                       \
    for (l = enna_config->type; l; l = l->next)                \
    {                                                          \
        const char *ext = l->data;                             \
        valhalla_config_set(vh, SCANNER_SUFFIX, ext);          \
    }                                                          \

static void
ondemand_cb(const char *file, valhalla_event_od_t e, const char *id, void *data)
{
    switch (e)
    {
    case VALHALLA_EVENTOD_PARSED:
        enna_log(ENNA_MSG_EVENT,
                 MODULE_NAME, _("[%s]: parsing done."), file);
        break;
    case VALHALLA_EVENTOD_GRABBED:
        enna_log(ENNA_MSG_EVENT,
                 MODULE_NAME, _("[%s]: %s grabber has finished"), file, id);
        break;
    case VALHALLA_EVENTOD_ENDED:
        enna_log(ENNA_MSG_INFO,
                 MODULE_NAME, _("[%s]: all metadata have been fetched."), file);
        break;
    }
}

static void
enna_metadata_db_init(void)
{
    int rc;
    Enna_Config_Data *cfgdata;
    char *value = NULL;
    char db[PATH_BUFFER];
    Eina_List *path = NULL, *l;
    Eina_List *bl_words = NULL;
    int parser_number   = ENNA_METADATA_DEFAULT_PARSER_NUMBER;
    int grabber_number  = ENNA_METADATA_DEFAULT_GRABBER_NUMBER;
    int commit_interval = ENNA_METADATA_DEFAULT_COMMIT_INTERVAL;
    int scan_loop       = ENNA_METADATA_DEFAULT_SCAN_LOOP;
    int scan_sleep      = ENNA_METADATA_DEFAULT_SCAN_SLEEP;
    int scan_priority   = ENNA_METADATA_DEFAULT_SCAN_PRIORITY;
    valhalla_verb_t verbosity = VALHALLA_MSG_WARNING;
    valhalla_init_param_t param;
    char dst[1024];

    cfgdata = enna_config_module_pair_get("media_db");
    if (cfgdata)
    {
        Eina_List *list;

        for (list = cfgdata->pair; list; list = list->next)
        {
            Config_Pair *pair = list->data;

            enna_config_value_store(&parser_number, "parser_number",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&grabber_number, "grabber_number",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&commit_interval, "commit_interval",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_loop, "scan_loop",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_sleep, "scan_sleep",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_priority, "scan_priority",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store (&bl_words, "blacklist_keywords",
                                     ENNA_CONFIG_STRING_LIST, pair);

            if (!strcmp("path", pair->key))
            {
                enna_config_value_store(&value, "path",
                                        ENNA_CONFIG_STRING, pair);
                if (strstr(value, "file://") == value)
                    path = eina_list_append(path, value + 7);
            }
            else if (!strcmp("verbosity", pair->key))
            {
                enna_config_value_store(&value, "verbosity",
                                        ENNA_CONFIG_STRING, pair);

                if (!strcmp("verbose", value))
                    verbosity = VALHALLA_MSG_VERBOSE;
                else if (!strcmp("info", value))
                    verbosity = VALHALLA_MSG_INFO;
                else if (!strcmp("warning", value))
                    verbosity = VALHALLA_MSG_WARNING;
                else if (!strcmp("error", value))
                    verbosity = VALHALLA_MSG_ERROR;
                else if (!strcmp("critical", value))
                    verbosity = VALHALLA_MSG_CRITICAL;
                else if (!strcmp("none", value))
                    verbosity = VALHALLA_MSG_NONE;
            }
        }
    }

    /* Configuration */
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* parser number  : %i", parser_number);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* grabber number : %i", grabber_number);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* commit interval: %i", commit_interval);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan loop      : %i", scan_loop);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan sleep     : %i", scan_sleep);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan priority  : %i", scan_priority);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* verbosity      : %i", verbosity);

    valhalla_verbosity(verbosity);

    snprintf(db, sizeof(db),
             "%s/.enna/%s", enna_util_user_home_get(), ENNA_METADATA_DB_NAME);

    memset(&param, 0, sizeof(valhalla_init_param_t));
    param.parser_nb   = parser_number;
    param.grabber_nb  = grabber_number;
    param.commit_int  = commit_interval;
    param.decrapifier = 1;
    param.od_cb       = ondemand_cb;

    vh = valhalla_init(db, &param);
    if (!vh)
        goto err;

    /* Add file suffixes */
    SUFFIX_ADD(music_filters);
    SUFFIX_ADD(video_filters);
    SUFFIX_ADD(photo_filters);

    /* Add paths */
    for (l = path; l; l = l->next)
    {
        const char *str = l->data;
        valhalla_config_set(vh, SCANNER_PATH, str, 1);
    }
    if (path)
    {
        eina_list_free(path);
        path = NULL;
    }

    /* blacklist some keywords */
    for (l = bl_words; l; l = l->next)
    {
        const char *keyword = l->data;
        valhalla_config_set(vh, PARSER_KEYWORD, keyword);
        enna_log(ENNA_MSG_EVENT, MODULE_NAME,
                 "Blacklisting '%s' from search", keyword);
    }
    if (bl_words)
    {
        eina_list_free(bl_words);
        bl_words = NULL;
    }

    /* set file download destinations */
    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/.enna/%s",
             enna_util_user_home_get(), PATH_COVERS);
    valhalla_config_set(vh, DOWNLOADER_DEST, dst, VALHALLA_DL_COVER);

    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/.enna/%s",
             enna_util_user_home_get(), PATH_FANARTS);
    valhalla_config_set(vh, DOWNLOADER_DEST, dst, VALHALLA_DL_FAN_ART);

    /* grabbers */
    value = get_lang();
    if (value)
    {
        if (strcmp(value, "fr"))
            valhalla_config_set(vh, GRABBER_STATE, "allocine", 0);
        free(value);
    }

    rc = valhalla_run(vh, scan_loop, scan_sleep, scan_priority);
    if (rc)
    {
        enna_log(ENNA_MSG_ERROR,
                 MODULE_NAME, "valhalla returns error code: %i", rc);
        valhalla_uninit(vh);
        vh = NULL;
        goto err;
    }

    enna_log(ENNA_MSG_EVENT, MODULE_NAME, "Valkyries are running");
    return;

 err:
    enna_log(ENNA_MSG_ERROR,
             MODULE_NAME, "valhalla module initialization");
    if (bl_words)
        eina_list_free(bl_words);
    if (path)
        eina_list_free(path);
}

static void
enna_metadata_db_uninit(void)
{
    valhalla_uninit(vh);
    vh = NULL;
}

void
enna_metadata_init(void)
{
    char dst[1024];

    /* try to create backdrops directory storage */
    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/.enna/%s",
             enna_util_user_home_get(), PATH_FANARTS);
    if (!ecore_file_is_dir(dst))
        ecore_file_mkdir(dst);

    /* try to create covers directory storage */
    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/.enna/%s",
             enna_util_user_home_get(), PATH_COVERS);
    if (!ecore_file_is_dir(dst))
        ecore_file_mkdir(dst);

    /* init database and scanner */
    enna_metadata_db_init();
}

void
enna_metadata_shutdown(void)
{
    /* uninit database and scanner */
    enna_metadata_db_uninit();
}

void *
enna_metadata_get_db(void)
{
    return vh;
}

Enna_Metadata *
enna_metadata_meta_new(const char *file)
{
  valhalla_db_filemeta_t *m = NULL;
  int shift = 0;

  if (!vh || !file)
      return NULL;

  if (!strncmp(file, "file://", 7))
      shift = 7;

  enna_log (ENNA_MSG_EVENT,
            MODULE_NAME, "Request for metadata on %s", file + shift);
  valhalla_db_file_get(vh, 0, file + shift, NULL, &m);

  return (Enna_Metadata *) m;
}

void
enna_metadata_meta_free(Enna_Metadata *meta)
{
    valhalla_db_filemeta_t *m = (void *) meta;

    if (m)
        VALHALLA_DB_FILEMETA_FREE(m);
}

char *
enna_metadata_meta_get(Enna_Metadata *meta, const char *name, int max)
{
  valhalla_db_filemeta_t *m, *n;
  int count = 0;
  buffer_t *b;
  char *str = NULL;

  if (!meta || !name)
      return NULL;

  b = buffer_new();
  m = (valhalla_db_filemeta_t *) meta;
  n = m;

  while (n)
  {
      if (n->meta_name && !strcmp(n->meta_name, name))
      {
          if (count == 0)
              buffer_append(b, n->data_value);
          else
              buffer_appendf(b, ", %s", n->data_value);
          count++;
          if (count >= max)
              break;
      }
      n = n->next;
  }

  str = b->buf ? strdup(b->buf) : NULL;
  if (str)
      enna_log(ENNA_MSG_EVENT, MODULE_NAME,
               "Requested metadata '%s' is associated to value '%s'",
               name, str);
  buffer_free(b);

  return str;
}

char *
enna_metadata_meta_get_all(Enna_Metadata *meta)
{
  valhalla_db_filemeta_t *m, *n;
  buffer_t *b;
  char *str = NULL;

  if (!meta)
      return NULL;

  b = buffer_new();
  m = (valhalla_db_filemeta_t *) meta;
  n = m;

  while (n)
  {
      buffer_appendf(b, "%s: %s\n", n->meta_name, n->data_value);
      n = n->next;
  }

  str = b->buf ? strdup(b->buf) : NULL;
  buffer_free(b);

  return str;
}

void
enna_metadata_set_position(Enna_Metadata *meta, double position)
{
    /* to be implemented */
}

void
enna_metadata_ondemand(const char *file)
{
  if (!vh || !file)
    return;

  valhalla_ondemand(vh, file);
}

char *
enna_metadata_meta_duration_get(Enna_Metadata *m)
{
    buffer_t *buf;
    char *runtime = NULL, *length;
    char *duration = NULL;

    if (!m)
        return NULL;

    buf = buffer_new();

    length = enna_metadata_meta_get(m, "duration", 1);
    if (!length)
    {
        runtime = enna_metadata_meta_get(m, "runtime", 1);
        length = enna_metadata_meta_get(m, "length", 1);
    }

    /* special hack for nfo files which already have a computed duration */
    if (length && strstr(length, "mn"))
    {
        duration = strdup(length);
        goto end;
    }

    if (runtime || length)
    {
        int hh = 0, mm = 0;

        if (runtime)
        {
            hh = (int) (atoi(runtime) / 60);
            mm = (int) (atoi(runtime) - 60 * hh);
        }
        else if (length)
        {
            hh = (int) (atoi(length) / 3600 / 1000);
            mm = (int) ((atoi(length) / 60 / 1000) - (60 * hh));
        }

        if (hh)
            buffer_appendf(buf, ngettext("%.2d hour ", "%.2d hours ", hh), hh);
        if (hh && mm)
            buffer_append(buf, " ");
        if (mm)
            buffer_appendf(buf,
                           ngettext("%.2d minute", "%.2d minutes", mm), mm);

        duration = buf->buf ? strdup(buf->buf) : NULL;
    }
    else
        duration = NULL;

end:
    ENNA_FREE(runtime);
    ENNA_FREE(length);
    buffer_free(buf);

    return duration;
}
