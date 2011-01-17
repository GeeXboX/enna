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

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "enna.h"
#include "metadata.h"
#include "input.h"

typedef enum
{
    ENNA_MP_EVENT_EOF,
} enna_mediaplayer_event_t;

typedef enum
{
    ENNA_MP_URI_TYPE_CDDA,
    ENNA_MP_URI_TYPE_DVD,
    ENNA_MP_URI_TYPE_DVDNAV,
    ENNA_MP_URI_TYPE_FTP,
    ENNA_MP_URI_TYPE_HTTP,
    ENNA_MP_URI_TYPE_MMS,
    ENNA_MP_URI_TYPE_NETVDR,
    ENNA_MP_URI_TYPE_RTP,
    ENNA_MP_URI_TYPE_RTSP,
    ENNA_MP_URI_TYPE_SMB,
    ENNA_MP_URI_TYPE_TCP,
    ENNA_MP_URI_TYPE_UDP,
    ENNA_MP_URI_TYPE_UNSV,
    ENNA_MP_URI_TYPE_VDR,
    ENNA_MP_URI_TYPE_SPOTIFY,
} enna_mediaplayer_uri_type_t;

typedef enum _PLAY_STATE PLAY_STATE;

enum _PLAY_STATE
{
    PLAYING,
    PAUSE,
    STOPPED
};

typedef struct _Enna_Event_Mediaplayer_Seek_Data Enna_Event_Mediaplayer_Seek_Data;

typedef enum _SEEK_TYPE SEEK_TYPE;

enum _SEEK_TYPE
{
    SEEK_ABS_PERCENT = 0,
    SEEK_ABS_SECONDS,
    SEEK_REL_SECONDS
};

struct _Enna_Event_Mediaplayer_Seek_Data
{
    int seek_value;
    SEEK_TYPE type;
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
int enna_mediaplayer_supported_uri_type(enna_mediaplayer_uri_type_t type);
void enna_mediaplayer_cfg_register(void);
int enna_mediaplayer_init(void);
void enna_mediaplayer_shutdown(void);
void enna_mediaplayer_file_append(Enna_Playlist *enna_playlist, Enna_File *file);
int enna_mediaplayer_select_nth(Enna_Playlist *enna_playlist, int n);
int enna_mediaplayer_selected_get(Enna_Playlist *enna_playlist);
Enna_Metadata *enna_mediaplayer_metadata_get(Enna_Playlist *enna_playlist);
int enna_mediaplayer_play(Enna_Playlist *enna_playlist);
int enna_mediaplayer_stop(void);
int enna_mediaplayer_pause(void);
int enna_mediaplayer_next(Enna_Playlist *enna_playlist);
int enna_mediaplayer_prev(Enna_Playlist *enna_playlist);
double enna_mediaplayer_position_get(void);
int enna_mediaplayer_position_percent_get(void);
void enna_mediaplayer_position_set(int seconds);
double enna_mediaplayer_length_get(void);
void enna_mediaplayer_seek_percent(int percent);
void enna_mediaplayer_seek_relative(int seconds);
void enna_mediaplayer_default_seek_backward (void);
void enna_mediaplayer_default_seek_forward (void);
void enna_mediaplayer_video_resize(int x, int y, int w, int h);
int enna_mediaplayer_playlist_load(const char *filename);
int enna_mediaplayer_playlist_save(const char *filename);
void enna_mediaplayer_playlist_clear(Enna_Playlist *enna_playlist);
int enna_mediaplayer_playlist_count(Enna_Playlist *enna_playlist);
PLAY_STATE enna_mediaplayer_state_get(void);
Enna_Playlist *enna_mediaplayer_playlist_create(void);
void enna_mediaplayer_playlist_free(Enna_Playlist *enna_playlist);
void enna_mediaplayer_playlist_stop_clear(Enna_Playlist *enna_playlist);
void enna_mediaplayer_send_input(enna_input event);
int enna_mediaplayer_volume_get(void);
void enna_mediaplayer_volume_set(int volume);
void enna_mediaplayer_default_increase_volume(void);
void enna_mediaplayer_default_decrease_volume(void);
void enna_mediaplayer_mute(void);
int enna_mediaplayer_mute_get(void);
Enna_File *enna_mediaplayer_current_file_get(void);
const char *enna_mediaplayer_get_current_uri(void);
void enna_mediaplayer_audio_previous(void);
void enna_mediaplayer_audio_next(void);
void enna_mediaplayer_audio_increase_delay(void);
void enna_mediaplayer_audio_decrease_delay(void);
void enna_mediaplayer_subtitle_set_visibility(void);
void enna_mediaplayer_subtitle_previous(void);
void enna_mediaplayer_subtitle_next(void);
void enna_mediaplayer_subtitle_set_alignment(void);
void enna_mediaplayer_subtitle_increase_position(void);
void enna_mediaplayer_subtitle_decrease_position(void);
void enna_mediaplayer_subtitle_increase_scale(void);
void enna_mediaplayer_subtitle_decrease_scale(void);
void enna_mediaplayer_subtitle_increase_delay(void);
void enna_mediaplayer_subtitle_decrease_delay(void);
void enna_mediaplayer_set_framedrop(void);
#endif /* MEDIAPLAYER_H */
