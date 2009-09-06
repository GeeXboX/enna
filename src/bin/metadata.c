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
#define ENNA_METADATA_DEFAULT_COMMIT_INTERVAL         128
#define ENNA_METADATA_DEFAULT_SCAN_LOOP              -1
#define ENNA_METADATA_DEFAULT_SCAN_SLEEP              900
#define ENNA_METADATA_DEFAULT_SCAN_PRIORITY           19

#define PATH_BACKDROPS          "backdrops"
#define PATH_COVERS             "covers"

#define PATH_BUFFER 4096

static valhalla_t *vh = NULL;

static void
enna_metadata_db_init (void)
{
    int rc;
    Enna_Config_Data *cfgdata;
    char *value = NULL;
    char db[PATH_BUFFER];
    Eina_List *path = NULL, *l;
    Eina_List *music_ext = NULL, *video_ext = NULL, *photo_ext = NULL;
    int parser_number   = ENNA_METADATA_DEFAULT_PARSER_NUMBER;
    int commit_interval = ENNA_METADATA_DEFAULT_COMMIT_INTERVAL;
    int scan_loop       = ENNA_METADATA_DEFAULT_SCAN_LOOP;
    int scan_sleep      = ENNA_METADATA_DEFAULT_SCAN_SLEEP;
    int scan_priority   = ENNA_METADATA_DEFAULT_SCAN_PRIORITY;
    valhalla_verb_t verbosity = VALHALLA_MSG_WARNING;
    char dst[1024];

    cfgdata = enna_config_module_pair_get("enna");
    if (cfgdata)
    {
        Eina_List *list;
        for (list = cfgdata->pair; list; list = list->next)
        {
            Config_Pair *pair = list->data;
            enna_config_value_store (&music_ext, "music_ext",
                                     ENNA_CONFIG_STRING_LIST, pair);
            enna_config_value_store (&video_ext, "video_ext",
                                     ENNA_CONFIG_STRING_LIST, pair);
            enna_config_value_store (&photo_ext, "photo_ext",
                                     ENNA_CONFIG_STRING_LIST, pair);
        }
    }

    cfgdata = enna_config_module_pair_get("valhalla");
    if (cfgdata)
    {
        Eina_List *list;

        for (list = cfgdata->pair; list; list = list->next)
        {
            Config_Pair *pair = list->data;

            enna_config_value_store(&parser_number, "parser_number",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&commit_interval, "commit_interval",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_loop, "scan_loop",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_sleep, "scan_sleep",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_priority, "scan_priority",
                                    ENNA_CONFIG_INT, pair);

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
    enna_log(ENNA_MSG_INFO,
             MODULE_NAME, "* parser number  : %i", parser_number);
    enna_log(ENNA_MSG_INFO,
             MODULE_NAME, "* commit interval: %i", commit_interval);
    enna_log(ENNA_MSG_INFO,
             MODULE_NAME, "* scan loop      : %i", scan_loop);
    enna_log(ENNA_MSG_INFO,
             MODULE_NAME, "* scan sleep     : %i", scan_sleep);
    enna_log(ENNA_MSG_INFO,
             MODULE_NAME, "* scan priority  : %i", scan_priority);
    enna_log(ENNA_MSG_INFO,
             MODULE_NAME, "* verbosity      : %i", verbosity);

    valhalla_verbosity(verbosity);

    snprintf(db, sizeof(db),
             "%s/.enna/%s", enna_util_user_home_get(), ENNA_METADATA_DB_NAME);

    vh = valhalla_init(db, parser_number, 1, commit_interval);
    if (!vh)
        goto err;

    /* Add file suffixes */
    for (l = music_ext; l; l = l->next)
    {
        const char *ext = l->data;
        valhalla_suffix_add(vh, ext);
    }
    if (music_ext)
    {
        eina_list_free(music_ext);
        music_ext = NULL;
    }

    for (l = video_ext; l; l = l->next)
    {
        const char *ext = l->data;
        valhalla_suffix_add(vh, ext);
    }
    if (video_ext)
    {
        eina_list_free(video_ext);
        video_ext = NULL;
    }

    for (l = photo_ext; l; l = l->next)
    {
        const char *ext = l->data;
        valhalla_suffix_add(vh, ext);
    }
    if (photo_ext)
    {
        eina_list_free(photo_ext);
        photo_ext = NULL;
    }

    /* Add paths */
    for (l = path; l; l = l->next)
    {
        const char *str = l->data;
        valhalla_path_add(vh, str, 1);
    }
    if (path)
    {
        eina_list_free(path);
        path = NULL;
    }

    /* set file download destinations */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_COVERS);
    valhalla_downloader_dest_set (vh, VALHALLA_DL_COVER, dst);

    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_BACKDROPS);
    valhalla_downloader_dest_set (vh, VALHALLA_DL_BACKDROP, dst);

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
    if (music_ext)
        eina_list_free(music_ext);
    if (video_ext)
        eina_list_free(video_ext);
    if (photo_ext)
        eina_list_free(photo_ext);
    if (path)
        eina_list_free(path);
}

static void
enna_metadata_db_uninit (void)
{
    valhalla_uninit (vh);
    vh = NULL;
}

void
enna_metadata_init (void)
{
    char dst[1024];

    /* try to create backdrops directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_BACKDROPS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* try to create covers directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_COVERS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* init database and scanner */
    enna_metadata_db_init ();
}

void
enna_metadata_shutdown (void)
{
    /* uninit database and scanner */
    enna_metadata_db_uninit ();
}

void *
enna_metadata_get_db (void)
{
    return &vh;
}

Enna_Metadata *
enna_metadata_meta_new (const char *file)
{
  valhalla_db_filemeta_t *m = NULL;
  int shift = 0;

  if (!vh || !file)
      return NULL;

  if (!strncmp (file, "file://", 7))
      shift = 7;

  enna_log (ENNA_MSG_EVENT,
            MODULE_NAME, "Request for metadata on %s", file + shift);
  valhalla_db_file_get (vh, 0, file + shift, NULL, &m);

  return (Enna_Metadata *) m;
}

void
enna_metadata_meta_free (Enna_Metadata *meta)
{
    valhalla_db_filemeta_t *m = (void *) meta;

    if (m)
        VALHALLA_DB_FILEMETA_FREE (m);
}

char *
enna_metadata_meta_get (Enna_Metadata *meta, const char *name, int max)
{
  valhalla_db_filemeta_t *m, *n;
  int count = 0;
  buffer_t *b;
  char *str = NULL;

  if (!meta || !name)
      return NULL;

  b = buffer_new ();
  m = (valhalla_db_filemeta_t *) meta;
  n = m;

  while (n)
  {
      if (n->meta_name && !strcmp (n->meta_name, name))
      {
          if (count == 0)
              buffer_append (b, n->data_value);
          else
              buffer_appendf (b, ", %s", n->data_value);
          count++;
          if (count >= max)
              break;
      }
      n = n->next;
  }

  str = b->buf ? strdup (b->buf) : NULL;
  if (str)
      enna_log (ENNA_MSG_EVENT, MODULE_NAME,
                "Requested metadata '%s' is associated to value '%s'",
                name, str);
  buffer_free (b);

  return str;
}

void
enna_metadata_set_position (Enna_Metadata *meta, double position)
{
    /* to be implemented */
}
