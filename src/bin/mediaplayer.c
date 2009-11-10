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

#include <sys/types.h>
#include <string.h>

#include <Ecore.h>
#include <Ecore_Data.h>
#include <Ecore_Input.h>

#include <player.h>

#include "enna.h"
#include "enna_config.h"
#include "mediaplayer.h"
#include "module.h"
#include "logs.h"
#include "input.h"

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

#define MAX_PLAYERS 4

/* a/v controls */
#define SEEK_STEP_DEFAULT         2.0 /* percent */
#define VOLUME_STEP_DEFAULT       5 /* percent */
#define AUDIO_DELAY_DEFAULT       0
#define SUB_VISIBILITY_DEFAULT    1
#define SUB_ALIGNMENT_DEFAULT     PLAYER_SUB_ALIGNMENT_BOTTOM
#define SUB_POSITION_DEFAULT      100
#define SUB_SCALE_DEFAULT         5
#define SUB_DELAY_DEFAULT         0
#define FRAMEDROP_DEFAULT         PLAYER_FRAMEDROP_DISABLE

typedef struct list_item_s
{
    const char *uri;
    const char *label;
} list_item_t;

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
    Ecore_Event_Handler *key_down_event_handler;
    Ecore_Event_Handler *mouse_button_event_handler;
    Ecore_Event_Handler *mouse_move_event_handler;
    Ecore_Pipe *pipe;
};

static Enna_Mediaplayer *mp = NULL;
static Enna_Config_Video *config_video = NULL;

static void
_event_cb (void *data, enna_mediaplayer_event_t event)
{
    switch (event)
    {
    case ENNA_MP_EVENT_EOF:
        enna_log (ENNA_MSG_EVENT, NULL, "End of stream");
        ecore_event_add (ENNA_EVENT_MEDIAPLAYER_EOS, NULL, NULL, NULL);
        break;
    default:
        break;
    }
}

static void
pipe_read (void *data, void *buf, unsigned int nbyte)
{
    enna_mediaplayer_event_t *event = buf;

    if (!mp->event_cb || !buf)
        return;

    mp->event_cb (mp->event_cb_data, *event);
}

static int
event_cb (player_event_t e, void *data)
{
    enna_mediaplayer_event_t event;

    if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
    {
        event = ENNA_MP_EVENT_EOF;
        ecore_pipe_write (mp->pipe, &event, sizeof (event));
    }

    return 0;
}

static int
event_key_down (void *data, int type, void *event)
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
        enna_log (ENNA_MSG_EVENT, NULL,
                  "Ecore_Event_Key_Down %s", e->keyname);
        evas_event_feed_key_down (enna->evas, e->keyname, e->key,
                                  e->compose, NULL, e->timestamp, NULL);
    }

    return 1;
}

static int
event_mouse_button (void *data, int type, void *event)
{
    Ecore_Event_Mouse_Button *e = event;

    /* Broadcast mouse position only for dvd player and only
       if libplayer window is on screen */
    if ((e->window == enna->ee_winid) || !mp->uri ||
        strncmp (mp->uri, URI_TYPE_DVDNAV, strlen (URI_TYPE_DVDNAV)))
        return 1;

    /* Set mouse position and send mouseclick event */
    enna_log (ENNA_MSG_EVENT, NULL,
              "Send Mouse click %d %d, uri : %s", e->x, e->y, mp->uri);
    player_set_mouse_position (mp->players[mp->dvd_type], e->x, e->y);
    player_dvd_nav (mp->players[mp->dvd_type], PLAYER_DVDNAV_MOUSECLICK);
    return 1;
}

static int
event_mouse_move (void *data, int type, void *event)
{
    Ecore_Event_Mouse_Move *e = event;

    /* Broadcast mouse position only for dvd player and only
       if libplayer window is on screen*/
    if ((e->window == enna->ee_winid) || !mp->uri ||
        strncmp (mp->uri, URI_TYPE_DVDNAV, strlen (URI_TYPE_DVDNAV)))
        return 1;

    /* Send mouse position to libplayer */
    player_set_mouse_position (mp->players[mp->dvd_type], e->x, e->y);
    return 1;
}

static mrl_t *
set_network_stream (const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_network_args_t *args;

    enna_log (ENNA_MSG_INFO, NULL, "Load Network Stream : %s", uri);

    args = calloc (1, sizeof (mrl_resource_network_args_t));
    args->url = strdup (uri);

    if (type == MRL_RESOURCE_NETVDR)
        mrl = mrl_new (mp->players[mp->tv_type], type, args);
    else
        mrl = mrl_new (mp->players[mp->default_type], type, args);

    return mrl;
}

