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
#include <Eet.h>

#include <valhalla.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "logs.h"
#include "utils.h"
#include "events_stack.h"

#define MODULE_NAME "enna"

#define ENNA_METADATA_DB_NAME                        "media.db"
#define ENNA_METADATA_DEFAULT_PARSER_NUMBER           2
#define ENNA_METADATA_DEFAULT_COMMIT_INTERVAL         128
#define ENNA_METADATA_DEFAULT_SCAN_LOOP              -1
#define ENNA_METADATA_DEFAULT_SCAN_SLEEP              900
#define ENNA_METADATA_DEFAULT_SCAN_PRIORITY           19

#define PATH_BACKDROPS          "backdrops"
#define PATH_COVERS             "covers"
#define PATH_METADATA           "metadata"
#define PATH_SNAPSHOTS          "snapshots"

#define PATH_BUFFER 4096

#define DEBUG 0
#define EET_DO_COMPRESS 1

static Eina_List *metadata_grabbers = NULL;
static Eet_File *ef;
static pthread_t grabber_thread_id = -1;
static events_stack_t *estack = NULL;
static valhalla_t *vh = NULL;

extern Enna *enna;

#if DEBUG == 1
static void
enna_metadata_video_dump (Enna_Metadata_Video *m)
{
    if (!m)
        return;

    printf ("*** Metadata Video:\n");
    printf (" -- Codec: %s\n",      m->codec);
    printf (" -- Width: %d\n",      m->width);
    printf (" -- Height: %d\n",     m->height);
    printf (" -- Aspect: %f\n",     m->aspect);
    printf (" -- Channels: %d\n",   m->channels);
    printf (" -- Streams: %d\n",    m->streams);
    printf (" -- FrameRate: %f\n",  m->framerate);
    printf (" -- BitRate: %d\n",    m->bitrate);
}

static void
enna_metadata_music_dump (Enna_Metadata_Music *m)
{
    if (!m)
        return;

    printf ("*** Metadata Music:\n");
    printf (" -- Artist: %s\n",      m->artist);
    printf (" -- Album: %s\n",       m->album);
    printf (" -- Year: %s\n",        m->year);
    printf (" -- Genre: %s\n",       m->genre);
    printf (" -- Comment: %s\n",     m->comment);
    printf (" -- DiscID: %s\n",      m->discid);
    printf (" -- Track: %d\n",       m->track);
    printf (" -- Rating: %d\n",      m->rating);
    printf (" -- Play_Count: %d\n",  m->play_count);
    printf (" -- Codec: %s\n",       m->codec);
    printf (" -- Bitrate: %d\n",     m->bitrate);
    printf (" -- Channels: %d\n",    m->channels);
    printf (" -- SampleRate: %d\n",  m->samplerate);
}

static void
enna_metadata_dump (Enna_Metadata *m)
{
    if (!m)
        return;

    printf ("*** Metadata:\n");
    printf (" -- Type: %d\n",              m->type);
    printf (" -- URI: %s\n",               m->uri);
    printf (" -- MD5: %s\n",               m->md5);
    printf (" -- Keywords: %s\n",          m->keywords);
    printf (" -- Title: %s\n",             m->title);
    printf (" -- Alternative title: %s\n", m->alternative_title);
    printf (" -- Size: %d\n",              m->size);
    printf (" -- Length: %d\n",            m->length);
    printf (" -- Position: %f\n",          m->position);
    printf (" -- Overview: %s\n",          m->overview);
    printf (" -- Runtime: %d\n",           m->runtime);
    printf (" -- Year: %d\n",              m->year);
    printf (" -- Categories: %s\n",        m->categories);
    printf (" -- Rating (0/5): %d\n",      m->rating);
    printf (" -- Budget: %d$\n",           m->budget);
    printf (" -- Country: %s\n",           m->country);
    printf (" -- Writer: %s\n",            m->writer);
    printf (" -- Director: %s\n",          m->director);
    printf (" -- Actors: %s\n",            m->actors);
    printf (" -- Studio: %s\n",            m->studio);
    printf (" -- Lyrics: %s\n",            m->lyrics);
    printf (" -- Cover: %s\n",             m->cover);
    printf (" -- Snapshot: %s\n",          m->snapshot);
    printf (" -- Backdrop: %s\n",          m->backdrop);
    printf (" -- Parsed: %d\n",            m->parsed);

    enna_metadata_video_dump (m->video);
    enna_metadata_music_dump (m->music);
}
#endif

static Eina_Hash *
eet_eina_hash_add (Eina_Hash *hash, const char *key, const void *data)
{
    if (!hash)
        hash = eina_hash_string_superfast_new (NULL);

    if (!hash)
        return NULL;

    eina_hash_add (hash, key, data);
    return hash;
}

