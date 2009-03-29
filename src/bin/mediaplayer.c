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

#include <string.h>

#include <Ecore.h>
#include <Ecore_Data.h>

#include "enna.h"
#include "enna_config.h"
#include "mediaplayer.h"
#include "module.h"
#include "logs.h"

typedef struct list_item_s
{
    const char *uri;
    const char *label;
} list_item_t;


typedef struct _Enna_Mediaplayer Enna_Mediaplayer;

struct _Enna_Mediaplayer
{
    PLAY_STATE play_state;
    Enna_Class_MediaplayerBackend *class;
};

static Enna_Mediaplayer *_mediaplayer;

static void
_event_cb(void *data, enna_mediaplayer_event_t event);

/* externally accessible functions */
int
enna_mediaplayer_init(void)
{
    Enna_Module *em;
    char *backend_name = NULL;

    if (!strcmp(enna_config->backend, "emotion"))
    {
#ifdef BUILD_BACKEND_EMOTION
        enna_log (ENNA_MSG_INFO, NULL, "Using Emotion Backend");
        backend_name = "emotion";
#else
        enna_log(ENNA_MSG_ERROR, NULL, "Backend selected not built !");
        return -1;
#endif
    }
    else if (!strcmp(enna_config->backend, "libplayer"))
    {
#ifdef BUILD_BACKEND_LIBPLAYER
        enna_log (ENNA_MSG_INFO, NULL, "Using libplayer Backend");
        backend_name = "libplayer";
#else
        enna_log(ENNA_MSG_ERROR, NULL, "Backend selected not built !");
        return -1;
#endif
    }
    else
    {
        enna_log(ENNA_MSG_ERROR, NULL,
                 "Unknown backend (%s)!", enna_config->backend);
        return -1;
    }

    _mediaplayer = calloc(1, sizeof(Enna_Mediaplayer));
    em = enna_module_open(backend_name, ENNA_MODULE_BACKEND, enna->evas);
    enna_module_enable(em);
    _mediaplayer->play_state = STOPPED;

    /* Create Ecore Event ID */
    ENNA_EVENT_MEDIAPLAYER_EOS = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_METADATA_UPDATE = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_START = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_STOP = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_PAUSE = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_UNPAUSE = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_PREV = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_NEXT = ecore_event_type_new();
    ENNA_EVENT_MEDIAPLAYER_SEEK = ecore_event_type_new();
    return 0;
}

void
enna_mediaplayer_shutdown(void)
{
    free(_mediaplayer);
}

void
enna_mediaplayer_uri_append(Enna_Playlist *enna_playlist,const char *uri, const char *label)
{
    list_item_t *item = calloc(1, sizeof(list_item_t));
    item->uri = uri ? strdup(uri) : NULL;
    item->label = label ? strdup(label) : NULL;
    enna_playlist->playlist = eina_list_append(enna_playlist->playlist, item);
}

int
enna_mediaplayer_play(Enna_Playlist *enna_playlist)
{
    if (!_mediaplayer->class)
    {
      enna_log(ENNA_MSG_CRITICAL, NULL, "mediaplayer class not registered!");
      return -1;
    }
    switch (_mediaplayer->play_state)
    {
        case STOPPED:
        {
            list_item_t *item;
            item = eina_list_nth(enna_playlist->playlist, enna_playlist->selected);
            if (_mediaplayer->class->func.class_stop)
                _mediaplayer->class->func.class_stop();
            if (item && item->uri && _mediaplayer->class->func.class_file_set)
                _mediaplayer->class->func.class_file_set(item->uri, item->label);
            if (_mediaplayer->class->func.class_play)
                _mediaplayer->class->func.class_play();
            _mediaplayer->play_state = PLAYING;
            ecore_event_add(ENNA_EVENT_MEDIAPLAYER_START, NULL, NULL, NULL);
        }
            break;
        case PLAYING:
            enna_mediaplayer_pause();
            break;
        case PAUSE:

            if (_mediaplayer->class->func.class_play)
                _mediaplayer->class->func.class_play();
            _mediaplayer->play_state = PLAYING;
            ecore_event_add(ENNA_EVENT_MEDIAPLAYER_UNPAUSE, NULL, NULL, NULL);
            break;
        default:
            break;

    }

    return 0;
}