static mrl_t *
set_dvd_stream (const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_videodisc_args_t *args;
    char *meta;
    uint32_t prop = 0;
    int tmp = 0;
    int title = 0;
    char *device;

    enna_log (ENNA_MSG_INFO, NULL, "Load DVD Video : %s", uri);

    args = calloc (1, sizeof (mrl_resource_videodisc_args_t));
    device = strstr (uri, "://");
    if (device)
        args->device = strdup (device + 3);

    mrl = mrl_new (mp->players[mp->dvd_type], type, args);

    meta = mrl_get_metadata_dvd (mp->players[mp->dvd_type],
                                 mrl, (uint8_t *) &prop);
    if (meta)
    {
        enna_log (ENNA_MSG_INFO, NULL, "Meta DVD VolumeID: %s", meta);
        free (meta);
    }

    if (prop)
    {
        int i;

        enna_log (ENNA_MSG_INFO, NULL, "Meta DVD Titles: %i", prop);

        for (i = 1; i <= prop; i++)
        {
            uint32_t chapters, angles, length;

            chapters =
                mrl_get_metadata_dvd_title (mp->players[mp->dvd_type], mrl, i,
                                            MRL_METADATA_DVD_TITLE_CHAPTERS);
            angles =
                mrl_get_metadata_dvd_title (mp->players[mp->dvd_type], mrl, i,
                                            MRL_METADATA_DVD_TITLE_ANGLES);
            length =
                mrl_get_metadata_dvd_title (mp->players[mp->dvd_type], mrl, i,
                                            MRL_METADATA_DVD_TITLE_LENGTH);

            enna_log (ENNA_MSG_INFO, NULL,
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
set_tv_stream (const char *device, const char *driver, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_tv_args_t *args;

    args = calloc (1, sizeof (mrl_resource_tv_args_t));

    if (type == MRL_RESOURCE_VDR)
    {
        enna_log (ENNA_MSG_INFO, NULL,
                  "VDR stream; device: '%s' driver: '%s'", device, driver);
        args->device = device ? strdup (device) : NULL;
        args->driver = driver ? strdup (driver) : NULL;
    }

    mrl = mrl_new (mp->players[mp->tv_type], type, args);
    return mrl;
}

static mrl_t *
set_cdda_stream (const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_cd_args_t *args;
    char device[16];
    int track;
    args = calloc (1, sizeof (mrl_resource_cd_args_t));

    sscanf(uri, "cdda://%d/%s", &track, device);
    args->device = strdup(device);
    args->track_start = track;
    args->track_end = track;
    mrl = mrl_new (mp->player, type, args);
    return mrl;
}

static mrl_t *
set_local_stream (const char *uri)
{
    mrl_t *mrl;
    mrl_resource_local_args_t *args;

    args = calloc (1, sizeof (mrl_resource_local_args_t));
    args->location = strdup (uri);
    mrl = mrl_new (mp->players[mp->default_type], MRL_RESOURCE_FILE, args);

    return mrl;
}

static void
init_sub_align (void)
{
    if (!config_video || !config_video->sub_align ||
        !strcmp(config_video->sub_align, "auto"))
        mp->subtitle_alignment = SUB_ALIGNMENT_DEFAULT;
    else if (!strcmp(config_video->sub_align, "bottom"))
        mp->subtitle_alignment = PLAYER_SUB_ALIGNMENT_BOTTOM;
    else if (!strcmp(config_video->sub_align, "middle"))
        mp->subtitle_alignment = PLAYER_SUB_ALIGNMENT_CENTER;
    else if (!strcmp(config_video->sub_align, "top"))
        mp->subtitle_alignment = PLAYER_SUB_ALIGNMENT_TOP;
}

static void
init_sub_pos (void)
{
    if (!config_video || !config_video->sub_pos ||
        !strcmp(config_video->sub_pos, "auto"))
        mp->subtitle_position = SUB_POSITION_DEFAULT;
    else
        mp->subtitle_position = atoi (config_video->sub_pos);
}

static void
init_sub_scale (void)
{
    if (!config_video || !config_video->sub_scale ||
        !strcmp(config_video->sub_scale, "auto"))
        mp->subtitle_scale = SUB_SCALE_DEFAULT;
    else
        mp->subtitle_scale = atoi (config_video->sub_scale);
}

static void
init_sub_visibility (void)
{
    if (!config_video || !config_video->sub_visibility ||
        !strcmp(config_video->sub_visibility, "auto"))
        mp->subtitle_visibility = SUB_VISIBILITY_DEFAULT;
    else if (!strcmp(config_video->sub_visibility, "no"))
        mp->subtitle_visibility = 0;
    else if (!strcmp(config_video->sub_visibility, "yes"))
        mp->subtitle_visibility = 1;
}

static void
init_framedrop (void)
{
    if (!config_video || !config_video->framedrop ||
        !strcmp(config_video->framedrop, "no"))
        mp->framedrop = FRAMEDROP_DEFAULT;
    else if (!strcmp(config_video->framedrop, "soft"))
        mp->framedrop = PLAYER_FRAMEDROP_SOFT;
    else if (!strcmp(config_video->framedrop, "hard"))
        mp->framedrop = PLAYER_FRAMEDROP_HARD;
}

static int
mp_file_set (const char *uri, const char *label)
{
    mrl_t *mrl = NULL;
    player_type_t player_type = mp->default_type;

    enna_log (ENNA_MSG_INFO, NULL, "Try to load : %s, %s", uri, label);

    /* try network streams */
    if (!strncmp (uri, URI_TYPE_FTP, strlen (URI_TYPE_FTP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_FTP);
    else if (!strncmp (uri, URI_TYPE_HTTP, strlen (URI_TYPE_HTTP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_HTTP);
    else if (!strncmp (uri, URI_TYPE_MMS, strlen (URI_TYPE_MMS)))
        mrl = set_network_stream (uri, MRL_RESOURCE_MMS);
    else if (!strncmp (uri, URI_TYPE_NETVDR, strlen (URI_TYPE_NETVDR)))
    {
        mrl = set_network_stream (uri, MRL_RESOURCE_NETVDR);
        player_type = mp->tv_type;
    }
    else if (!strncmp (uri, URI_TYPE_RTP, strlen (URI_TYPE_RTP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_RTP);
    else if (!strncmp (uri, URI_TYPE_RTSP, strlen (URI_TYPE_RTSP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_RTSP);
    else if (!strncmp (uri, URI_TYPE_SMB, strlen (URI_TYPE_SMB)))
        mrl = set_network_stream (uri, MRL_RESOURCE_SMB);
    else if (!strncmp (uri, URI_TYPE_TCP, strlen (URI_TYPE_TCP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_TCP);
    else if (!strncmp (uri, URI_TYPE_UDP, strlen (URI_TYPE_UDP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_UDP);
    else if (!strncmp (uri, URI_TYPE_UNSV, strlen (URI_TYPE_UNSV)))
        mrl = set_network_stream (uri, MRL_RESOURCE_UNSV);

    /* Try DVD video */
    else if (!strncmp (uri, URI_TYPE_DVD, strlen (URI_TYPE_DVD)))
    {
        mrl = set_dvd_stream (uri, MRL_RESOURCE_DVD);
        player_type = mp->dvd_type;
    }
    else if (!strncmp (uri, URI_TYPE_DVDNAV, strlen(URI_TYPE_DVDNAV)))
    {
        mrl = set_dvd_stream (uri, MRL_RESOURCE_DVDNAV);
        player_type = mp->dvd_type;
    }

    /* Try TV */
    else if (!strncmp (uri, URI_TYPE_VDR, strlen (URI_TYPE_VDR)))
    {
        char *device = NULL;
        char *driver = strstr (uri, "#");
        size_t device_len = strlen (uri) - strlen (URI_TYPE_VDR);

        if (driver)
        {
            device_len -= strlen (driver);
            driver++;
            device = malloc (device_len);
            strncpy (device, uri + strlen(URI_TYPE_VDR), device_len);
        }
        else if (device_len)
            device = strdup (uri + strlen (URI_TYPE_VDR));

        mrl = set_tv_stream (device, driver, MRL_RESOURCE_VDR);
        player_type = mp->tv_type;
    }
    /* Try CD Audio */
    else if (!strncmp (uri, URI_TYPE_CDDA, strlen(URI_TYPE_CDDA)))
    {
        mrl = set_cdda_stream (uri, MRL_RESOURCE_CDDA);
    }
    /* default is local files */
    if (!mrl)
        mrl = set_local_stream (uri);

    if (!mrl)
        return 1;

    ENNA_FREE (mp->uri);
    mp->uri = strdup (uri);

    ENNA_FREE (mp->label);
    mp->label = label ? strdup (label) : NULL;

    mp->audio_delay = AUDIO_DELAY_DEFAULT;
    mp->subtitle_delay = SUB_DELAY_DEFAULT;

    /* Get the video configuration parameters from file */
    config_video = enna_config->cfg_video;

    /* Initialization of subtitles variables */
    init_sub_align ();
    init_sub_pos ();
    init_sub_scale ();
    init_framedrop ();
    init_sub_visibility ();

    mp->player_type = player_type;
    mp->player = mp->players[player_type];

    player_mrl_set (mp->player, mrl);

    if (mp->subtitle_alignment != SUB_ALIGNMENT_DEFAULT)
        player_subtitle_set_alignment (mp->player, mp->subtitle_alignment);
    if (mp->subtitle_position != SUB_POSITION_DEFAULT)
        player_subtitle_set_position (mp->player, mp->subtitle_position);
    if (mp->subtitle_scale != SUB_SCALE_DEFAULT)
        player_subtitle_scale (mp->player, mp->subtitle_scale, 1);
    player_subtitle_set_visibility (mp->player,  mp->subtitle_visibility);
    if (mp->framedrop != FRAMEDROP_DEFAULT)
        player_set_framedrop (mp->player, mp->framedrop);

    return 0;
}

static int
mp_play (void)
{
    player_pb_state_t state = player_playback_get_state (mp->player);

    if (state == PLAYER_PB_STATE_PAUSE)
        player_playback_pause (mp->player); /* unpause */
    else if (state == PLAYER_PB_STATE_IDLE)
        player_playback_start (mp->player);

    return 0;
}

static int
mp_seek (double percent)
{
    player_playback_seek (mp->player,
                          (int) (100 * percent), PLAYER_PB_SEEK_PERCENT);
    return 0;
}

static int
mp_stop (void)
{
    player_playback_stop (mp->player);
    return 0;
}

static int
mp_pause (void)
{
    enna_log (ENNA_MSG_INFO, NULL, "pause");
    if (player_playback_get_state (mp->player) == PLAYER_PB_STATE_PLAY)
        player_playback_pause (mp->player);

    return 0;
}

static double
mp_position_get (void)
{
    double time_pos = 0.0;

    time_pos = (double) player_get_time_pos (mp->player) / 1000.0;
    return time_pos < 0.0 ? 0.0 : time_pos;
}

static int
mp_position_percent_get (void)
{
    int percent_pos = 0;

    percent_pos = player_get_percent_pos (mp->player);
    return percent_pos < 0 ? 0 : percent_pos;
}

static double
mp_length_get (void)
{
    return (double) mrl_get_property (mp->player,
                                      NULL, MRL_PROPERTY_LENGTH) / 1000.0;
}

static void
mp_video_resize (int x, int y, int w, int h)
{
    const int flags = PLAYER_X_WINDOW_X | PLAYER_X_WINDOW_Y |
                PLAYER_X_WINDOW_W | PLAYER_X_WINDOW_H;

    /* if w or h is 0, libplayer guess the best size automatically */
    player_x_window_set_properties (mp->player, x, y, w, h, flags);
}

static void
mp_event_cb_set (void (*event_cb)(void *data,
                                  enna_mediaplayer_event_t event),
                 void *data)
{
    /* FIXME: function to call when end of stream is send by libplayer */
    mp->event_cb_data = data;
    mp->event_cb = event_cb;
}

static void
mp_send_key (enna_input event)
{
    const player_vdr_t vdr_keymap[] = {
        [ENNA_INPUT_MENU]       = PLAYER_VDR_MENU,
        [ENNA_INPUT_LEFT]       = PLAYER_VDR_LEFT,
        [ENNA_INPUT_RIGHT]      = PLAYER_VDR_RIGHT,
        [ENNA_INPUT_UP]         = PLAYER_VDR_UP,
        [ENNA_INPUT_DOWN]       = PLAYER_VDR_DOWN,
        [ENNA_INPUT_HOME]       = PLAYER_VDR_PREVIOUS,
        [ENNA_INPUT_END]        = PLAYER_VDR_NEXT,
        [ENNA_INPUT_NEXT]       = PLAYER_VDR_CHANNELPLUS,
        [ENNA_INPUT_PREV]       = PLAYER_VDR_CHANNELMINUS,
        [ENNA_INPUT_OK]         = PLAYER_VDR_OK,
        [ENNA_INPUT_EXIT]       = PLAYER_VDR_BACK,
        [ENNA_INPUT_QUIT]       = PLAYER_VDR_BACK,
        [ENNA_INPUT_KEY_N]      = PLAYER_VDR_INFO,
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
        [ENNA_INPUT_KEY_Q]      = PLAYER_VDR_RED,
        [ENNA_INPUT_KEY_W]      = PLAYER_VDR_GREEN,
        [ENNA_INPUT_KEY_E]      = PLAYER_VDR_YELLOW,
        [ENNA_INPUT_KEY_R]      = PLAYER_VDR_BLUE,
        [ENNA_INPUT_KEY_A]      = PLAYER_VDR_PLAY,
        [ENNA_INPUT_KEY_S]      = PLAYER_VDR_PAUSE,
        [ENNA_INPUT_KEY_D]      = PLAYER_VDR_STOP,
        [ENNA_INPUT_KEY_F]      = PLAYER_VDR_RECORD,
        [ENNA_INPUT_KEY_B]      = PLAYER_VDR_CHANNELPREVIOUS,
        [ENNA_INPUT_KEY_G]      = PLAYER_VDR_USER_1,
        [ENNA_INPUT_KEY_H]      = PLAYER_VDR_USER_2,
        [ENNA_INPUT_KEY_J]      = PLAYER_VDR_USER_3,
        [ENNA_INPUT_KEY_K]      = PLAYER_VDR_USER_4,
        [ENNA_INPUT_KEY_L]      = PLAYER_VDR_USER_5,
        [ENNA_INPUT_KEY_Z]      = PLAYER_VDR_FASTREW,
        [ENNA_INPUT_KEY_X]      = PLAYER_VDR_FASTFWD,
        [ENNA_INPUT_KEY_C]      = PLAYER_VDR_AUDIO,
        [ENNA_INPUT_KEY_V]      = PLAYER_VDR_SUBTITLES,
        [ENNA_INPUT_PLUS]       = PLAYER_VDR_VOLPLUS,
        [ENNA_INPUT_MINUS]      = PLAYER_VDR_VOLMINUS,
        [ENNA_INPUT_KEY_M]      = PLAYER_VDR_MUTE,
        [ENNA_INPUT_KEY_T]      = PLAYER_VDR_SCHEDULE,
        [ENNA_INPUT_KEY_Y]      = PLAYER_VDR_CHANNELS,
        [ENNA_INPUT_KEY_U]      = PLAYER_VDR_TIMERS,
        [ENNA_INPUT_KEY_I]      = PLAYER_VDR_RECORDINGS,
        [ENNA_INPUT_KEY_O]      = PLAYER_VDR_SETUP,
        [ENNA_INPUT_KEY_P]      = PLAYER_VDR_COMMANDS,
    };

    if (mp->uri && strncmp (mp->uri, URI_TYPE_VDR, strlen (URI_TYPE_VDR)) &&
        strncmp (mp->uri, URI_TYPE_NETVDR, strlen (URI_TYPE_NETVDR)))
        return;

    if (event >= ARRAY_NB_ELEMENTS (vdr_keymap) || event < 0)
        return;

    switch (event)
    {
    case ENNA_INPUT_UNKNOWN:
        enna_log (ENNA_MSG_INFO, NULL, "Unknown event received: %d", event);
        break;
    default:
        player_vdr (mp->player, vdr_keymap[event]);
    }
}

/* externally accessible functions */
int
enna_mediaplayer_supported_uri_type (enna_mediaplayer_uri_type_t type)
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
    };

    if (type >= ARRAY_NB_ELEMENTS (type_list) || type < 0)
        return 0;

    return libplayer_wrapper_supported_res (type_list[type].p_type,
                                            type_list[type].res);
}

#define ENNA_MP_PLAYER_INIT(type)                                            \
    do                                                                       \
    {                                                                        \
        mp->players[type] =                                                  \
            player_init (type, ao, vo, verbosity, enna->ee_winid, event_cb); \
        if (!mp->players[type])                                              \
            goto err;                                                        \
    }                                                                        \
    while (0)

#define CFG_VIDEO(field) \
    enna_config_value_store (&video->field, #field, ENNA_CONFIG_STRING, pair);

int
enna_mediaplayer_init (void)
{
    Enna_Config_Data *cfgdata;
    Enna_Config_Video *video;
    char *value = NULL;

    player_type_t type = PLAYER_TYPE_MPLAYER;
    player_type_t dvd_type = PLAYER_TYPE_XINE;
    player_type_t tv_type = PLAYER_TYPE_XINE;
    player_vo_t vo = PLAYER_VO_AUTO;
    player_ao_t ao = PLAYER_AO_AUTO;
    player_verbosity_level_t verbosity = PLAYER_MSG_WARNING;

    int dvd_set = 0, tv_set = 0;

    /* set default video config */
    enna_config->cfg_video = calloc (1, sizeof (Enna_Config_Video));
    video = enna_config->cfg_video;

    /* Load Config file values */
    cfgdata = enna_config_module_pair_get ("mediaplayer");

    enna_log (ENNA_MSG_INFO, NULL, "parameters:");

    if (cfgdata)
    {
        Eina_List *l;

        for (l = cfgdata->pair; l; l = l->next)
        {
            Config_Pair *pair = l->data;

            if (!strcmp("type", pair->key))
            {
                enna_config_value_store (&value, "type",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, NULL,
                          " * type: %s", value);

                if (!strcmp ("gstreamer", value))
                    type = PLAYER_TYPE_GSTREAMER;
                else if (!strcmp ("mplayer", value))
                    type = PLAYER_TYPE_MPLAYER;
                else if (!strcmp ("vlc", value))
                    type = PLAYER_TYPE_VLC;
                else if (!strcmp ("xine", value))
                    type = PLAYER_TYPE_XINE;
                else
                    enna_log (ENNA_MSG_WARNING, NULL,
                              "   - unknown type, 'mplayer' used instead");
            }
            else if (!strcmp ("dvd_type", pair->key))
            {
                enna_config_value_store (&value, "dvd_type",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, NULL,
                          " * dvd_type: %s", value);

                if (!strcmp ("gstreamer", value))
                    dvd_type = PLAYER_TYPE_GSTREAMER;
                else if (!strcmp ("mplayer", value))
                    dvd_type = PLAYER_TYPE_MPLAYER;
                else if (!strcmp ("vlc", value))
                    dvd_type = PLAYER_TYPE_VLC;
                else if (!strcmp ("xine", value))
                    dvd_type = PLAYER_TYPE_XINE;
                else
                    enna_log (ENNA_MSG_WARNING, NULL,
                              "   - unknown dvd_type, 'xine' used instead");
                dvd_set = 1;
            }
            else if (!strcmp ("tv_type", pair->key))
            {
                enna_config_value_store (&value, "tv_type",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, NULL,
                          " * tv_type: %s", value);

                if (!strcmp ("gstreamer", value))
                    tv_type = PLAYER_TYPE_GSTREAMER;
                else if (!strcmp ("mplayer", value))
                    tv_type = PLAYER_TYPE_MPLAYER;
                else if (!strcmp ("vlc", value))
                    tv_type = PLAYER_TYPE_VLC;
                else if (!strcmp ("xine", value))
                    tv_type = PLAYER_TYPE_XINE;
                else
                    enna_log (ENNA_MSG_WARNING, NULL,
                              "   - unknown tv_type, 'xine' used instead");
                tv_set = 1;
            }
            else if (!strcmp ("video_out", pair->key))
            {
                enna_config_value_store (&value, "video_out",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, NULL,
                          " * video out: %s", value);

                if (!strcmp ("auto", value))
                    vo = PLAYER_VO_AUTO;
                else if (!strcmp ("vdpau", value))
                    vo = PLAYER_VO_VDPAU;
                else if (!strcmp ("x11", value))
                    vo = PLAYER_VO_X11;
                else if (!strcmp ("xv", value))
                    vo = PLAYER_VO_XV;
                else if (!strcmp ("gl", value))
                    vo = PLAYER_VO_GL;
                else if (!strcmp ("fb", value))
                    vo = PLAYER_VO_FB;
                else
                    enna_log (ENNA_MSG_WARNING, NULL,
                              "   - unknown video_out, 'auto' used instead");
            }
            else if (!strcmp ("audio_out", pair->key))
            {
                enna_config_value_store (&value, "audio_out",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, NULL,
                          " * audio out: %s", value);

                if (!strcmp ("auto", value))
                    ao = PLAYER_AO_AUTO;
                else if (!strcmp ("alsa", value))
                    ao = PLAYER_AO_ALSA;
                else if (!strcmp ("oss", value))
                    ao = PLAYER_AO_OSS;
                else
                    enna_log (ENNA_MSG_WARNING, NULL,
                              "   - unknown audio_out, 'auto' used instead");
            }
            else if (!strcmp ("verbosity", pair->key))
            {
                enna_config_value_store (&value, "verbosity",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, NULL,
                          " * verbosity level: %s", value);

                if (!strcmp ("verbose", value))
                    verbosity = PLAYER_MSG_VERBOSE;
                else if (!strcmp ("info", value))
                    verbosity = PLAYER_MSG_INFO;
                else if (!strcmp ("warning", value))
                    verbosity = PLAYER_MSG_WARNING;
                else if (!strcmp ("error", value))
                    verbosity = PLAYER_MSG_ERROR;
                else if (!strcmp ("critical", value))
                    verbosity = PLAYER_MSG_CRITICAL;
                else if (!strcmp ("none", value))
                    verbosity = PLAYER_MSG_NONE;
                else
                    enna_log (ENNA_MSG_WARNING, NULL,
                              "   - unknown verbosity, " \
                              "'warning' used instead");
            }

            CFG_VIDEO (sub_align);
            CFG_VIDEO (sub_pos);
            CFG_VIDEO (sub_scale);
            CFG_VIDEO (sub_visibility);
            CFG_VIDEO (framedrop);
        }
    }

    if (!value)
        enna_log (ENNA_MSG_INFO, NULL,
                  " * use all parameters by default");

    /* Main player type is mandatory! */
    if (!libplayer_wrapper_enabled (type))
        goto err;

    /*
     * When dvd_type or tv_type are set in the enna.cfg config file, then
     * these values are used. But if the wrapper is not enabled in libplayer,
     * an error is returned.
     * Otherwise, if nothing is changed by the user but the wrapper is not
     * enabled, then the main type is used instead.
     */
    if (dvd_type != type && !libplayer_wrapper_enabled (dvd_type))
    {
        if (dvd_set)
            goto err;
        dvd_type = type;
    }

    if (tv_type != type && !libplayer_wrapper_enabled (tv_type))
    {
        if (tv_set)
            goto err;
        tv_type = type;
    }

    mp = calloc (1, sizeof (Enna_Mediaplayer));

    mp->key_down_event_handler =
        ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, event_key_down, NULL);
    mp->mouse_button_event_handler =
        ecore_event_handler_add (ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                 event_mouse_button, NULL);
    mp->mouse_move_event_handler =
        ecore_event_handler_add (ECORE_EVENT_MOUSE_MOVE,
                                 event_mouse_move, NULL);

    mp->pipe = ecore_pipe_add (pipe_read, NULL);

    ENNA_MP_PLAYER_INIT (type);
    if (dvd_type != type)
        ENNA_MP_PLAYER_INIT (dvd_type);
    if (tv_type != type && tv_type != dvd_type)
        ENNA_MP_PLAYER_INIT (tv_type);

    mp_event_cb_set (_event_cb, NULL);

    mp->uri = NULL;
    mp->label = NULL;
    mp->default_type = type;
    mp->dvd_type = dvd_type;
    mp->tv_type = tv_type;
    mp->player = mp->players[type];
    mp->player_type = type;
    mp->play_state = STOPPED;

    /* Create Ecore Event ID */
    ENNA_EVENT_MEDIAPLAYER_EOS = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_METADATA_UPDATE = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_START = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_STOP = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_PAUSE = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_UNPAUSE = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_PREV = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_NEXT = ecore_event_type_new ();
    ENNA_EVENT_MEDIAPLAYER_SEEK = ecore_event_type_new ();

    return 1;

 err:
    enna_log (ENNA_MSG_ERROR, NULL, "Mediaplayer initialization");
    enna_mediaplayer_shutdown ();
    return 0;
}

void
enna_mediaplayer_shutdown(void)
{
    int i;

    if (!mp)
        return;

    ENNA_FREE (mp->uri);
    ENNA_FREE (mp->label);
    ENNA_EVENT_HANDLER_DEL(mp->key_down_event_handler);
    ENNA_EVENT_HANDLER_DEL(mp->mouse_button_event_handler);
    ENNA_EVENT_HANDLER_DEL(mp->mouse_move_event_handler);
    if (mp->pipe)
        ecore_pipe_del (mp->pipe);
    player_playback_stop (mp->player);
    for (i = 0; i < MAX_PLAYERS; i++)
        if (mp->players[i])
            player_uninit (mp->players[i]);
    ENNA_FREE (mp);
}

char *enna_mediaplayer_get_current_uri(Enna_Playlist *enna_playlist)
{
  list_item_t *item;

  item = eina_list_nth (enna_playlist->playlist, enna_playlist->selected);
  if (!item->uri)
    return NULL;
  return strdup (item->uri);
}


void
enna_mediaplayer_uri_append (Enna_Playlist *enna_playlist,
                             const char *uri, const char *label)
{
    list_item_t *item = calloc (1, sizeof (list_item_t));
    item->uri = uri ? strdup (uri) : NULL;
    item->label = label ? strdup (label) : NULL;
    enna_playlist->playlist =
        eina_list_append (enna_playlist->playlist, item);
}

int
enna_mediaplayer_play(Enna_Playlist *enna_playlist)
{
    switch (mp->play_state)
    {
    case STOPPED:
    {
        list_item_t *item;
        item = eina_list_nth (enna_playlist->playlist,
                              enna_playlist->selected);
        mp_stop ();
        if (item && item->uri)
            mp_file_set (item->uri, item->label);
        mp_play ();
        mp->play_state = PLAYING;
        ecore_event_add (ENNA_EVENT_MEDIAPLAYER_START, NULL, NULL, NULL);
        break;
    }
    case PLAYING:
        enna_mediaplayer_pause ();
        break;
    case PAUSE:
        mp_play ();
        mp->play_state = PLAYING;
        ecore_event_add (ENNA_EVENT_MEDIAPLAYER_UNPAUSE,
                         NULL, NULL, NULL);
        break;
    default:
        break;
    }

    return 0;
}

int
enna_mediaplayer_select_nth (Enna_Playlist *enna_playlist,int n)
{
    if (n < 0 || n > eina_list_count (enna_playlist->playlist) - 1)
        return -1;

    enna_log (ENNA_MSG_EVENT, NULL, "select %d", n);
    enna_playlist->selected = n;

    return 0;
}

int
enna_mediaplayer_selected_get (Enna_Playlist *enna_playlist)
{
    return enna_playlist->selected;
}

int
enna_mediaplayer_stop (void)
{
    mp_stop ();
    mp->play_state = STOPPED;
    ecore_event_add (ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);

    return 0;
}

int
enna_mediaplayer_pause (void)
{
    mp_pause ();
    mp->play_state = PAUSE;
    ecore_event_add (ENNA_EVENT_MEDIAPLAYER_PAUSE, NULL, NULL, NULL);

    return 0;
}

static void
enna_mediaplayer_change (Enna_Playlist *enna_playlist, int type)
{
    list_item_t *item;

    item = eina_list_nth (enna_playlist->playlist, enna_playlist->selected);
    enna_log (ENNA_MSG_EVENT, NULL, "select %d", enna_playlist->selected);
    if (!item)
        return;

    enna_mediaplayer_stop ();
    enna_mediaplayer_play (enna_playlist);
    ecore_event_add (type, NULL, NULL, NULL);
}

int
enna_mediaplayer_next (Enna_Playlist *enna_playlist)
{
    enna_playlist->selected++;
    if (enna_playlist->selected >
        eina_list_count (enna_playlist->playlist) - 1)
    {
        enna_playlist->selected--;
        return -1;
    }

    enna_mediaplayer_change (enna_playlist, ENNA_EVENT_MEDIAPLAYER_NEXT);
    return 0;
}

int
enna_mediaplayer_prev (Enna_Playlist *enna_playlist)
{
    enna_playlist->selected--;
    if (enna_playlist->selected < 0)
    {
        enna_playlist->selected = 0;
        return -1;
    }

    enna_mediaplayer_change (enna_playlist, ENNA_EVENT_MEDIAPLAYER_PREV);
    return 0;
}

double
enna_mediaplayer_position_get (void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
        mp_position_get (): 0.0;
}

int
enna_mediaplayer_position_percent_get (void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
            mp_position_percent_get () : 0;
}

int
enna_mediaplayer_position_set (double position)
{
    enna_log (ENNA_MSG_EVENT, NULL, "Seeking to: %f seconds", position);

    if (mp->play_state == PAUSE || mp->play_state == PLAYING)
    {
        Enna_Event_Mediaplayer_Seek_Data *ev;

        ev = calloc (1, sizeof (Enna_Event_Mediaplayer_Seek_Data));
        if (!ev)
            return 0;

        ev->seek_value = position;
        ecore_event_add (ENNA_EVENT_MEDIAPLAYER_SEEK, ev, NULL, NULL);
        player_playback_seek (mp->player,
                              position, PLAYER_PB_SEEK_ABSOLUTE);

    }

    return 0;
}

double
enna_mediaplayer_length_get (void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
        mp_length_get () : 0.0;
}

int
enna_mediaplayer_seek (double percent)
{
    enna_log (ENNA_MSG_EVENT, NULL,
              "Seeking to: %d%%", (int) (100 * percent));

    if (mp->play_state == PAUSE || mp->play_state == PLAYING)
    {
        Enna_Event_Mediaplayer_Seek_Data *ev;

        ev = calloc (1, sizeof (Enna_Event_Mediaplayer_Seek_Data));
        if (!ev)
            return 0;

        ev->seek_value = percent;
        ecore_event_add (ENNA_EVENT_MEDIAPLAYER_SEEK, ev, NULL, NULL);
        return mp_seek (percent);
    }

    return 0;
}

void
enna_mediaplayer_default_seek_backward (void)
{
    int pos;
    pos = enna_mediaplayer_position_percent_get ();
    enna_mediaplayer_seek (((double) pos - SEEK_STEP_DEFAULT) / 100.0);
}

void
enna_mediaplayer_default_seek_forward (void)
{
    int pos;
    pos = enna_mediaplayer_position_percent_get ();
    enna_mediaplayer_seek (((double) pos + SEEK_STEP_DEFAULT) / 100.0);
}

void
enna_mediaplayer_video_resize (int x, int y, int w, int h)
{
    mp_video_resize (x, y, w, h);
}

int
enna_mediaplayer_playlist_load (const char *filename)
{
    return 0;
}

int
enna_mediaplayer_playlist_save (const char *filename)
{
    return 0;
}

void
enna_mediaplayer_playlist_clear (Enna_Playlist *enna_playlist)
{
    eina_list_free (enna_playlist->playlist);
    enna_playlist->playlist = NULL;
    enna_playlist->selected = 0;
}

Enna_Metadata *
enna_mediaplayer_metadata_get (Enna_Playlist *enna_playlist)
{
    list_item_t *item;

    item = eina_list_nth (enna_playlist->playlist, enna_playlist->selected);
    if (!item)
        return NULL;

    if (item->uri)
        return enna_metadata_meta_new ((char *) item->uri);

    return NULL;
}

int
enna_mediaplayer_playlist_count (Enna_Playlist *enna_playlist)
{
    return eina_list_count (enna_playlist->playlist);
}

PLAY_STATE
enna_mediaplayer_state_get (void)
{
    return mp->play_state;
}

Enna_Playlist *
enna_mediaplayer_playlist_create (void)
{
    Enna_Playlist *enna_playlist;

    enna_playlist = calloc (1, sizeof (Enna_Playlist));
    enna_playlist->selected = 0;
    enna_playlist->playlist = NULL;
    return enna_playlist;
}

void
enna_mediaplayer_playlist_free (Enna_Playlist *enna_playlist)
{
    eina_list_free (enna_playlist->playlist);
    free (enna_playlist);
}

void
enna_mediaplayer_playlist_stop_clear (Enna_Playlist *enna_playlist)
{
    enna_mediaplayer_playlist_clear (enna_playlist);
    mp_stop ();
    mp->play_state = STOPPED;
    ecore_event_add (ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);
}

void
enna_mediaplayer_send_input (enna_input event)
{
    mp_send_key (event);
}

int
enna_mediaplayer_volume_get (void)
{
    return player_audio_volume_get (mp->player);
}

void
enna_mediaplayer_volume_set (int value)
{
    player_audio_volume_set (mp->player, value);
}

void
enna_mediaplayer_default_increase_volume (void)
{
    int vol = enna_mediaplayer_volume_get ();
    player_audio_volume_set (mp->player, vol + VOLUME_STEP_DEFAULT);
}

void
enna_mediaplayer_default_decrease_volume (void)
{
    int vol = enna_mediaplayer_volume_get ();
    player_audio_volume_set (mp->player, vol - VOLUME_STEP_DEFAULT);
}

void
enna_mediaplayer_mute (void)
{
    player_mute_t m;

    m = player_audio_mute_get (mp->player);
    player_audio_mute_set (mp->player, (m == PLAYER_MUTE_ON) ?
                           PLAYER_MUTE_OFF : PLAYER_MUTE_ON);
}

int
enna_mediaplayer_mute_get (void)
{
    player_mute_t m;
    m = player_audio_mute_get (mp->player);
    if (m == PLAYER_MUTE_ON)
        return 1;
    else
        return 0;
}

void
enna_mediaplayer_audio_previous (void)
{
    player_audio_prev (mp->player);
}

void
enna_mediaplayer_audio_next (void)
{
    player_audio_next (mp->player);
}

void
enna_mediaplayer_audio_increase_delay (void)
{
    mp->audio_delay += 100;
    player_audio_set_delay  (mp->player, mp->audio_delay, 0);
}

void
enna_mediaplayer_audio_decrease_delay (void)
{
    mp->audio_delay -= 100;
    player_audio_set_delay  (mp->player, mp->audio_delay, 0);
}

void
enna_mediaplayer_subtitle_set_visibility (void)
{
    mp->subtitle_visibility = !mp->subtitle_visibility;
    player_subtitle_set_visibility (mp->player, mp->subtitle_visibility);
}

void
enna_mediaplayer_subtitle_previous (void)
{
    player_subtitle_prev (mp->player);
}

void
enna_mediaplayer_subtitle_next (void)
{
    player_subtitle_next (mp->player);
}

void
enna_mediaplayer_subtitle_set_alignment (void)
{
    mp->subtitle_alignment = (mp->subtitle_alignment + 1) % 3;
    player_subtitle_set_alignment (mp->player, mp->subtitle_alignment);
}

void
enna_mediaplayer_subtitle_increase_position (void)
{
    mp->subtitle_position += 1;
    player_subtitle_set_position (mp->player, mp->subtitle_position);
}

void
enna_mediaplayer_subtitle_decrease_position (void)
{
    mp->subtitle_position -= 1;
    player_subtitle_set_position (mp->player, mp->subtitle_position);
}

void
enna_mediaplayer_subtitle_increase_scale (void)
{
    mp->subtitle_scale += 1;
    player_subtitle_scale  (mp->player, mp->subtitle_scale, 1);
}

void
enna_mediaplayer_subtitle_decrease_scale (void)
{
    mp->subtitle_scale -= 1;
    player_subtitle_scale  (mp->player, mp->subtitle_scale, 1);
}

void
enna_mediaplayer_subtitle_increase_delay (void)
{
    mp->subtitle_delay += 100;
    player_subtitle_set_delay  (mp->player, mp->subtitle_delay);
}

void
enna_mediaplayer_subtitle_decrease_delay (void)
{
    mp->subtitle_delay -= 100;
    player_subtitle_set_delay  (mp->player, mp->subtitle_delay);
}

void
enna_mediaplayer_set_framedrop (void)
{
    mp->framedrop = (mp->framedrop + 1) % 3;
    player_set_framedrop (mp->player, mp->framedrop);
}
