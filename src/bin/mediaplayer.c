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

#include <sys/types.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Input.h>

#include <player.h>

#include "enna.h"
#include "enna_config.h"
#include "mediaplayer.h"
#include "module.h"
#include "logs.h"
#include "input.h"
#include "utils.h"
#include "browser.h"

#define URI_TYPE_CDDA     "cdda://"
#define URI_TYPE_DVD      "dvd://"
#define URI_TYPE_DVDNAV   "dvdnav://"
#define URI_TYPE_FTP      "ftp://"
#define URI_TYPE_HTTP     "http://"
#define URI_TYPE_MMS      "mms://"
#define URI_TYPE_NETVDR   "netvdr://"
#define URI_TYPE_RTP      "rtp://"
#define URI_TYPE_RTSP     "rtsp://"
#define URI_TYPE_SMB      "smb://"
#define URI_TYPE_TCP      "tcp://"
#define URI_TYPE_UDP      "udp://"
#define URI_TYPE_UNSV     "unsv://"
#define URI_TYPE_VDR      "vdr:/"
#define URI_TYPE_SPOTIFY  "spotify:"
#define MAX_PLAYERS 5

/* a/v controls */
#define SEEK_STEP_DEFAULT         10 /* seconds */
#define VOLUME_STEP_DEFAULT       5 /* percent */
#define AUDIO_DELAY_DEFAULT       0
#define SUB_VISIBILITY_DEFAULT    1
#define SUB_ALIGNMENT_DEFAULT     PLAYER_SUB_ALIGNMENT_BOTTOM
#define SUB_POSITION_DEFAULT      100
#define SUB_SCALE_DEFAULT         5
#define SUB_DELAY_DEFAULT         0
#define FRAMEDROP_DEFAULT         PLAYER_FRAMEDROP_DISABLE

typedef struct mediaplayer_cfg_s {
    player_type_t type;
    player_type_t dvd_type;
    player_type_t tv_type;
    player_type_t spotify_type;
    player_vo_t vo;
    player_ao_t ao;
    player_verbosity_level_t verbosity;
    int dvd_set;
    int tv_set;
    int spotify_set;
    char *sub_align;
    char *sub_pos;
    char *sub_scale;
    char *sub_visibility;
    char *framedrop;
} mediaplayer_cfg_t;

typedef struct _Enna_Mediaplayer Enna_Mediaplayer;

struct _Enna_Mediaplayer
{
    PLAY_STATE play_state;
    player_t *player;
    player_t *players[MAX_PLAYERS];
    void (*event_cb)(void *data, enna_mediaplayer_event_t event);
    void *event_cb_data;
    char *uri;
    char *label;
    int audio_delay;
    int subtitle_visibility;
    int subtitle_alignment;
    int subtitle_position;
    int subtitle_scale;
    int subtitle_delay;
    int framedrop;
    player_type_t player_type;
    player_type_t default_type;
    player_type_t dvd_type;
    player_type_t tv_type;
    player_type_t spotify_type;
    Ecore_Event_Handler *key_down_event_handler;
    Ecore_Event_Handler *mouse_button_event_handler;
    Ecore_Event_Handler *mouse_move_event_handler;
    Ecore_Pipe *pipe;
    Enna_Playlist *cur_playlist;
};

static Enna_Mediaplayer *mp = NULL;
static mediaplayer_cfg_t mp_cfg;

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

static void
pipe_read(void *data, void *buf, unsigned int nbyte)
{
    enna_mediaplayer_event_t *event = buf;

    if (!mp->event_cb || !buf)
        return;

    mp->event_cb(mp->event_cb_data, *event);
}

static int
event_cb(player_event_t e, void *data)
{
    enna_mediaplayer_event_t event;

    if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
    {
        event = ENNA_MP_EVENT_EOF;
        ecore_pipe_write(mp->pipe, &event, sizeof(event));
    }

    return 0;
}

static Eina_Bool
event_key_down(void *data, int type, void *event)
{
    Ecore_Event_Key *e;
    e = event;
    /*
       HACK !
       If e->window is the same than enna winid, don't manage this event
       ecore_evas_x will do this for us.
       But if e->window is different than enna winid event are sent to
       libplayer subwindow and we must broadcast this event to Evas
    */
    if (e->window != enna->ee_winid)
    {
        enna_log(ENNA_MSG_EVENT, NULL,
                 "Ecore_Event_Key_Down %s", e->keyname);
        evas_event_feed_key_down(enna->evas, e->keyname, e->key,
                                 e->compose, NULL, e->timestamp, NULL);
    }

    return 1;
}

static Eina_Bool
event_mouse_button(void *data, int type, void *event)
{
    mrl_resource_t res;
    Ecore_Event_Mouse_Button *e = event;

    /* Broadcast mouse position only for dvd player and only
       if libplayer window is on screen */
    if ((e->window == enna->ee_winid) || !mp->uri)
        return 1;

    res = mrl_get_resource(mp->player, NULL);
    if (res != MRL_RESOURCE_DVDNAV)
        return 1;

    /* Set mouse position and send mouseclick event */
    enna_log(ENNA_MSG_EVENT, NULL,
             "Send Mouse click %d %d, uri : %s", e->x, e->y, mp->uri);
    player_set_mouse_position(mp->players[mp->dvd_type], e->x, e->y);
    player_dvd_nav(mp->players[mp->dvd_type], PLAYER_DVDNAV_MOUSECLICK);
    return 1;
}

static Eina_Bool
event_mouse_move(void *data, int type, void *event)
{
    mrl_resource_t res;
    Ecore_Event_Mouse_Move *e = event;

    /* Broadcast mouse position only for dvd player and only
       if libplayer window is on screen */
    if ((e->window == enna->ee_winid) || !mp->uri)
        return 1;

    res = mrl_get_resource(mp->player, NULL);
    if (res != MRL_RESOURCE_DVDNAV)
        return 1;

    /* Send mouse position to libplayer */
    player_set_mouse_position(mp->players[mp->dvd_type], e->x, e->y);
    return 1;
}

static mrl_t *
set_network_stream(const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_network_args_t *args;

    enna_log(ENNA_MSG_INFO, NULL, "Load Network Stream : %s", uri);

    args = calloc(1, sizeof(mrl_resource_network_args_t));
    args->url = strdup(uri);

    if (type == MRL_RESOURCE_NETVDR)
        mrl = mrl_new(mp->players[mp->tv_type], type, args);
    else
        mrl = mrl_new(mp->players[mp->default_type], type, args);

    return mrl;
}