int
enna_mediaplayer_select_nth(Enna_Playlist *enna_playlist,int n)
{
    list_item_t *item;
    if (n < 0 || n > eina_list_count(enna_playlist->playlist) - 1)
        return -1;
    item = eina_list_nth(enna_playlist->playlist, n);
    enna_log(ENNA_MSG_EVENT, NULL, "select %d", n);
    if (item && item->uri && _mediaplayer->class && _mediaplayer->class->func.class_file_set)
        _mediaplayer->class->func.class_file_set(item->uri, item->label);
    enna_playlist->selected = n;
    return 0;
}

int
enna_mediaplayer_selected_get(Enna_Playlist *enna_playlist)
{
    return enna_playlist->selected;
}

int
enna_mediaplayer_stop(void)
{

    if (_mediaplayer->class)
    {
        if (_mediaplayer->class->func.class_stop)
            _mediaplayer->class->func.class_stop();
        _mediaplayer->play_state = STOPPED;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);
    }
    return 0;
}

int
enna_mediaplayer_pause(void)
{
    if (_mediaplayer->class)
    {
        if (_mediaplayer->class->func.class_pause)
            _mediaplayer->class->func.class_pause();
        _mediaplayer->play_state = PAUSE;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_PAUSE, NULL, NULL, NULL);
    }
    return 0;
}

int
enna_mediaplayer_next(Enna_Playlist *enna_playlist)
{
    list_item_t *item;

    enna_playlist->selected++;
    if (enna_playlist->selected > eina_list_count(enna_playlist->playlist) - 1)
    {
        enna_playlist->selected--;
        return -1;
    }
    item = eina_list_nth(enna_playlist->playlist, enna_playlist->selected);
    enna_log(ENNA_MSG_EVENT, NULL, "select %d", enna_playlist->selected);
    if (item)
    {
        enna_mediaplayer_stop();
        if (item->uri && _mediaplayer->class && _mediaplayer->class->func.class_file_set)
            _mediaplayer->class->func.class_file_set(item->uri, item->label);
        enna_mediaplayer_play(enna_playlist);
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_NEXT, NULL, NULL, NULL);
    }

    return 0;
}

int
enna_mediaplayer_prev(Enna_Playlist *enna_playlist)
{
    list_item_t *item;

    enna_playlist->selected--;
    if (enna_playlist->selected < 0)
    {
            enna_playlist->selected = 0;
        return -1;
    }
    item = eina_list_nth(enna_playlist->playlist, enna_playlist->selected);
    enna_log(ENNA_MSG_EVENT, NULL, "select %d", enna_playlist->selected);
    if (item)
    {
        enna_mediaplayer_stop();
        if (item->uri && _mediaplayer->class && _mediaplayer->class->func.class_file_set)
            _mediaplayer->class->func.class_file_set(item->uri, item->label);
        enna_mediaplayer_play(enna_playlist);
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_PREV, NULL, NULL, NULL);
    }
    return 0;
}

double
enna_mediaplayer_position_get(void)
{
    if (_mediaplayer->play_state == PAUSE || _mediaplayer->play_state
            == PLAYING)
    {
        if (_mediaplayer->class && _mediaplayer->class->func.class_position_get)
            return _mediaplayer->class->func.class_position_get();
    }
    return 0.0;
}

double
enna_mediaplayer_length_get(void)
{
    if (_mediaplayer->play_state == PAUSE || _mediaplayer->play_state
            == PLAYING)
    {
        if (_mediaplayer->class && _mediaplayer->class->func.class_length_get)
            return _mediaplayer->class->func.class_length_get();
    }
    return 0.0;
}