#define EDD_NEW(str) \
  eet_data_descriptor_new (#str, sizeof (str), \
                           (void *) eina_list_next, \
                           (void *) eina_list_append, \
                           (void *) eina_list_data_get, \
                           (void *) eina_list_free, \
                           (void *) eina_hash_foreach, \
                           (void *) eet_eina_hash_add, \
                           (void *) eina_hash_free)

#define EDD_ADD(str, field, type) \
  EET_DATA_DESCRIPTOR_ADD_BASIC (edd, Enna_Metadata##str, \
                                 #field, field, EET_T_##type)

static Eet_Data_Descriptor *
enna_metadata_video_desc (void)
{
    Eet_Data_Descriptor *edd;

    edd = EDD_NEW (Enna_Metadata_Video);

    EDD_ADD (_Video, codec,     STRING);
    EDD_ADD (_Video, width,     INT);
    EDD_ADD (_Video, height,    INT);
    EDD_ADD (_Video, aspect,    FLOAT);
    EDD_ADD (_Video, channels,  INT);
    EDD_ADD (_Video, streams,   INT);
    EDD_ADD (_Video, framerate, FLOAT);
    EDD_ADD (_Video, bitrate,   INT);

    return edd;
}

static Eet_Data_Descriptor *
enna_metadata_music_desc (void)
{
    Eet_Data_Descriptor *edd;

    edd = EDD_NEW (Enna_Metadata_Music);

    EDD_ADD (_Music, artist,     STRING);
    EDD_ADD (_Music, album,      STRING);
    EDD_ADD (_Music, year,       STRING);
    EDD_ADD (_Music, genre,      STRING);
    EDD_ADD (_Music, comment,    STRING);
    EDD_ADD (_Music, discid,     STRING);
    EDD_ADD (_Music, track,      INT);
    EDD_ADD (_Music, rating,     INT);
    EDD_ADD (_Music, play_count, INT);
    EDD_ADD (_Music, codec,      STRING);
    EDD_ADD (_Music, bitrate,    INT);
    EDD_ADD (_Music, channels,   INT);
    EDD_ADD (_Music, samplerate, INT);

    return edd;
}

static Eet_Data_Descriptor *
enna_metadata_desc (void)
{
    Eet_Data_Descriptor *edd, *edd_video, *edd_music;

    edd = EDD_NEW (Enna_Metadata);

    EDD_ADD (, type,        INT);
    EDD_ADD (, uri,         STRING);
    EDD_ADD (, md5,         STRING);
    EDD_ADD (, keywords,    STRING);
    EDD_ADD (, title,       STRING);
    EDD_ADD (, alternative_title,  STRING);
    EDD_ADD (, size,        LONG_LONG);
    EDD_ADD (, length,      INT);
    EDD_ADD (, position,    DOUBLE);
    EDD_ADD (, overview,    STRING);
    EDD_ADD (, runtime,     INT);
    EDD_ADD (, year,        INT);
    EDD_ADD (, categories,  STRING);
    EDD_ADD (, rating,      INT);
    EDD_ADD (, budget,      INT);
    EDD_ADD (, country,     STRING);
    EDD_ADD (, writer,      STRING);
    EDD_ADD (, director,    STRING);
    EDD_ADD (, actors,      STRING);
    EDD_ADD (, studio,      STRING);
    EDD_ADD (, lyrics,      STRING);
    EDD_ADD (, cover,       STRING);
    EDD_ADD (, snapshot,    STRING);
    EDD_ADD (, backdrop,    STRING);
    EDD_ADD (, parsed,      INT);

    edd_video = enna_metadata_video_desc ();
    EET_DATA_DESCRIPTOR_ADD_SUB (edd, Enna_Metadata,
                                 "video", video, edd_video);

    edd_music = enna_metadata_music_desc ();
    EET_DATA_DESCRIPTOR_ADD_SUB (edd, Enna_Metadata,
                                 "music", music, edd_music);

    return edd;
}

static Enna_Metadata *
enna_metadata_load_from_eet (char *md5)
{
    Enna_Metadata *m = NULL;
    Eet_Data_Descriptor *edd;
    char file[1024];

    if (!md5)
        return NULL;

    if (!enna->metadata_cache)
        return NULL;

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Trying to load %s from EET.", md5);

    memset (file, '\0', sizeof (file));
    snprintf (file, sizeof (file), "%s/.enna/%s/%s.eet",
              enna_util_user_home_get (), PATH_METADATA, md5);

    ef = eet_open (file, EET_FILE_MODE_READ);
    if (!ef)
        return NULL;

    edd = enna_metadata_desc ();
    m = eet_data_read (ef, edd, md5);
    if (!m)
    {
        eet_data_descriptor_free (edd);
        return NULL;
    }

#if DEBUG == 1
    enna_metadata_dump (m);
#endif

    eet_data_descriptor_free (edd);
    eet_close (ef);

    return m;
}

static void
enna_metadata_save_to_eet (Enna_Metadata *m)
{
    Eet_Data_Descriptor *edd;
    char file[1024];

    if (!m)
        return;

    if (!enna->metadata_cache)
        return;

    memset (file, '\0', sizeof (file));
    snprintf (file, sizeof (file), "%s/.enna/%s/%s.eet",
              enna_util_user_home_get (), PATH_METADATA, m->md5);

    ecore_file_unlink (file);
    ef = eet_open (file, EET_FILE_MODE_WRITE);
    if (!ef)
        return;

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Trying to save %s to EET.", m->md5);

    edd = enna_metadata_desc ();
    if (!eet_data_write (ef, edd, m->md5, m, EET_DO_COMPRESS))
        enna_log (ENNA_MSG_WARNING, MODULE_NAME,
                  "Error writing EET data.");

    eet_data_descriptor_free (edd);
    eet_close (ef);
}

Enna_Metadata *
enna_metadata_new (char *uri)
{
    Enna_Metadata *m;
    char *md5;

    if (!uri)
      return NULL;

    md5 = md5sum (uri);
    m = enna_metadata_load_from_eet (md5);
    free (md5);
    if (m)
        return m;

    m = calloc(1, sizeof(Enna_Metadata));
    m->video = calloc(1, sizeof(Enna_Metadata_Video));
    m->music = calloc(1, sizeof(Enna_Metadata_Music));

    m->type = ENNA_METADATA_UNKNOWN;
    m->uri = strdup (uri);
    m->md5 = md5sum (uri);
    m->parsed = 0;

    return m;
}

void enna_metadata_free(Enna_Metadata *m)
{
    if (!m)
        return;

    ENNA_FREE(m->uri);
    ENNA_FREE(m->md5);
    ENNA_FREE(m->keywords);
    ENNA_FREE(m->title);
    ENNA_FREE(m->alternative_title); 
    ENNA_FREE(m->overview);
    ENNA_FREE(m->categories);
    ENNA_FREE(m->country);
    ENNA_FREE(m->writer);
    ENNA_FREE(m->director);
    ENNA_FREE(m->actors);
    ENNA_FREE(m->studio);
    ENNA_FREE(m->lyrics);
    ENNA_FREE(m->cover);
    ENNA_FREE(m->snapshot);
    ENNA_FREE(m->backdrop);
    if (m->video)
    {
        ENNA_FREE(m->video->codec);
        free(m->video);
    }
    if (m->music)
    {
        ENNA_FREE(m->music->artist);
        ENNA_FREE(m->music->album);
        ENNA_FREE(m->music->year);
        ENNA_FREE(m->music->genre);
        ENNA_FREE(m->music->comment);
        ENNA_FREE(m->music->discid);
        ENNA_FREE(m->music->codec);
        free(m->music);
    }
    free(m);
}

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

    /* try to create metadata directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_METADATA);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* try to create snapshots directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_SNAPSHOTS);
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

void
enna_metadata_add_keywords (Enna_Metadata *meta, char *keywords)
{
    char key[1024];

    if (!meta || !keywords)
        return;

    if (!meta->keywords)
        meta->keywords = strdup (keywords);
    else
    {
        memset (key, '\0', sizeof (key));
        snprintf (key, sizeof (key), "%s %s", meta->keywords, keywords);
        free (meta->keywords);
        meta->keywords = strdup (key);
    }

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Metadata keywords set to '%s'", meta->keywords);
}

void
enna_metadata_add_actors (Enna_Metadata *meta, char *actor)
{
    char act[1024];

    if (!meta || !actor)
        return;

    if (!meta->actors)
        meta->actors = strdup (actor);
    else
    {
        memset (act, '\0', sizeof (act));
        snprintf (act, sizeof (act), "%s, %s", meta->actors, actor);
        free (meta->actors);
        meta->actors = strdup (act);
    }

    enna_log (ENNA_MSG_EVENT, MODULE_NAME,
              "Metadata actors set to '%s'", meta->actors);
}

void
enna_metadata_add_category (Enna_Metadata *meta, char *category)
{
    char cat[1024];

    if (!meta || !category)
        return;

    /* check that the category hasn't already been added to list */
    if (meta->categories && strstr (meta->categories, category))
      return;

    if (!meta->categories)
        meta->categories = strdup (category);
    else
    {
        memset (cat, '\0', sizeof (cat));
        snprintf (cat, sizeof (cat), "%s, %s", meta->categories, category);
        free (meta->categories);
        meta->categories = strdup (cat);
    }
}