static mrl_t *
set_dvd_stream(const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_videodisc_args_t *args;
    char *meta;
    uint32_t prop = 0;
    int tmp = 0;
    int title = 0;
    char *device;

    enna_log(ENNA_MSG_INFO, NULL, "Load DVD Video : %s", uri);

    args = calloc(1, sizeof(mrl_resource_videodisc_args_t));
    device = strstr(uri, "://");
    if (device)
        args->device = strdup(device + 3);

    mrl = mrl_new(mp->players[mp->dvd_type], type, args);

    meta = mrl_get_metadata_dvd(mp->players[mp->dvd_type],
                                mrl, (uint8_t *) &prop);
    if (meta)
    {
        enna_log(ENNA_MSG_INFO, NULL, "Meta DVD VolumeID: %s", meta);
        free(meta);
    }

    if (prop)
    {
        int i;

        enna_log(ENNA_MSG_INFO, NULL, "Meta DVD Titles: %i", prop);

        for (i = 1; i <= prop; i++)
        {
            uint32_t chapters, angles, length;

            chapters =
                mrl_get_metadata_dvd_title(mp->players[mp->dvd_type], mrl, i,
                                           MRL_METADATA_DVD_TITLE_CHAPTERS);
            angles =
                mrl_get_metadata_dvd_title(mp->players[mp->dvd_type], mrl, i,
                                           MRL_METADATA_DVD_TITLE_ANGLES);
            length =
                mrl_get_metadata_dvd_title(mp->players[mp->dvd_type], mrl, i,
                                           MRL_METADATA_DVD_TITLE_LENGTH);

            enna_log(ENNA_MSG_INFO, NULL,
                     "Meta DVD Title %i (%.2f sec), " \
                     "Chapters: %i, Angles: %i",
                     i, length / 1000.0, chapters, angles);
            if (length > tmp)
            {
                tmp = length;
                title = i;
            }
        }
    }
    args->title_start = title;

    return mrl;
}

static mrl_t *
set_tv_stream(const char *device, const char *driver, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_tv_args_t *args;

    args = calloc(1, sizeof(mrl_resource_tv_args_t));

    if (type == MRL_RESOURCE_VDR)
    {
        enna_log(ENNA_MSG_INFO, NULL,
                 "VDR stream; device: '%s' driver: '%s'", device, driver);
        args->device = device ? strdup(device) : NULL;
        args->driver = driver ? strdup(driver) : NULL;
    }

    mrl = mrl_new(mp->players[mp->tv_type], type, args);
    return mrl;
}

static mrl_t *
set_cdda_stream(const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_cd_args_t *args;
    char device[16];
    int track;
    args = calloc(1, sizeof(mrl_resource_cd_args_t));

    sscanf(uri, "cdda://%d/%s", &track, device);
    args->device = strdup(device);
    args->track_start = track;
    args->track_end = track;
    mrl = mrl_new(mp->player, type, args);
    return mrl;
}

static mrl_t *
set_spotify_stream(const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_local_args_t *args;
    
    args = calloc(1, sizeof(mrl_resource_cd_args_t));
    args->location = strdup(uri);
   
    mrl = mrl_new(mp->players[mp->spotify_type], type, args);
    return mrl;
}



static mrl_t *
set_local_stream(const char *uri)
{
    mrl_t *mrl;
    mrl_resource_local_args_t *args;

    printf("set local stream : %s\n", uri);
    args = calloc(1, sizeof(mrl_resource_local_args_t));
    args->location = strdup(uri);
    mrl = mrl_new(mp->players[mp->default_type], MRL_RESOURCE_FILE, args);

    return mrl;
}

static void
init_sub_align(void)
{
    if (!mp_cfg.sub_align || !strcmp(mp_cfg.sub_align, "auto"))
        mp->subtitle_alignment = SUB_ALIGNMENT_DEFAULT;
    else if (!strcmp(mp_cfg.sub_align, "bottom"))
        mp->subtitle_alignment = PLAYER_SUB_ALIGNMENT_BOTTOM;
    else if (!strcmp(mp_cfg.sub_align, "middle"))
        mp->subtitle_alignment = PLAYER_SUB_ALIGNMENT_CENTER;
    else if (!strcmp(mp_cfg.sub_align, "top"))
        mp->subtitle_alignment = PLAYER_SUB_ALIGNMENT_TOP;
}

static void
init_sub_pos(void)
{
    if (!mp_cfg.sub_pos || !strcmp(mp_cfg.sub_pos, "auto"))
        mp->subtitle_position = SUB_POSITION_DEFAULT;
    else
        mp->subtitle_position = atoi(mp_cfg.sub_pos);
}

static void
init_sub_scale(void)
{
    if (!mp_cfg.sub_scale || !strcmp(mp_cfg.sub_scale, "auto"))
        mp->subtitle_scale = SUB_SCALE_DEFAULT;
    else
        mp->subtitle_scale = atoi(mp_cfg.sub_scale);
}

static void
init_sub_visibility(void)
{
    if (!mp_cfg.sub_visibility || !strcmp(mp_cfg.sub_visibility, "auto"))
        mp->subtitle_visibility = SUB_VISIBILITY_DEFAULT;
    else if (!strcmp(mp_cfg.sub_visibility, "no"))
        mp->subtitle_visibility = 0;
    else if (!strcmp(mp_cfg.sub_visibility, "yes"))
        mp->subtitle_visibility = 1;
}

static void
init_framedrop(void)
{
    if (!mp_cfg.framedrop || !strcmp(mp_cfg.framedrop, "no"))
        mp->framedrop = FRAMEDROP_DEFAULT;
    else if (!strcmp(mp_cfg.framedrop, "soft"))
        mp->framedrop = PLAYER_FRAMEDROP_SOFT;
    else if (!strcmp(mp_cfg.framedrop, "hard"))
        mp->framedrop = PLAYER_FRAMEDROP_HARD;
}

