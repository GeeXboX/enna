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
#define ENNA_METADATA_DEFAULT_SCAN_DELAY              4
#define ENNA_METADATA_DEFAULT_SCAN_PRIORITY           19
#define ENNA_METADATA_DEFAULT_DECRAPIFIER             1

#define ENNA_METADATA_DEFAULT_KEYWORDS "0tv,1080p,2hd,720p,ac3,booya,caph,crimson,ctu,dimension,divx,dot,dsr,dvdrip,dvdscr,e7,etach,fov,fqm,hdq,hdtv,lol,mainevent,notv,orenji,pdtv,proper,pushercrew,repack,reseed,screencam,screener,sys,vtv,x264,xor,xvid,cdNUM,CDNUM,SExEP,sSEeEP,SSEEEP"

#define ENNA_METADATA_DEFAULT_GRABBERS "allocine,amazon,chartlyrics,exif,ffmpeg,imdb,lastfm,local,lyricwiki,nfo,tmdb,tvdb,tvrage"

#define PATH_FANARTS            "fanarts"
#define PATH_COVERS             "covers"

#define PATH_BUFFER 4096

typedef struct db_cfg_s {
    Eina_List *path;
    Eina_List *bl_words;
    Eina_List *grabbers;
    int parser_number;
    int grabber_number;
    int commit_interval;
    int scan_loop;
    int scan_sleep;
    int scan_delay;
    int scan_priority;
    int decrapifier;
    valhalla_verb_t verbosity;
} db_cfg_t;

struct _Enna_Metadata
{
    struct _Enna_Metadata *next;
    char *meta;
    char *data;
};

typedef struct _Enna_Pipe_Data Enna_Pipe_Data;

struct _Enna_Pipe_Data
{
    char *file;
    Enna_Metadata_OnDemand ev;
};

static db_cfg_t db_cfg;
static valhalla_t *vh = NULL;
static Ecore_Pipe *vh_pipe;
static Eina_List *od_files = NULL;

#define SUFFIX_ADD(type)                                       \
    for (l = enna_config->type; l; l = l->next)                \
    {                                                          \
        const char *ext = l->data;                             \
        valhalla_config_set(vh, SCANNER_SUFFIX, ext);          \
    }                                                          \

static void
pipe_read(void *data, void *buf, unsigned int nbyte)
{
    Enna_Pipe_Data *od;
    Eina_List *l;
    Enna_File *file;

    if (!buf)
        return;

    memcpy(&od, buf, nbyte);

    if (od->ev == ENNA_METADATA_OD_GRABBED)
    {
        EINA_LIST_FOREACH(od_files, l, file)
        {
            if (!strcmp(file->mrl+7, od->file))
            {
                enna_file_meta_callback_call(file);
            }
        }
    }
    free(od->file);
    free(od);
}

static void
ondemand_cb(const char *file, valhalla_event_od_t e, const char *id, void *data)
{
    Enna_Pipe_Data *od;
    Enna_Metadata_OnDemand ev = ENNA_METADATA_OD_PARSED;

    if (!file)
        return;

    switch (e)
    {
    case VALHALLA_EVENTOD_PARSED:
        enna_log(ENNA_MSG_EVENT,
                 MODULE_NAME, _("[%s]: parsing done."), file);
        break;
    case VALHALLA_EVENTOD_GRABBED:
        ev = ENNA_METADATA_OD_GRABBED;
        enna_log(ENNA_MSG_EVENT,
                 MODULE_NAME, _("[%s]: %s grabber has finished"), file, id);
        break;
    case VALHALLA_EVENTOD_ENDED:
        ev = ENNA_METADATA_OD_ENDED;
        enna_log(ENNA_MSG_INFO,
                 MODULE_NAME, _("[%s]: all metadata have been fetched."), file);
        break;
    }

    od = calloc(1, sizeof(Enna_Pipe_Data));
    if (!od)
        return;

    od->file = strdup(file);
    od->ev   = ev;

    ecore_pipe_write(vh_pipe, &od, sizeof(od));
}

