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

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "enna.h"
#include "metadata.h"

typedef enum
{
    ENNA_BACKEND_EMOTION,
    ENNA_BACKEND_LIBPLAYER,
} enna_mediaplayer_backend_t;

typedef enum
{
    ENNA_MP_EVENT_EOF,
} enna_mediaplayer_event_t;

typedef enum _PLAY_STATE PLAY_STATE;

enum _PLAY_STATE
{
    PLAYING,
    PAUSE,
    STOPPED
};

typedef struct _Enna_Event_Mediaplayer_Seek_Data Enna_Event_Mediaplayer_Seek_Data;

struct _Enna_Event_Mediaplayer_Seek_Data
{
    double seek_value;
};

typedef struct _Enna_Class_MediaplayerBackend Enna_Class_MediaplayerBackend;

struct _Enna_Class_MediaplayerBackend
{
    const char *name;
    int pri;
    struct
    {
        void (*class_init)(int dummy);
        void (*class_shutdown)(int dummy);
        int (*class_file_set)(const char *uri, const char *label);
        int (*class_play)(void);
        int (*class_seek)(double percent);
        int (*class_stop)(void);
        int (*class_pause)(void);
        double (*class_position_get)(void);
        double (*class_length_get)(void);
        void (*class_video_resize)(int x, int y, int w, int h);
        void (*class_event_cb_set)(void (*event_cb)(void*data, enna_mediaplayer_event_t event), void *data);
        Evas_Object *(*class_video_obj_get)(void);
    } func;
};

typedef struct _Enna_Playlist Enna_Playlist;

struct _Enna_Playlist
{
    int selected;
    Eina_List *playlist;
};

/* Mediaplayer event */
int ENNA_EVENT_MEDIAPLAYER_EOS;
int ENNA_EVENT_MEDIAPLAYER_METADATA_UPDATE;
int ENNA_EVENT_MEDIAPLAYER_START;
int ENNA_EVENT_MEDIAPLAYER_STOP;
int ENNA_EVENT_MEDIAPLAYER_PAUSE;
int ENNA_EVENT_MEDIAPLAYER_UNPAUSE;
int ENNA_EVENT_MEDIAPLAYER_PREV;
int ENNA_EVENT_MEDIAPLAYER_NEXT;
int ENNA_EVENT_MEDIAPLAYER_SEEK;

/* Mediaplayer API functions */
int enna_mediaplayer_init(void);
void enna_mediaplayer_shutdown(void);
void enna_mediaplayer_uri_append(Enna_Playlist *enna_playlist,const char *uri, const char *label);
int enna_mediaplayer_select_nth(Enna_Playlist *enna_playlist,int n);
int enna_mediaplayer_selected_get(Enna_Playlist *enna_playlist);
Enna_Metadata *enna_mediaplayer_metadata_get(Enna_Playlist *enna_playlist);
int enna_mediaplayer_play(Enna_Playlist *enna_playlist);
int enna_mediaplayer_stop(void);
int enna_mediaplayer_pause(void);
int enna_mediaplayer_next(Enna_Playlist *enna_playlist);
int enna_mediaplayer_prev(Enna_Playlist *enna_playlist);
double enna_mediaplayer_position_get(void);
double enna_mediaplayer_length_get(void);
int enna_mediaplayer_seek(double percent);
void enna_mediaplayer_video_resize(int x, int y, int w, int h);
int enna_mediaplayer_playlist_load(const char *filename);
int enna_mediaplayer_playlist_save(const char *filename);
void enna_mediaplayer_playlist_clear(Enna_Playlist *enna_playlist);
int enna_mediaplayer_playlist_count(Enna_Playlist *enna_playlist);
Evas_Object *enna_mediaplayer_video_obj_get(void);
int
        enna_mediaplayer_backend_register(Enna_Class_MediaplayerBackend *class);
PLAY_STATE enna_mediaplayer_state_get(void);
Enna_Playlist *enna_mediaplayer_playlist_create(void);
void enna_mediaplayer_playlist_free(Enna_Playlist *enna_playlist);
void enna_mediaplayer_playlist_stop_clear(Enna_Playlist *enna_playlist);
#endif /* MEDIAPLAYER_H */