static int
mp_file_set(const char *uri, const char *label)
{
    mrl_t *mrl = NULL;
    player_type_t player_type = mp->default_type;

    enna_log(ENNA_MSG_INFO, NULL, "Try to load : %s , %s", uri, label);

    /* try network streams */
    if (!strncmp(uri, URI_TYPE_FTP, strlen(URI_TYPE_FTP)))
        mrl = set_network_stream(uri, MRL_RESOURCE_FTP);
    else if (!strncmp(uri, URI_TYPE_HTTP, strlen(URI_TYPE_HTTP)))
        mrl = set_network_stream(uri, MRL_RESOURCE_HTTP);
    else if (!strncmp(uri, URI_TYPE_MMS, strlen(URI_TYPE_MMS)))
        mrl = set_network_stream(uri, MRL_RESOURCE_MMS);
    else if (!strncmp(uri, URI_TYPE_NETVDR, strlen(URI_TYPE_NETVDR)))
    {
        mrl = set_network_stream(uri, MRL_RESOURCE_NETVDR);
        player_type = mp->tv_type;
    }
    else if (!strncmp(uri, URI_TYPE_RTP, strlen(URI_TYPE_RTP)))
        mrl = set_network_stream(uri, MRL_RESOURCE_RTP);
    else if (!strncmp(uri, URI_TYPE_RTSP, strlen(URI_TYPE_RTSP)))
        mrl = set_network_stream(uri, MRL_RESOURCE_RTSP);
    else if (!strncmp(uri, URI_TYPE_SMB, strlen(URI_TYPE_SMB)))
        mrl = set_network_stream(uri, MRL_RESOURCE_SMB);
    else if (!strncmp(uri, URI_TYPE_TCP, strlen(URI_TYPE_TCP)))
        mrl = set_network_stream(uri, MRL_RESOURCE_TCP);
    else if (!strncmp(uri, URI_TYPE_UDP, strlen(URI_TYPE_UDP)))
        mrl = set_network_stream(uri, MRL_RESOURCE_UDP);
    else if (!strncmp(uri, URI_TYPE_UNSV, strlen(URI_TYPE_UNSV)))
        mrl = set_network_stream(uri, MRL_RESOURCE_UNSV);

    /* Try DVD video */
    else if (!strncmp(uri, URI_TYPE_DVD, strlen(URI_TYPE_DVD)))
    {
        mrl = set_dvd_stream(uri, MRL_RESOURCE_DVD);
        player_type = mp->dvd_type;
    }
    else if (!strncmp(uri, URI_TYPE_DVDNAV, strlen(URI_TYPE_DVDNAV)))
    {
        mrl = set_dvd_stream(uri, MRL_RESOURCE_DVDNAV);
        player_type = mp->dvd_type;
    }

    /* Try TV */
    else if (!strncmp(uri, URI_TYPE_VDR, strlen(URI_TYPE_VDR)))
    {
        char *device = NULL;
        char *driver = strstr(uri, "#");
        size_t device_len = strlen(uri) - strlen(URI_TYPE_VDR);

        if (driver)
        {
            device_len -= strlen(driver);
            driver++;
            device = malloc(device_len);
            strncpy(device, uri + strlen(URI_TYPE_VDR), device_len);
        }
        else if (device_len)
            device = strdup(uri + strlen(URI_TYPE_VDR));

        mrl = set_tv_stream(device, driver, MRL_RESOURCE_VDR);
        player_type = mp->tv_type;
    }
    /* Try CD Audio */
    else if (!strncmp(uri, URI_TYPE_CDDA, strlen(URI_TYPE_CDDA)))
    {
        mrl = set_cdda_stream(uri, MRL_RESOURCE_CDDA);
    }
    /* Try Spotify stream */
    else if (!strncmp(uri, URI_TYPE_SPOTIFY, strlen(URI_TYPE_SPOTIFY)))
      {
	mrl = set_spotify_stream(uri, MRL_RESOURCE_FILE);
	player_type = mp->spotify_type;
      }
    /* default is local files */
    if (!mrl)
    {
	printf("MRL is NULL try local file\n");
        const char *it;
        it = strrchr(uri, '.');
        if (it && !strcmp(it, ".iso")) /* consider ISO file as DVD */
        {
            mrl = set_dvd_stream(uri, MRL_RESOURCE_DVDNAV);
            player_type = mp->dvd_type;
        }
        else
	  {
            mrl = set_local_stream(uri);
	  }
    }

    if (!mrl)
      {
	printf("MRL is NULL\n");
        return 1;
      }
    ENNA_FREE(mp->uri);
    mp->uri = strdup(uri);

    ENNA_FREE(mp->label);
    mp->label = label ? strdup(label) : NULL;

    mp->audio_delay = AUDIO_DELAY_DEFAULT;
    mp->subtitle_delay = SUB_DELAY_DEFAULT;

    /* Initialization of subtitles variables */
    init_sub_align();
    init_sub_pos();
    init_sub_scale();
    init_framedrop();
    init_sub_visibility();

    mp->player_type = player_type;
    mp->player = mp->players[player_type];

    player_mrl_set(mp->player, mrl);

    if (mp->subtitle_alignment != SUB_ALIGNMENT_DEFAULT)
        player_subtitle_set_alignment(mp->player, mp->subtitle_alignment);
    if (mp->subtitle_position != SUB_POSITION_DEFAULT)
        player_subtitle_set_position(mp->player, mp->subtitle_position);
    if (mp->subtitle_scale != SUB_SCALE_DEFAULT)
        player_subtitle_scale(mp->player, mp->subtitle_scale, 1);
    player_subtitle_set_visibility(mp->player, mp->subtitle_visibility);
    if (mp->framedrop != FRAMEDROP_DEFAULT)
        player_set_framedrop(mp->player, mp->framedrop);

    return 0;
}

static int
mp_play(void)
{
    player_pb_state_t state = player_playback_get_state(mp->player);

    if (state == PLAYER_PB_STATE_PAUSE)
        player_playback_pause(mp->player); /* unpause */
    else if (state == PLAYER_PB_STATE_IDLE)
        player_playback_start(mp->player);

    return 0;
}

static int
mp_stop(void)
{
    player_playback_stop(mp->player);
    return 0;
}

static int
mp_pause(void)
{
    enna_log(ENNA_MSG_INFO, NULL, "pause");
    if (player_playback_get_state(mp->player) == PLAYER_PB_STATE_PLAY)
        player_playback_pause(mp->player);

    return 0;
}

static double
mp_position_get(void)
{
    double time_pos = 0.0;

    time_pos = (double) player_get_time_pos(mp->player) / 1000.0;
    return time_pos < 0.0 ? 0.0 : time_pos;
}

static int
mp_position_percent_get(void)
{
    int percent_pos = 0;

    percent_pos = player_get_percent_pos(mp->player);
    return percent_pos < 0 ? 0 : percent_pos;
}

static double
mp_length_get(void)
{
    return (double) mrl_get_property(mp->player,
                                     NULL, MRL_PROPERTY_LENGTH) / 1000.0;
}

static void
mp_video_resize(int x, int y, int w, int h)
{
    const int flags = PLAYER_X_WINDOW_X | PLAYER_X_WINDOW_Y |
                      PLAYER_X_WINDOW_W | PLAYER_X_WINDOW_H;

    /* if w or h is 0, libplayer guess the best size automatically */
    player_x_window_set_properties(mp->player, x, y, w, h, flags);
}