#define CFG_INT(field)                                                \
    v = enna_config_int_get(section, #field);                         \
    if (v) db_cfg.field = v;

static const struct {
    const char *name;
    valhalla_verb_t verb;
} map_vh_verbosity[] = {
    { "none",        VALHALLA_MSG_NONE         },
    { "verbose",     VALHALLA_MSG_VERBOSE      },
    { "info",        VALHALLA_MSG_INFO         },
    { "warning",     VALHALLA_MSG_WARNING      },
    { "error",       VALHALLA_MSG_ERROR        },
    { "critical",    VALHALLA_MSG_CRITICAL     },
    { NULL,          VALHALLA_MSG_NONE         }
};

static void
cfg_db_section_load(const char *section)
{
    const char *value;
    Eina_List *vl, *gl;
    int v;

    vl = enna_config_string_list_get(section, "path");
    if (vl)
    {
        Eina_List *l;
        char *c;

        EINA_LIST_FREE(db_cfg.path, c)
            ENNA_FREE(c);

        EINA_LIST_FOREACH(vl, l, c)
            db_cfg.path = eina_list_append(db_cfg.path, strdup(c));
    }

    value = enna_config_string_get(section, "blacklist_keywords");
    if (value)
    {
        char *c;

        EINA_LIST_FREE(db_cfg.bl_words, c)
            ENNA_FREE(c);

        db_cfg.bl_words = enna_util_tuple_get(value, ",");
    }

    gl = enna_config_string_list_get(section, "grabber");
    if (gl)
    {
        Eina_List *l;
        char *c;

        EINA_LIST_FREE(db_cfg.grabbers, c)
            ENNA_FREE(c);

        EINA_LIST_FOREACH(gl, l, c)
            db_cfg.grabbers = eina_list_append(db_cfg.grabbers, c);
    }

    CFG_INT(parser_number);
    CFG_INT(grabber_number);
    CFG_INT(commit_interval);
    CFG_INT(scan_loop);
    CFG_INT(scan_sleep);
    CFG_INT(scan_delay);
    CFG_INT(scan_priority);
    CFG_INT(decrapifier);

    value = enna_config_string_get(section, "verbosity");
    if (value)
    {
        int i;

        for (i = 0; map_vh_verbosity[i].name; i++)
            if (!strcmp(value, map_vh_verbosity[i].name))
            {
                db_cfg.verbosity = map_vh_verbosity[i].verb;
                break;
            }
    }

    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* parser number  : %i", db_cfg.parser_number);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* grabber number : %i", db_cfg.grabber_number);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* commit interval: %i", db_cfg.commit_interval);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan loop      : %i", db_cfg.scan_loop);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan sleep     : %i", db_cfg.scan_sleep);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan delay     : %i", db_cfg.scan_delay);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* scan priority  : %i", db_cfg.scan_priority);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* decrapifier    : %i", !!db_cfg.decrapifier);
    enna_log(ENNA_MSG_EVENT,
             MODULE_NAME, "* verbosity      : %i", db_cfg.verbosity);
}