void
enna_metadata_add_grabber (Enna_Metadata_Grabber *grabber)
{
    Eina_List *tmp;

    if (!grabber || !grabber->name)
        return;

    tmp = eina_list_nth_list (metadata_grabbers, 0);
    do {
        Enna_Metadata_Grabber *g = NULL;

        if (tmp)
            g = (Enna_Metadata_Grabber *) tmp->data;

        if (g && !strcmp (g->name, grabber->name))
            return; /* already added grabber */
    } while ((tmp = eina_list_next (tmp)));

    metadata_grabbers = eina_list_append (metadata_grabbers, grabber);
}

void
enna_metadata_remove_grabber (char *name)
{
    Eina_List *tmp;

    if (!name)
        return;

    tmp = eina_list_nth_list (metadata_grabbers, 0);
    do {
        Enna_Metadata_Grabber *g = NULL;

        if (tmp)
            g = (Enna_Metadata_Grabber *) tmp->data;

        if (g && !strcmp (g->name, name))
        {
            tmp = eina_list_remove (tmp, g);
            return;
        }
    } while ((tmp = eina_list_next (tmp)));
}

void
enna_metadata_grab (Enna_Metadata *meta, int caps)
{
    int i;

    if (!meta)
        return;

    /* avoid parsing the same resource twice */
    if (meta->parsed)
        return;

    /* do not grab metadata from non-local streams */
    if (strncmp (meta->uri, "file://", 7))
      return;

    for (i = ENNA_GRABBER_PRIORITY_MAX; i < ENNA_GRABBER_PRIORITY_MIN; i++)
    {
        Eina_List *tmp;

        tmp = eina_list_nth_list (metadata_grabbers, 0);
        do {
            Enna_Metadata_Grabber *g = NULL;

            if (tmp)
                g = (Enna_Metadata_Grabber *) tmp->data;
            if (!g)
                continue;

            /* check for grabber's priority */
            if (g->priority != i)
                continue;

            /* check if network is allowed */
            if (g->require_network && !enna->use_network)
                continue;

            /* check if grabber has the requested capabilities */
            if (g->caps & caps)
                g->grab (meta, caps);
        } while ((tmp = eina_list_next (tmp)));
    }

    meta->parsed = 1;
    enna_metadata_save_to_eet (meta);
}