static void
mp_event_cb_set(void (*event_cb)(void *data,
                                 enna_mediaplayer_event_t event),
                void *data)
{
    /* FIXME: function to call when end of stream is send by libplayer */
    mp->event_cb_data = data;
    mp->event_cb = event_cb;
}

static void
mp_send_key(enna_input event)
{
    const player_vdr_t vdr_keymap[] = {
        [ENNA_INPUT_MENU]       = PLAYER_VDR_MENU,
        [ENNA_INPUT_LEFT]       = PLAYER_VDR_LEFT,
        [ENNA_INPUT_RIGHT]      = PLAYER_VDR_RIGHT,
        [ENNA_INPUT_UP]         = PLAYER_VDR_UP,
        [ENNA_INPUT_DOWN]       = PLAYER_VDR_DOWN,
        [ENNA_INPUT_CHANPLUS]   = PLAYER_VDR_CHANNELPLUS,
        [ENNA_INPUT_CHANMINUS]  = PLAYER_VDR_CHANNELMINUS,
        [ENNA_INPUT_PREV]       = PLAYER_VDR_PREVIOUS,
        [ENNA_INPUT_NEXT]       = PLAYER_VDR_NEXT,
        [ENNA_INPUT_OK]         = PLAYER_VDR_OK,
        [ENNA_INPUT_BACK]       = PLAYER_VDR_BACK,
        [ENNA_INPUT_INFO]       = PLAYER_VDR_INFO,
        [ENNA_INPUT_KEY_0]      = PLAYER_VDR_0,
        [ENNA_INPUT_KEY_1]      = PLAYER_VDR_1,
        [ENNA_INPUT_KEY_2]      = PLAYER_VDR_2,
        [ENNA_INPUT_KEY_3]      = PLAYER_VDR_3,
        [ENNA_INPUT_KEY_4]      = PLAYER_VDR_4,
        [ENNA_INPUT_KEY_5]      = PLAYER_VDR_5,
        [ENNA_INPUT_KEY_6]      = PLAYER_VDR_6,
        [ENNA_INPUT_KEY_7]      = PLAYER_VDR_7,
        [ENNA_INPUT_KEY_8]      = PLAYER_VDR_8,
        [ENNA_INPUT_KEY_9]      = PLAYER_VDR_9,
        [ENNA_INPUT_RED]        = PLAYER_VDR_RED,
        [ENNA_INPUT_GREEN]      = PLAYER_VDR_GREEN,
        [ENNA_INPUT_YELLOW]     = PLAYER_VDR_YELLOW,
        [ENNA_INPUT_BLUE]       = PLAYER_VDR_BLUE,
        [ENNA_INPUT_PLAY]       = PLAYER_VDR_PLAY,
        [ENNA_INPUT_PAUSE]      = PLAYER_VDR_PAUSE,
        [ENNA_INPUT_STOP]       = PLAYER_VDR_STOP,
        [ENNA_INPUT_RECORD]     = PLAYER_VDR_RECORD,
        [ENNA_INPUT_CHANPREV]   = PLAYER_VDR_CHANNELPREVIOUS,
        [ENNA_INPUT_FORWARD]    = PLAYER_VDR_FASTREW,
        [ENNA_INPUT_REWIND]     = PLAYER_VDR_FASTFWD,
        [ENNA_INPUT_AUDIO_PREV] = PLAYER_VDR_AUDIO,
        [ENNA_INPUT_AUDIO_NEXT] = PLAYER_VDR_AUDIO,
        [ENNA_INPUT_SUBTITLES]  = PLAYER_VDR_SUBTITLES,
        [ENNA_INPUT_SUBS_PREV]  = PLAYER_VDR_SUBTITLES,
        [ENNA_INPUT_SUBS_NEXT]  = PLAYER_VDR_SUBTITLES,
        [ENNA_INPUT_VOLPLUS]    = PLAYER_VDR_VOLPLUS,
        [ENNA_INPUT_VOLMINUS]   = PLAYER_VDR_VOLMINUS,
        [ENNA_INPUT_MUTE]       = PLAYER_VDR_MUTE,
        [ENNA_INPUT_SCHEDULE]   = PLAYER_VDR_SCHEDULE,
        [ENNA_INPUT_CHANNELS]   = PLAYER_VDR_CHANNELS,
        [ENNA_INPUT_TIMERS]     = PLAYER_VDR_TIMERS,
        [ENNA_INPUT_RECORDINGS] = PLAYER_VDR_RECORDINGS,
    };

    if (mp->uri && strncmp(mp->uri, URI_TYPE_VDR, strlen(URI_TYPE_VDR)) &&
        strncmp(mp->uri, URI_TYPE_NETVDR, strlen(URI_TYPE_NETVDR)))
        return;

    if (event >= ARRAY_NB_ELEMENTS(vdr_keymap) || event < 0)
        return;

    switch (event)
    {
    case ENNA_INPUT_UNKNOWN:
        enna_log(ENNA_MSG_INFO, NULL, "Unknown event received: %d", event);
        break;
    default:
        player_vdr(mp->player, vdr_keymap[event]);
    }
}

/* externally accessible functions */
int
enna_mediaplayer_supported_uri_type(enna_mediaplayer_uri_type_t type)
{
    struct {
        const mrl_resource_t res;
        player_type_t p_type;
    } type_list[] = {
        [ENNA_MP_URI_TYPE_CDDA]   = { MRL_RESOURCE_CDDA,    mp->default_type },
        [ENNA_MP_URI_TYPE_DVD]    = { MRL_RESOURCE_DVD,     mp->dvd_type     },
        [ENNA_MP_URI_TYPE_DVDNAV] = { MRL_RESOURCE_DVDNAV,  mp->dvd_type     },
        [ENNA_MP_URI_TYPE_FTP]    = { MRL_RESOURCE_FTP,     mp->default_type },
        [ENNA_MP_URI_TYPE_HTTP]   = { MRL_RESOURCE_HTTP,    mp->default_type },
        [ENNA_MP_URI_TYPE_MMS]    = { MRL_RESOURCE_MMS,     mp->default_type },
        [ENNA_MP_URI_TYPE_NETVDR] = { MRL_RESOURCE_NETVDR,  mp->tv_type      },
        [ENNA_MP_URI_TYPE_RTP]    = { MRL_RESOURCE_RTP,     mp->default_type },
        [ENNA_MP_URI_TYPE_RTSP]   = { MRL_RESOURCE_RTSP,    mp->default_type },
        [ENNA_MP_URI_TYPE_SMB]    = { MRL_RESOURCE_SMB,     mp->default_type },
        [ENNA_MP_URI_TYPE_TCP]    = { MRL_RESOURCE_TCP,     mp->default_type },
        [ENNA_MP_URI_TYPE_UDP]    = { MRL_RESOURCE_UDP,     mp->default_type },
        [ENNA_MP_URI_TYPE_UNSV]   = { MRL_RESOURCE_UNSV,    mp->default_type },
        [ENNA_MP_URI_TYPE_VDR]    = { MRL_RESOURCE_VDR,     mp->tv_type      },
	[ENNA_MP_URI_TYPE_SPOTIFY]= { MRL_RESOURCE_FILE,    mp->spotify_type },
    };

    if (type >= ARRAY_NB_ELEMENTS(type_list) || type < 0)
        return 0;

    return libplayer_wrapper_supported_res(type_list[type].p_type,
                                           type_list[type].res);
}