#define CFG_INT_SET(field)                                                \
    enna_config_int_set(section, #field, db_cfg.field);

static void
cfg_db_section_save(const char *section)
{
    char *words;
    int i;

    enna_config_string_list_set(section, "path", db_cfg.path);

    words = enna_util_tuple_set(db_cfg.bl_words, ",");
    enna_config_string_set(section, "blacklist_keywords", words);
    ENNA_FREE(words);

    enna_config_string_list_set(section, "grabber", db_cfg.grabbers);

    CFG_INT_SET(parser_number);
    CFG_INT_SET(grabber_number);
    CFG_INT_SET(commit_interval);
    CFG_INT_SET(scan_loop);
    CFG_INT_SET(scan_sleep);
    CFG_INT_SET(scan_delay);
    CFG_INT_SET(scan_priority);
    CFG_INT_SET(decrapifier);

    for (i = 0; map_vh_verbosity[i].name; i++)
        if (db_cfg.verbosity == map_vh_verbosity[i].verb)
        {
            enna_config_string_set(section, "verbosity",
                                   map_vh_verbosity[i].name);
            break;
        }
}

static void
cfg_db_free (void)
{
    char *c;

    EINA_LIST_FREE(db_cfg.path, c)
      ENNA_FREE(c);

    EINA_LIST_FREE(db_cfg.bl_words, c)
      ENNA_FREE(c);

    EINA_LIST_FREE(db_cfg.grabbers, c)
      ENNA_FREE(c);
}

static void
cfg_db_section_set_default (void)
{
    cfg_db_free();

    db_cfg.path            = NULL;
    db_cfg.bl_words        = NULL;
    db_cfg.grabbers        = NULL;
    db_cfg.parser_number   = ENNA_METADATA_DEFAULT_PARSER_NUMBER;
    db_cfg.grabber_number  = ENNA_METADATA_DEFAULT_GRABBER_NUMBER;
    db_cfg.commit_interval = ENNA_METADATA_DEFAULT_COMMIT_INTERVAL;
    db_cfg.scan_loop       = ENNA_METADATA_DEFAULT_SCAN_LOOP;
    db_cfg.scan_sleep      = ENNA_METADATA_DEFAULT_SCAN_SLEEP;
    db_cfg.scan_delay      = ENNA_METADATA_DEFAULT_SCAN_DELAY;
    db_cfg.scan_priority   = ENNA_METADATA_DEFAULT_SCAN_PRIORITY;
    db_cfg.decrapifier     = ENNA_METADATA_DEFAULT_DECRAPIFIER;
    db_cfg.verbosity       = VALHALLA_MSG_WARNING;

    /* set the blacklisted keywords list */
    db_cfg.bl_words = enna_util_tuple_get(ENNA_METADATA_DEFAULT_KEYWORDS, ",");

    /* set the enabled grabbers list */
    db_cfg.grabbers = enna_util_tuple_get(ENNA_METADATA_DEFAULT_GRABBERS, ",");
}

static Enna_Config_Section_Parser cfg_db = {
    "media_db",
    cfg_db_section_load,
    cfg_db_section_save,
    cfg_db_section_set_default,
    cfg_db_free,
};

void
enna_metadata_cfg_register (void)
{
    enna_config_section_parser_register(&cfg_db);
}

static void
enna_metadata_db_init(void)
{
    int rc;
    char *value = NULL;
    char db[PATH_BUFFER];
    Eina_List *l;
    valhalla_init_param_t param;
    char dst[1024];
    const char *grabber = NULL;
    Eina_List *glist = NULL;

    valhalla_verbosity(db_cfg.verbosity);

    snprintf(db, sizeof(db),
             "%s/%s", enna_util_data_home_get(), ENNA_METADATA_DB_NAME);

    memset(&param, 0, sizeof(valhalla_init_param_t));
    param.parser_nb   = db_cfg.parser_number;
    param.grabber_nb  = db_cfg.grabber_number;
    param.commit_int  = db_cfg.commit_interval;
    param.decrapifier = !!db_cfg.decrapifier;
    param.od_cb       = ondemand_cb;

    vh = valhalla_init(db, &param);
    if (!vh)
        goto err;

    /* Add file suffixes */
    SUFFIX_ADD(music_filters);
    SUFFIX_ADD(video_filters);
    SUFFIX_ADD(photo_filters);

    /* Add paths */
    for (l = db_cfg.path; l; l = l->next)
    {
        const char *str = l->data;

        if (strstr(str, "file://") == str)
            str += 7;
        valhalla_config_set(vh, SCANNER_PATH, str, 1);
    }

    /* blacklist some keywords */
    for (l = db_cfg.bl_words; l; l = l->next)
    {
        const char *keyword = l->data;
        valhalla_config_set(vh, PARSER_KEYWORD, keyword);
        enna_log(ENNA_MSG_EVENT, MODULE_NAME,
                 "Blacklisting '%s' from search", keyword);
    }

    /* fetch list of available grabbers and disable them all by default */
    while ((grabber = valhalla_grabber_next(vh, grabber)))
    {
        valhalla_config_set(vh, GRABBER_STATE, grabber, 0);
        glist = eina_list_append(glist, grabber);
    }

    /* enable requested grabbers */
    for (l = db_cfg.grabbers; l; l = l->next)
    {
        const char *g = l->data;
        const char *gl;
        Eina_List *l;

        EINA_LIST_FOREACH(glist, l, gl)
        {
            if (!strcmp(g, gl))
            {
                valhalla_config_set(vh, GRABBER_STATE, g, 1);

                enna_log(ENNA_MSG_INFO, MODULE_NAME,
                         "Enabling grabber '%s'", g);
                break;
            }
        }
    }

    /* set file download destinations */
    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/%s",
             enna_util_data_home_get(), PATH_COVERS);
    valhalla_config_set(vh, DOWNLOADER_DEST, dst, VALHALLA_DL_COVER);

    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/%s",
             enna_util_data_home_get(), PATH_FANARTS);
    valhalla_config_set(vh, DOWNLOADER_DEST, dst, VALHALLA_DL_FAN_ART);

    /* lang-specific grabbers workaround */
    value = get_lang();
    if (value)
    {
        if (strcmp(value, "fr"))
            valhalla_config_set(vh, GRABBER_STATE, "allocine", 0);
        free(value);
    }

    rc = valhalla_run(vh, db_cfg.scan_loop, db_cfg.scan_sleep,
                      db_cfg.scan_delay, db_cfg.scan_priority);
    if (rc)
    {
        enna_log(ENNA_MSG_ERROR,
                 MODULE_NAME, "valhalla returns error code: %i", rc);
        valhalla_uninit(vh);
        vh = NULL;
        goto err;
    }

    vh_pipe = ecore_pipe_add(pipe_read, NULL);

    enna_log(ENNA_MSG_EVENT, MODULE_NAME, "Valkyries are running");
    return;

 err:
    enna_log(ENNA_MSG_ERROR,
             MODULE_NAME, "valhalla module initialization");
}