int
enna_mediaplayer_seek(double percent)
{
    enna_log(ENNA_MSG_EVENT, NULL, "Seeking to: %d%%", (int) (100 * percent));
    if (_mediaplayer->play_state == PAUSE || _mediaplayer->play_state
            == PLAYING)
    {
            Enna_Event_Mediaplayer_Seek_Data *ev;
            ev = calloc(1, sizeof(Enna_Event_Mediaplayer_Seek_Data));
            if (!ev)
                return 0;
            ev->seek_value=percent;
            ecore_event_add(ENNA_EVENT_MEDIAPLAYER_SEEK, ev, NULL, NULL);
            if (_mediaplayer->class && _mediaplayer->class->func.class_seek)
                return _mediaplayer->class->func.class_seek(percent);
    }
    return 0;
}

void
enna_mediaplayer_video_resize(int x, int y, int w, int h)
{
    if (_mediaplayer->class && _mediaplayer->class->func.class_video_resize)
        _mediaplayer->class->func.class_video_resize(x, y, w, h);
}

int
enna_mediaplayer_playlist_load(const char *filename)
{
    return 0;
}

int
enna_mediaplayer_playlist_save(const char *filename)
{
    return 0;
}

void
enna_mediaplayer_playlist_clear(Enna_Playlist *enna_playlist)
{
    eina_list_free(enna_playlist->playlist);
    enna_playlist->playlist = NULL;
    enna_playlist->selected = 0;
}

Enna_Metadata *
enna_mediaplayer_metadata_get(Enna_Playlist *enna_playlist)
{
    list_item_t *item;
    item = eina_list_nth(enna_playlist->playlist, enna_playlist->selected);

    if (!item)
        return NULL;

    if (item->uri)
        return enna_metadata_new ((char *) item->uri);

    return NULL;
}

int
enna_mediaplayer_playlist_count(Enna_Playlist *enna_playlist)
{
    return eina_list_count(enna_playlist->playlist);
}

int
enna_mediaplayer_backend_register(Enna_Class_MediaplayerBackend *class)
{
    if (!class || !_mediaplayer)
        return -1;
    _mediaplayer->class = class;
    if (class->func.class_init)
        class->func.class_init(0);

    if (class->func.class_event_cb_set)
        class->func.class_event_cb_set(_event_cb, NULL);

    return 0;
}

Evas_Object *
enna_mediaplayer_video_obj_get(void)
{
    if (_mediaplayer->class->func.class_video_obj_get)
    return _mediaplayer->class->func.class_video_obj_get();
    else
    return NULL;
}

static void
_event_cb(void *data, enna_mediaplayer_event_t event)
{
    switch (event)
    {
        case ENNA_MP_EVENT_EOF:
            enna_log(ENNA_MSG_EVENT, NULL, "End of stream");
            ecore_event_add(ENNA_EVENT_MEDIAPLAYER_EOS, NULL, NULL, NULL);
            break;
        default:
            break;
    }
}

PLAY_STATE
enna_mediaplayer_state_get(void)
{
    return(_mediaplayer->play_state);
}

Enna_Playlist *
enna_mediaplayer_playlist_create(void)
{
    Enna_Playlist  *enna_playlist = calloc(1, sizeof(Enna_Playlist));
    enna_playlist->selected=0;
    enna_playlist->playlist=NULL;
    return enna_playlist;
}

void
enna_mediaplayer_playlist_free(Enna_Playlist *enna_playlist)
{
    eina_list_free(enna_playlist->playlist);
    free(enna_playlist);
}

void
enna_mediaplayer_playlist_stop_clear(Enna_Playlist *enna_playlist)
{
    enna_mediaplayer_playlist_clear(enna_playlist);
    if (_mediaplayer->class)
    {
        if (_mediaplayer->class->func.class_stop)
            _mediaplayer->class->func.class_stop();
        _mediaplayer->play_state = STOPPED;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);
    }
}

void
enna_mediaplayer_send_key(enna_key_t key)
{
    if (_mediaplayer->class)
    {
        if (_mediaplayer->class->func.class_send_key)
            _mediaplayer->class->func.class_send_key(key);
    }
}