#define CFG_VIDEO(field)                                                \
    value = enna_config_string_get(section, #field);                    \
    mp_cfg.field = value ? strdup(value) : NULL;

static const struct {
    const char *name;
    player_type_t type;
} map_player_type[] = {
    { "xine",       PLAYER_TYPE_XINE         },
    { "mplayer",    PLAYER_TYPE_MPLAYER      },
    { "vlc",        PLAYER_TYPE_VLC          },
    { "gstreamer",  PLAYER_TYPE_GSTREAMER    },
    { "spotify",    PLAYER_TYPE_SPOTIFY      },
    { NULL,         PLAYER_TYPE_DUMMY        }
};

static const struct {
    const char *name;
    player_vo_t vo;
} map_player_vo[] = {
    { "auto",       PLAYER_VO_AUTO           },
    { "vdpau",      PLAYER_VO_VDPAU          },
    { "x11",        PLAYER_VO_X11            },
    { "xv",         PLAYER_VO_XV             },
    { "gl",         PLAYER_VO_GL             },
    { "fb",         PLAYER_VO_FB             },
    { NULL,         PLAYER_VO_NULL           }
};

static const struct {
    const char *name;
    player_ao_t ao;
} map_player_ao[] = {
    { "auto",       PLAYER_AO_AUTO           },
    { "alsa",       PLAYER_AO_ALSA           },
    { "oss",        PLAYER_AO_OSS            },
    { "pulse",      PLAYER_AO_PULSE          },
    { NULL,         PLAYER_AO_NULL           }
};

static const struct {
    const char *name;
    player_verbosity_level_t verb;
} map_player_verbosity[] = {
    { "none",        PLAYER_MSG_NONE         },
    { "verbose",     PLAYER_MSG_VERBOSE      },
    { "info",        PLAYER_MSG_INFO         },
    { "warning",     PLAYER_MSG_WARNING      },
    { "error",       PLAYER_MSG_ERROR        },
    { "critical",    PLAYER_MSG_CRITICAL     },
    { NULL,          PLAYER_MSG_NONE         }
};

static void
cfg_mediaplayer_section_load (const char *section)
{
    const char *value = NULL;
    int i;

    enna_log(ENNA_MSG_INFO, NULL, "parameters:");

    value = enna_config_string_get(section, "type");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * type: %s", value);

        for (i = 0; map_player_type[i].name; i++)
            if (!strcmp(value, map_player_type[i].name))
            {
                mp_cfg.type = map_player_type[i].type;
                break;
            }
    }

    value = enna_config_string_get(section, "dvd_type");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * dvd_type: %s", value);

        for (i = 0; map_player_type[i].name; i++)
            if (!strcmp(value, map_player_type[i].name))
            {
                mp_cfg.dvd_type = map_player_type[i].type;
                break;
            }
        mp_cfg.dvd_set = 1;
    }

    value = enna_config_string_get(section, "tv_type");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * tv_type: %s", value);

        for (i = 0; map_player_type[i].name; i++)
            if (!strcmp(value, map_player_type[i].name))
            {
                mp_cfg.tv_type = map_player_type[i].type;
                break;
            }
        mp_cfg.tv_set = 1;
    }

    value = enna_config_string_get(section, "spotify_type");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * spotify_type: %s", value);

        for (i = 0; map_player_type[i].name; i++)
            if (!strcmp(value, map_player_type[i].name))
            {
                mp_cfg.spotify_type = map_player_type[i].type;
                break;
            }
        mp_cfg.spotify_set = 1;
    }

    value = enna_config_string_get(section, "video_out");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * video out: %s", value);

        for (i = 0; map_player_vo[i].name; i++)
            if (!strcmp(value, map_player_vo[i].name))
            {
                mp_cfg.vo = map_player_vo[i].vo;
                break;
            }
    }

    value = enna_config_string_get(section, "audio_out");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * audio out: %s", value);

        for (i = 0; map_player_ao[i].name; i++)
            if (!strcmp(value, map_player_ao[i].name))
            {
                mp_cfg.ao = map_player_ao[i].ao;
                break;
            }
    }

    value = enna_config_string_get(section, "verbosity");
    if (value)
    {
        enna_log(ENNA_MSG_INFO, NULL, " * verbosity level: %s", value);

        for (i = 0; map_player_verbosity[i].name; i++)
            if (!strcmp(value, map_player_verbosity[i].name))
            {
                mp_cfg.verbosity = map_player_verbosity[i].verb;
                break;
            }
    }

    CFG_VIDEO(sub_align);
    CFG_VIDEO(sub_pos);
    CFG_VIDEO(sub_scale);
    CFG_VIDEO(sub_visibility);
    CFG_VIDEO(framedrop);

    if (!value)
        enna_log(ENNA_MSG_INFO, NULL, " * use all parameters by default");
}