static void
enna_metadata_db_uninit(void)
{
    Enna_File *file;

    valhalla_uninit(vh);
    vh = NULL;

    if (vh_pipe)
    {
        ecore_pipe_del(vh_pipe);
        vh_pipe = NULL;
    }

    EINA_LIST_FREE(od_files, file)
        enna_file_free(file);
    od_files = NULL;
}

void
enna_metadata_init(void)
{
    char dst[1024];

    /* try to create backdrops directory storage */
    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/%s",
             enna_util_data_home_get(), PATH_FANARTS);
    if (!ecore_file_is_dir(dst))
        ecore_file_mkdir(dst);

    /* try to create covers directory storage */
    memset(dst, '\0', sizeof(dst));
    snprintf(dst, sizeof(dst), "%s/%s",
             enna_util_data_home_get(), PATH_COVERS);
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
  Enna_Metadata *m = NULL, *it = NULL;
  valhalla_db_stmt_t *stmt;
  const valhalla_db_metares_t *metares;
  int shift = 0;

  if (!vh || !file)
      return NULL;

  if (!strncmp(file, "file://", 7))
      shift = 7;

  enna_log (ENNA_MSG_EVENT,
            MODULE_NAME, "Request for metadata on %s", file + shift);
  stmt = valhalla_db_file_get(vh, 0, file + shift, NULL);
  if (!stmt)
      return NULL;

  while ((metares = valhalla_db_file_read(vh, stmt)))
  {
    Enna_Metadata *new;
    new = calloc(1, sizeof(Enna_Metadata));
    if (!new)
        continue;

    new->meta = strdup(metares->meta_name);
    new->data = strdup(metares->data_value);

    if (m)
    {
        it->next = new;
        it = it->next;
    }
    else
        m = it = new;
  }

  return m;
}