void
enna_metadata_set_position (Enna_Metadata *meta, double position)
{
    if (!meta)
        return;

    meta->position = position;
    enna_metadata_save_to_eet (meta);
}


void *
grabber_thread(void *arg)
{
    event_elt_t * evt = NULL;
    Enna_Metadata_Request *r = NULL;
    
    estack = events_stack_create();
    if (!estack)
    {
        enna_log (ENNA_MSG_ERROR, MODULE_NAME, "grabber thread cannot allocate events stack");
        return NULL; 
    }
    
    for (;;) 
    {
        unsigned int addr;
        r = NULL;

        evt = events_stack_pop_event(estack);
        if (!evt)
            continue;
        
        r = (Enna_Metadata_Request*) evt->data;
        
        free (evt);
        
        if (!r)
            continue;
        
        enna_metadata_grab (r->metadata, r->caps);
        
        addr = (unsigned int )(r->metadata);
        ecore_pipe_write (enna->pipe_grabber, &addr, sizeof (unsigned int*));
	free (r);
    }
    
    events_stack_destroy(estack);
    return NULL;
}

int 
grabber_start_thread(void)
{
    if (grabber_thread_id != -1)
        return -1;
    
    if (pthread_create(&grabber_thread_id, NULL, grabber_thread, NULL)) 
    {
        enna_log (ENNA_MSG_ERROR, MODULE_NAME, "Cannot start grabber thread - cause : %s", strerror(errno));
        return -1;
    }
    return 0;
}

int 
grabber_stop_thread(void)
{
    if (grabber_thread_id  == -1)
      return 0;
    
    if (pthread_cancel(grabber_thread_id)) 
    {
        enna_log (ENNA_MSG_ERROR, MODULE_NAME, "grabber thread cannot be stopped - cause : %s", strerror(errno));
        return -1;
    }
    
    return 0;
}


int 
enna_metadata_grab_request(Enna_Metadata_Request *r)
{
    event_elt_t *e = NULL;

    e = calloc(sizeof(event_elt_t), 1);
    e->data = r;
    return events_stack_push_event(estack, e);
}