static void
cfg_mediaplayer_section_save (const char *section)
{
    int i;

    /* Default type */
    for (i = 0; map_player_type[i].name; i++)
        if (mp_cfg.type == map_player_type[i].type)
        {
            enna_config_string_set(section, "type",
                                   map_player_type[i].name);
            break;
        }

    /* DVD Type */
    for (i = 0; map_player_type[i].name; i++)
        if (mp_cfg.dvd_type == map_player_type[i].type)
        {
            enna_config_string_set(section, "dvd_type",
                                   map_player_type[i].name);
            break;
        }

    /* TV Type */
    for (i = 0; map_player_type[i].name; i++)
        if (mp_cfg.tv_type == map_player_type[i].type)
        {
            enna_config_string_set(section, "tv_type",
                                   map_player_type[i].name);
            break;
        }

    /* Spotify Type */
    for (i = 0; map_player_type[i].name; i++)
        if (mp_cfg.spotify_type == map_player_type[i].type)
        {
            enna_config_string_set(section, "spotify_type",
                                   map_player_type[i].name);
            break;
        }

    /* VO Type */
    for (i = 0; map_player_vo[i].name; i++)
        if (mp_cfg.vo == map_player_vo[i].vo)
        {
            enna_config_string_set(section, "video_out",
                                   map_player_vo[i].name);
            break;
        }

    /* AO Type */
    for (i = 0; map_player_ao[i].name; i++)
        if (mp_cfg.ao == map_player_ao[i].ao)
        {
            enna_config_string_set(section, "audio_out",
                                   map_player_ao[i].name);
            break;
        }

    /* Verbosity */
    for (i = 0; map_player_verbosity[i].name; i++)
        if (mp_cfg.verbosity == map_player_verbosity[i].verb)
        {
            enna_config_string_set(section, "verbosity",
                                   map_player_verbosity[i].name);
            break;
        }

    enna_config_string_set(section, "sub_align",      mp_cfg.sub_align);
    enna_config_string_set(section, "sub_pos",        mp_cfg.sub_pos);
    enna_config_string_set(section, "sub_scale",      mp_cfg.sub_scale);
    enna_config_string_set(section, "sub_visibility", mp_cfg.sub_visibility);
    enna_config_string_set(section, "framedrop",      mp_cfg.framedrop);
}

static void
cfg_mediaplayer_free (void)
{
    ENNA_FREE(mp_cfg.sub_align);
    ENNA_FREE(mp_cfg.sub_pos);
    ENNA_FREE(mp_cfg.sub_scale);
    ENNA_FREE(mp_cfg.sub_visibility);
    ENNA_FREE(mp_cfg.framedrop);
}

static void
cfg_mediaplayer_section_set_default (void)
{
    cfg_mediaplayer_free();

    mp_cfg.type           = PLAYER_TYPE_MPLAYER;
    mp_cfg.dvd_type       = PLAYER_TYPE_XINE;
    mp_cfg.tv_type        = PLAYER_TYPE_XINE;
    mp_cfg.spotify_type   = PLAYER_TYPE_SPOTIFY;
    mp_cfg.vo             = PLAYER_VO_AUTO;
    mp_cfg.ao             = PLAYER_AO_AUTO;
    mp_cfg.verbosity      = PLAYER_MSG_WARNING;
    mp_cfg.dvd_set        = 0;
    mp_cfg.tv_set         = 0;
    mp_cfg.spotify_set    = 0;
    mp_cfg.sub_align      = NULL;
    mp_cfg.sub_pos        = NULL;
    mp_cfg.sub_scale      = NULL;
    mp_cfg.sub_visibility = NULL;
    mp_cfg.framedrop      = NULL;
}

static Enna_Config_Section_Parser cfg_mediaplayer = {
    "mediaplayer",
    cfg_mediaplayer_section_load,
    cfg_mediaplayer_section_save,
    cfg_mediaplayer_section_set_default,
    cfg_mediaplayer_free,
};

void
enna_mediaplayer_cfg_register (void)
{
    enna_config_section_parser_register(&cfg_mediaplayer);
}

int
enna_mediaplayer_init(void)
{
    player_init_param_t param;

    memset(&param, 0, sizeof(player_init_param_t));
    param.ao       = mp_cfg.ao;
    param.vo       = mp_cfg.vo;
    param.winid    = (unsigned long) enna->ee_winid;
    param.event_cb = event_cb;

    /* Main player type is mandatory! */
    if (!libplayer_wrapper_enabled(mp_cfg.type))
      {
	printf("%d not supported\n", mp_cfg.type);
        goto err;
      }

    /*
     * When dvd_type or tv_type are set in the enna.cfg config file, then
     * these values are used. But if the wrapper is not enabled in libplayer,
     * an error is returned.
     * Otherwise, if nothing is changed by the user but the wrapper is not
     * enabled, then the main type is used instead.
     */
    if (mp_cfg.dvd_type != mp_cfg.type &&
        !libplayer_wrapper_enabled(mp_cfg.dvd_type))
    {
        if (mp_cfg.dvd_set)
            goto err;
        mp_cfg.dvd_type = mp_cfg.type;
    }

    if (mp_cfg.tv_type != mp_cfg.type &&
        !libplayer_wrapper_enabled(mp_cfg.tv_type))
    {
        if (mp_cfg.tv_set)
            goto err;
        mp_cfg.tv_type = mp_cfg.type;
    }

    if (mp_cfg.spotify_type != mp_cfg.type &&
        !libplayer_wrapper_enabled(mp_cfg.spotify_type))
    {
        if (mp_cfg.spotify_set)
	  {
	    printf("spotify_set = 1\n");
            goto err;
	  }
        mp_cfg.spotify_type = mp_cfg.type;
    }

    mp = calloc(1, sizeof(Enna_Mediaplayer));

    mp->key_down_event_handler =
        ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, event_key_down, NULL);
    mp->mouse_button_event_handler =
        ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                event_mouse_button, NULL);
    mp->mouse_move_event_handler =
        ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                event_mouse_move, NULL);

    mp->pipe = ecore_pipe_add(pipe_read, NULL);

    mp->players[mp_cfg.type] =
        player_init(mp_cfg.type, mp_cfg.verbosity, &param);
    if (!mp->players[mp_cfg.type])
        goto err;
    
    if (mp_cfg.dvd_type != mp_cfg.type)
    {
        mp->players[mp_cfg.dvd_type] =
            player_init(mp_cfg.dvd_type, mp_cfg.verbosity, &param);
        if (!mp->players[mp_cfg.dvd_type])
        {
            enna_log(ENNA_MSG_WARNING, NULL,
                     "DVD mediaplayer initialization failed, "
                     "falling back to the default mediaplayer");
            mp_cfg.dvd_type = mp_cfg.type;
        }
    }
    
    if (mp_cfg.tv_type != mp_cfg.type && mp_cfg.tv_type != mp_cfg.dvd_type)
    {
        mp->players[mp_cfg.tv_type] =
            player_init(mp_cfg.tv_type, mp_cfg.verbosity, &param);
        if (!mp->players[mp_cfg.tv_type])
        {
            enna_log(ENNA_MSG_WARNING, NULL,
                     "TV mediaplayer initialization failed, "
                     "falling back to the default mediaplayer");
            mp_cfg.tv_type = mp_cfg.type;
        }
    }

    if (mp_cfg.spotify_type != mp_cfg.type && mp_cfg.spotify_type != mp_cfg.dvd_type)
    {
        mp->players[mp_cfg.spotify_type] =
            player_init(mp_cfg.spotify_type, mp_cfg.verbosity, &param);
        if (!mp->players[mp_cfg.spotify_type])
        {
            enna_log(ENNA_MSG_WARNING, NULL,
                     "Spotify mediaplayer initialization failed, "
                     "falling back to the default mediaplayer");
            mp_cfg.spotify_type = mp_cfg.type;
        }
    }

    mp_event_cb_set(_event_cb, NULL);

    mp->uri = NULL;
    mp->label = NULL;
    mp->default_type = mp_cfg.type;
    mp->dvd_type = mp_cfg.dvd_type;
    mp->tv_type = mp_cfg.tv_type;
    mp->spotify_type = mp_cfg.spotify_type;
    mp->player = mp->players[mp_cfg.type];
    mp->player_type = mp_cfg.type;
    mp->play_state = STOPPED;

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

    return 1;

 err:
    enna_log(ENNA_MSG_ERROR, NULL, "Mediaplayer initialization");
    enna_mediaplayer_shutdown();
    return 0;
}