void
enna_metadata_meta_free(Enna_Metadata *meta)
{
    Enna_Metadata *next;

    if (!meta)
        return;

    while (meta)
    {
        next = meta->next;
        if (meta->meta)
            free(meta->meta);
        if (meta->data)
            free(meta->data);
        free(meta);
        meta = next;
    }
}

const char *
enna_metadata_meta_get(const Enna_Metadata *meta, const char *name, int max)
{
  int count = 0;
  Enna_Buffer *b;
  const char *str = NULL;

  if (!meta || !name)
      return NULL;

  b = enna_buffer_new();

  for (; meta; meta = meta->next)
      if (meta->meta && !strcmp(meta->meta, name))
      {
          if (count == 0)
              enna_buffer_append(b, meta->data);
          else
              enna_buffer_appendf(b, ", %s", meta->data);
          count++;
          if (count >= max)
              break;
      }

  str = b->buf ? eina_stringshare_add(b->buf) : NULL;
  if (str)
      enna_log(ENNA_MSG_EVENT, MODULE_NAME,
               "Requested metadata '%s' is associated to value '%s'",
               name, str);
  enna_buffer_free(b);

  return str;
}

void
enna_metadata_meta_set(Enna_Metadata *meta, Enna_File *file, const char *name, const char *data)
{
    if (!meta || !file || !file->mrl || !name || !data)
        return;

    for (; meta; meta = meta->next)
    {
        if (meta->meta && !strcmp(meta->meta, name))
        {
            valhalla_db_metadata_update(vh, file->mrl + 7,
                                        name, meta->data, data,
                                        VALHALLA_LANG_UNDEF);
            return;
        }
    }


    valhalla_db_metadata_insert(vh, file->mrl + 7,
                                name, data, VALHALLA_LANG_UNDEF,
                                VALHALLA_META_GRP_MISCELLANEOUS);

}

const char *
enna_metadata_meta_get_all(const Enna_Metadata *meta)
{
  Enna_Buffer *b;
  const char *str = NULL;

  if (!meta)
      return NULL;

  b = enna_buffer_new();

  for (; meta; meta = meta->next)
      enna_buffer_appendf(b, "%s: %s\n", meta->meta, meta->data);

  str = b->buf ? eina_stringshare_add(b->buf) : NULL;
  enna_buffer_free(b);

  return str;
}

void
enna_metadata_set_position(Enna_Metadata *meta, double position)
{
    /* to be implemented */
}

void
enna_metadata_ondemand_add(Enna_File *file)
{
    const char *uri;

    if (!vh || !file || !file->mrl)
        return;

    uri = file->mrl;
    if (!strncmp(uri, "file://", 7))
        uri += 7;

    /* Add file to the list of on demand files */
    od_files = eina_list_append(od_files, file);
    valhalla_ondemand(vh, uri);
}

void
enna_metadata_ondemand_del(Enna_File *file)
{
  if (!vh || !file)
    return;

  /* Add file to the list of on demand files */
  od_files = eina_list_remove(od_files, file);
}


char *
enna_metadata_meta_duration_get(const Enna_Metadata *m)
{
    Enna_Buffer *buf;
    const char *runtime = NULL, *length;
    char *duration = NULL;

    if (!m)
        return NULL;

    buf = enna_buffer_new();

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
            enna_buffer_appendf(buf, ngettext("%.2d hour ", "%.2d hours ", hh), hh);
        if (hh && mm)
            enna_buffer_append(buf, " ");
        if (mm)
            enna_buffer_appendf(buf,
                           ngettext("%.2d minute", "%.2d minutes", mm), mm);

        duration = buf->buf ? strdup(buf->buf) : NULL;
    }
    else
        duration = NULL;

end:
    eina_stringshare_del(runtime);
    eina_stringshare_del(length);
    enna_buffer_free(buf);

    return duration;
}