void
enna_mediaplayer_shutdown(void)
{
    int i;

    if (!mp)
        return;

    ENNA_FREE(mp->uri);
    ENNA_FREE(mp->label);
    ENNA_EVENT_HANDLER_DEL(mp->key_down_event_handler);
    ENNA_EVENT_HANDLER_DEL(mp->mouse_button_event_handler);
    ENNA_EVENT_HANDLER_DEL(mp->mouse_move_event_handler);
    if (mp->pipe)
        ecore_pipe_del(mp->pipe);
    player_playback_stop(mp->player);
    for (i = 0; i < MAX_PLAYERS; i++)
        if (mp->players[i])
            player_uninit(mp->players[i]);
    ENNA_FREE(mp);
}

Enna_File *
enna_mediaplayer_current_file_get()
{ 
    Enna_File *item;

    if (!mp->cur_playlist || mp->play_state != PLAYING)
        return NULL;
    
    item = eina_list_nth(mp->cur_playlist->playlist, mp->cur_playlist->selected);
    if (!item)
        return NULL;

    return item;
}

char *
enna_mediaplayer_get_current_uri()
{
  Enna_File *item;

  if (!mp->cur_playlist || mp->play_state != PLAYING)
    return NULL;

  item = eina_list_nth(mp->cur_playlist->playlist, mp->cur_playlist->selected);
  if (!item || !item->mrl)
    return NULL;
  return eina_stringshare_add(item->mrl);
}

void
enna_mediaplayer_file_append(Enna_Playlist *enna_playlist, Enna_File *file)
{
    Enna_File *f;
    f = enna_file_ref(file);
    enna_playlist->playlist = eina_list_append(enna_playlist->playlist, f);
}

int
enna_mediaplayer_play(Enna_Playlist *enna_playlist)
{
    mp->cur_playlist = enna_playlist;

    switch (mp->play_state)
    {
    case STOPPED:
    {
        Enna_File *item;
        item = eina_list_nth(enna_playlist->playlist,
                             enna_playlist->selected);
        mp_stop();
        if (item && item->mrl)
            mp_file_set(item->mrl, item->label);
        mp_play();
        mp->play_state = PLAYING;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_START, NULL, NULL, NULL);
        break;
    }
    case PLAYING:
        enna_mediaplayer_pause();
        break;
    case PAUSE:
        mp_play();
        mp->play_state = PLAYING;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_UNPAUSE,
                        NULL, NULL, NULL);
        break;
    default:
        break;
    }

    return 0;
}

int
enna_mediaplayer_select_nth(Enna_Playlist *enna_playlist,int n)
{
    if (n < 0 || n > eina_list_count(enna_playlist->playlist) - 1)
        return -1;

    enna_log(ENNA_MSG_EVENT, NULL, "select %d", n);
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
    mp_stop();
    mp->play_state = STOPPED;
    ecore_event_add(ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);

    return 0;
}

int
enna_mediaplayer_pause(void)
{
    mp_pause();
    mp->play_state = PAUSE;
    ecore_event_add(ENNA_EVENT_MEDIAPLAYER_PAUSE, NULL, NULL, NULL);

    return 0;
}

static void
enna_mediaplayer_change(Enna_Playlist *enna_playlist, int type)
{
    Enna_File *item;

    item = eina_list_nth(enna_playlist->playlist, enna_playlist->selected);
    enna_log(ENNA_MSG_EVENT, NULL, "select %d", enna_playlist->selected);
    if (!item)
        return;

    enna_mediaplayer_stop();
    enna_mediaplayer_play(enna_playlist);
    ecore_event_add(type, NULL, NULL, NULL);
}

int
enna_mediaplayer_next(Enna_Playlist *enna_playlist)
{
    enna_playlist->selected++;
    if(enna_playlist->selected >
       eina_list_count(enna_playlist->playlist) - 1)
    {
        enna_playlist->selected--;
        return -1;
    }

    enna_mediaplayer_change(enna_playlist, ENNA_EVENT_MEDIAPLAYER_NEXT);
    return 0;
}

int
enna_mediaplayer_prev(Enna_Playlist *enna_playlist)
{
    enna_playlist->selected--;
    if (enna_playlist->selected < 0)
    {
        enna_playlist->selected = 0;
        return -1;
    }

    enna_mediaplayer_change(enna_playlist, ENNA_EVENT_MEDIAPLAYER_PREV);
    return 0;
}

double
enna_mediaplayer_position_get(void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
        mp_position_get(): 0.0;
}

int
enna_mediaplayer_position_percent_get(void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
            mp_position_percent_get() : 0;
}

double
enna_mediaplayer_length_get(void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
        mp_length_get() : 0.0;
}

static void
enna_mediaplayer_seek(int value, SEEK_TYPE type)
{
    const player_pb_seek_t pl_seek[] = {
        [SEEK_ABS_PERCENT] = PLAYER_PB_SEEK_PERCENT,
        [SEEK_ABS_SECONDS] = PLAYER_PB_SEEK_ABSOLUTE,
        [SEEK_REL_SECONDS] = PLAYER_PB_SEEK_RELATIVE
    };

    enna_log(ENNA_MSG_EVENT, NULL, "Seeking to: %d%c",
             value, type == SEEK_ABS_PERCENT ? '%' : 's');

    if (type >= ARRAY_NB_ELEMENTS(pl_seek))
        return;

    if (mp->play_state == PAUSE || mp->play_state == PLAYING)
    {
        Enna_Event_Mediaplayer_Seek_Data *ev;

        ev = calloc(1, sizeof(Enna_Event_Mediaplayer_Seek_Data));
        if (!ev)
            return;

        ev->seek_value = value;
        ev->type       = type;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_SEEK, ev, NULL, NULL);
        if (type != SEEK_ABS_PERCENT)
          value *= 1000;
        player_playback_seek(mp->player, value, pl_seek[type]);
    }
}

void
enna_mediaplayer_position_set(int seconds)
{
    enna_mediaplayer_seek(seconds, SEEK_ABS_SECONDS);
}

void
enna_mediaplayer_seek_percent(int percent)
{
    enna_mediaplayer_seek(percent, SEEK_ABS_PERCENT);
}

void
enna_mediaplayer_seek_relative(int seconds)
{
    enna_mediaplayer_seek(seconds, SEEK_REL_SECONDS);
}

void
enna_mediaplayer_default_seek_backward(void)
{
    enna_mediaplayer_seek_relative(-SEEK_STEP_DEFAULT);
}

void
enna_mediaplayer_default_seek_forward(void)
{
    enna_mediaplayer_seek_relative(SEEK_STEP_DEFAULT);
}

void
enna_mediaplayer_video_resize(int x, int y, int w, int h)
{
    mp_video_resize(x, y, w, h);
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
    Enna_File *f;

    EINA_LIST_FREE(enna_playlist->playlist, f)
        enna_file_free(f);
    enna_playlist->playlist = NULL;
    enna_playlist->selected = 0;
}

Enna_Metadata *
enna_mediaplayer_metadata_get(Enna_Playlist *enna_playlist)
{
    Enna_File *item;

    item = eina_list_nth(enna_playlist->playlist, enna_playlist->selected);
    if (!item)
        return NULL;

    if (item->mrl)
        return enna_metadata_meta_new((char *) item->mrl);

    return NULL;
}

int
enna_mediaplayer_playlist_count(Enna_Playlist *enna_playlist)
{
    return eina_list_count(enna_playlist->playlist);
}

PLAY_STATE
enna_mediaplayer_state_get(void)
{
    return mp->play_state;
}

Enna_Playlist *
enna_mediaplayer_playlist_create(void)
{
    Enna_Playlist *enna_playlist;

    enna_playlist = calloc(1, sizeof(Enna_Playlist));
    enna_playlist->selected = 0;
    enna_playlist->playlist = NULL;
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
    mp_stop();
    mp->play_state = STOPPED;
    ecore_event_add(ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);
}

void
enna_mediaplayer_send_input(enna_input event)
{
    mp_send_key(event);
}

int
enna_mediaplayer_volume_get(void)
{
    return (mp && mp->player) ? player_audio_volume_get(mp->player) : 0;
}

void
enna_mediaplayer_volume_set(int value)
{
    player_audio_volume_set(mp->player, value);
}

void
enna_mediaplayer_default_increase_volume(void)
{
    int vol = enna_mediaplayer_volume_get();
    vol = MMIN(vol + VOLUME_STEP_DEFAULT, 100);
    player_audio_volume_set(mp->player, vol);
}

void
enna_mediaplayer_default_decrease_volume(void)
{
    int vol = enna_mediaplayer_volume_get();
    vol = MMAX(vol - VOLUME_STEP_DEFAULT, 0);
    player_audio_volume_set(mp->player, vol);
}

void
enna_mediaplayer_mute(void)
{
    player_mute_t m;

    m = player_audio_mute_get(mp->player);
    player_audio_mute_set(mp->player, (m == PLAYER_MUTE_ON) ?
                          PLAYER_MUTE_OFF : PLAYER_MUTE_ON);
}

int
enna_mediaplayer_mute_get(void)
{
    player_mute_t m;

    if (!mp)
      return 0;

    m = player_audio_mute_get(mp->player);
    if (m == PLAYER_MUTE_ON)
        return 1;
    else
        return 0;
}

void
enna_mediaplayer_audio_previous(void)
{
    player_audio_prev(mp->player);
}

void
enna_mediaplayer_audio_next(void)
{
    player_audio_next(mp->player);
}

void
enna_mediaplayer_audio_increase_delay(void)
{
    mp->audio_delay += 100;
    player_audio_set_delay(mp->player, mp->audio_delay, 0);
}

void
enna_mediaplayer_audio_decrease_delay(void)
{
    mp->audio_delay -= 100;
    player_audio_set_delay(mp->player, mp->audio_delay, 0);
}

void
enna_mediaplayer_subtitle_set_visibility(void)
{
    mp->subtitle_visibility = !mp->subtitle_visibility;
    player_subtitle_set_visibility(mp->player, mp->subtitle_visibility);
}

void
enna_mediaplayer_subtitle_previous(void)
{
    player_subtitle_prev(mp->player);
}

void
enna_mediaplayer_subtitle_next(void)
{
    player_subtitle_next(mp->player);
}

void
enna_mediaplayer_subtitle_set_alignment(void)
{
    mp->subtitle_alignment = (mp->subtitle_alignment + 1) % 3;
    player_subtitle_set_alignment(mp->player, mp->subtitle_alignment);
}

void
enna_mediaplayer_subtitle_increase_position(void)
{
    mp->subtitle_position += 1;
    player_subtitle_set_position(mp->player, mp->subtitle_position);
}

void
enna_mediaplayer_subtitle_decrease_position(void)
{
    mp->subtitle_position -= 1;
    player_subtitle_set_position(mp->player, mp->subtitle_position);
}

void
enna_mediaplayer_subtitle_increase_scale(void)
{
    mp->subtitle_scale += 1;
    player_subtitle_scale(mp->player, mp->subtitle_scale, 1);
}

void
enna_mediaplayer_subtitle_decrease_scale(void)
{
    mp->subtitle_scale -= 1;
    player_subtitle_scale(mp->player, mp->subtitle_scale, 1);
}

void
enna_mediaplayer_subtitle_increase_delay(void)
{
    mp->subtitle_delay += 100;
    player_subtitle_set_delay(mp->player, mp->subtitle_delay);
}

void
enna_mediaplayer_subtitle_decrease_delay(void)
{
    mp->subtitle_delay -= 100;
    player_subtitle_set_delay(mp->player, mp->subtitle_delay);
}

void
enna_mediaplayer_set_framedrop(void)
{
    mp->framedrop = (mp->framedrop + 1) % 3;
    player_set_framedrop(mp->player, mp->framedrop);
}
