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
#include <Ecore_X.h>
#include <player.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "logs.h"
#include "mediaplayer.h"

#define ENNA_MODULE_NAME "libplayer"

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
#define URI_TYPE_DVD      "dvd://"
#define URI_TYPE_DVDNAV   "dvdnav://"
#define URI_TYPE_VDR      "vdr:/"

#define MAX_PLAYERS 4

typedef struct _Enna_Module_libplayer
{
    Evas *evas;
    player_t *player;
    player_t *players[MAX_PLAYERS];
    Enna_Module *em;
    void (*event_cb)(void *data, enna_mediaplayer_event_t event);
    void *event_cb_data;
    char *uri;
    char *label;
    player_type_t player_type;
    player_type_t default_type;
    player_type_t dvd_type;
    player_type_t tv_type;
    Ecore_Event_Handler *key_down_event_handler;
    Ecore_Pipe *pipe;
} Enna_Module_libplayer;

static Enna_Module_libplayer *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _class_init(int dummy)
{
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "class init");
}

static void _class_shutdown(int dummy)
{
    int i;

    if (mod->uri)
        free(mod->uri);
    if (mod->label)
        free(mod->label);
    ecore_event_handler_del(mod->key_down_event_handler);
    ecore_pipe_del(mod->pipe);
    player_playback_stop(mod->player);
    for (i = 0; i < MAX_PLAYERS; i++)
        if (mod->players[i])
            player_uninit(mod->players[i]);
}

static mrl_t * set_network_stream(const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_network_args_t *args;

    args = calloc(1, sizeof(mrl_resource_network_args_t));
    args->url = strdup(uri);

    if (type == MRL_RESOURCE_NETVDR)
        mrl = mrl_new(mod->players[mod->tv_type], type, args);
    else
        mrl = mrl_new(mod->players[mod->default_type], type, args);

    return mrl;

}

static mrl_t * set_dvd_stream(const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_videodisc_args_t *args;
    char *meta;
    uint32_t prop = 0;
    int tmp = 0;
    int title = 0;

    args = calloc(1, sizeof(mrl_resource_videodisc_args_t));
    mrl = mrl_new(mod->players[mod->dvd_type], type, args);

    meta = mrl_get_metadata_dvd (mod->players[mod->dvd_type], mrl, (uint8_t *) &prop);
    if (meta)
    {
        enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME, "Meta DVD VolumeID: %s", meta);
        free (meta);
    }

    if (prop)
    {
        int i;

        enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME, "Meta DVD Titles: %i", prop);

        for (i = 1; i <= prop; i++)
        {
            uint32_t chapters, angles, length;

            chapters = mrl_get_metadata_dvd_title (mod->players[mod->dvd_type], mrl, i,
                MRL_METADATA_DVD_TITLE_CHAPTERS);
            angles = mrl_get_metadata_dvd_title (mod->players[mod->dvd_type], mrl, i,
                MRL_METADATA_DVD_TITLE_ANGLES);
            length = mrl_get_metadata_dvd_title (mod->players[mod->dvd_type], mrl, i,
                MRL_METADATA_DVD_TITLE_LENGTH);

            enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,"Meta DVD Title %i (%.2f sec), Chapters: %i, Angles: %i",
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

static mrl_t * set_tv_stream(const char *device, const char *driver, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_tv_args_t *args;

    args = calloc(1, sizeof(mrl_resource_tv_args_t));

    if (type == MRL_RESOURCE_VDR)
    {
        enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME, "VDR stream; device: '%s' driver: '%s'", device, driver);
        if(device)
            args->device = strdup(device);
        if(driver)
            args->driver = strdup(driver);
    }

    mrl = mrl_new(mod->players[mod->tv_type], type, args);
    return mrl;
}

static mrl_t * set_local_stream(const char *uri)
{
    mrl_t *mrl;
    mrl_resource_local_args_t *args;

    args = calloc(1, sizeof(mrl_resource_local_args_t));
    args->location = strdup(uri);
    mrl = mrl_new(mod->players[mod->default_type], MRL_RESOURCE_FILE, args);

    return mrl;
}

static int _class_file_set(const char *uri, const char *label)
{
    mrl_t *mrl = NULL;
    player_type_t player_type = mod->default_type;

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
        player_type = mod->tv_type;
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

    /* Try Dvd video */
    else if (!strncmp(uri, URI_TYPE_DVD, strlen(URI_TYPE_DVD))) {
        mrl = set_dvd_stream(uri, MRL_RESOURCE_DVD);
        player_type = mod->dvd_type;
    } else if (!strncmp(uri, URI_TYPE_DVDNAV, strlen(URI_TYPE_DVDNAV))) {
        mrl = set_dvd_stream(uri, MRL_RESOURCE_DVDNAV);
        player_type = mod->dvd_type;
    }

    /* Try TV */
    else if (!strncmp(uri, URI_TYPE_VDR, strlen(URI_TYPE_VDR))) {
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
        player_type = mod->tv_type;
    }

    /* default is local files */
    if (!mrl)
        mrl = set_local_stream(uri);

    if (!mrl)
        return 1;

    if (mod->uri)
        free(mod->uri);
    mod->uri = strdup(uri);

    if (mod->label)
        free(mod->label);
    mod->label = label ? strdup(label) : NULL;

    mod->player_type = player_type;
    mod->player = mod->players[player_type];

    player_mrl_set(mod->player, mrl);
    return 0;
}

static int _class_play(void)
{
    player_pb_state_t state = player_playback_get_state(mod->player);

    if (state == PLAYER_PB_STATE_PAUSE)
        player_playback_pause(mod->player); /* unpause */
    else if (state == PLAYER_PB_STATE_IDLE)
        player_playback_start(mod->player);
    return 0;
}

static int _class_seek(double percent)
{
    player_playback_seek(mod->player,
                         (int) (100 * percent), PLAYER_PB_SEEK_PERCENT);
    return 0;
}

static int _class_stop(void)
{
    player_playback_stop(mod->player);
    return 0;
}

static int _class_pause(void)
{
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "pause");
    if (player_playback_get_state(mod->player) == PLAYER_PB_STATE_PLAY)
        player_playback_pause(mod->player);
    return 0;
}

static double _class_position_get()
{
    double time_pos = 0.0;

    time_pos = (double) player_get_time_pos(mod->player) / 1000.0;
    return time_pos < 0.0 ? 0.0 : time_pos;
}

static double _class_length_get()
{
    return (double) mrl_get_property(mod->player,
                                     NULL, MRL_PROPERTY_LENGTH) / 1000.0;
}

static void _class_video_resize(int x, int y, int w, int h)
{
    int flags = PLAYER_X_WINDOW_X | PLAYER_X_WINDOW_Y |
                PLAYER_X_WINDOW_W | PLAYER_X_WINDOW_H;

    /* if w or h is 0, libplayer guess the best size automatically */
    player_x_window_set_properties(mod->player, x, y, w, h, flags);
}

static void _class_event_cb_set(void (*event_cb)(void *data,
                                                 enna_mediaplayer_event_t event),
                                void *data)
{
    /* FIXME: function to call when end of stream is send by libplayer */

    mod->event_cb_data = data;
    mod->event_cb = event_cb;
}

static void _class_send_key(enna_key_t key)
{
    player_vdr_t vdr_keymap[] = {
        [ENNA_KEY_MENU]       = PLAYER_VDR_MENU,
        [ENNA_KEY_QUIT]       = PLAYER_VDR_POWER,
        [ENNA_KEY_LEFT]       = PLAYER_VDR_LEFT,
        [ENNA_KEY_RIGHT]      = PLAYER_VDR_RIGHT,
        [ENNA_KEY_UP]         = PLAYER_VDR_UP,
        [ENNA_KEY_DOWN]       = PLAYER_VDR_DOWN,
        [ENNA_KEY_HOME]       = PLAYER_VDR_PREVIOUS,
        [ENNA_KEY_END]        = PLAYER_VDR_NEXT,
        [ENNA_KEY_PAGE_UP]    = PLAYER_VDR_CHANNELPLUS,
        [ENNA_KEY_PAGE_DOWN]  = PLAYER_VDR_CHANNELMINUS,
        [ENNA_KEY_OK]         = PLAYER_VDR_OK,
        [ENNA_KEY_CANCEL]     = PLAYER_VDR_BACK,
        [ENNA_KEY_SPACE]      = PLAYER_VDR_INFO,
        [ENNA_KEY_0]          = PLAYER_VDR_0,
        [ENNA_KEY_1]          = PLAYER_VDR_1,
        [ENNA_KEY_2]          = PLAYER_VDR_2,
        [ENNA_KEY_3]          = PLAYER_VDR_3,
        [ENNA_KEY_4]          = PLAYER_VDR_4,
        [ENNA_KEY_5]          = PLAYER_VDR_5,
        [ENNA_KEY_6]          = PLAYER_VDR_6,
        [ENNA_KEY_7]          = PLAYER_VDR_7,
        [ENNA_KEY_8]          = PLAYER_VDR_8,
        [ENNA_KEY_9]          = PLAYER_VDR_9,
        [ENNA_KEY_Q]          = PLAYER_VDR_RED,
        [ENNA_KEY_W]          = PLAYER_VDR_GREEN,
        [ENNA_KEY_E]          = PLAYER_VDR_YELLOW,
        [ENNA_KEY_R]          = PLAYER_VDR_BLUE,
        [ENNA_KEY_A]          = PLAYER_VDR_PLAY,
        [ENNA_KEY_S]          = PLAYER_VDR_PAUSE,
        [ENNA_KEY_D]          = PLAYER_VDR_STOP,
        [ENNA_KEY_F]          = PLAYER_VDR_RECORD,
        [ENNA_KEY_G]          = PLAYER_VDR_CHANNELPREVIOUS,
        [ENNA_KEY_H]          = PLAYER_VDR_USER_1,
        [ENNA_KEY_J]          = PLAYER_VDR_USER_2,
        [ENNA_KEY_K]          = PLAYER_VDR_USER_3,
        [ENNA_KEY_L]          = PLAYER_VDR_USER_4,
        [ENNA_KEY_Z]          = PLAYER_VDR_FASTREW,
        [ENNA_KEY_X]          = PLAYER_VDR_FASTFWD,
        [ENNA_KEY_C]          = PLAYER_VDR_AUDIO,
        [ENNA_KEY_V]          = PLAYER_VDR_SUBTITLES,
        [ENNA_KEY_B]          = PLAYER_VDR_VOLPLUS,
        [ENNA_KEY_N]          = PLAYER_VDR_VOLMINUS,
        [ENNA_KEY_M]          = PLAYER_VDR_MUTE,
        [ENNA_KEY_T]          = PLAYER_VDR_SCHEDULE,
        [ENNA_KEY_Y]          = PLAYER_VDR_CHANNELS,
        [ENNA_KEY_U]          = PLAYER_VDR_TIMERS,
        [ENNA_KEY_I]          = PLAYER_VDR_RECORDINGS,
        [ENNA_KEY_O]          = PLAYER_VDR_SETUP,
        [ENNA_KEY_P]          = PLAYER_VDR_COMMANDS,
    };

    if (strncmp(mod->uri, URI_TYPE_VDR, strlen(URI_TYPE_VDR)) &&
        strncmp(mod->uri, URI_TYPE_NETVDR, strlen(URI_TYPE_NETVDR)))
        return;

    if (key >= ARRAY_NB_ELEMENTS(vdr_keymap) || key < 0)
        return;

    switch(key)
    {
    case ENNA_KEY_UNKNOWN:
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 "Unknown key pressed %i", key);
        break;
    default:
        player_vdr(mod->player, vdr_keymap[key]);
    }
}

static void _pipe_read(void *data, void *buf, unsigned int nbyte)
{
    enna_mediaplayer_event_t *event = buf;

    if (!mod->event_cb || !buf)
        return;

    mod->event_cb(mod->event_cb_data, *event);
}

static int _event_cb(player_event_t e, void *data)
{
    enna_mediaplayer_event_t event;

    if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
    {
        event = ENNA_MP_EVENT_EOF;
        ecore_pipe_write(mod->pipe, &event, sizeof(event));
    }
    return 0;
}

static int _x_event_key_down(void *data, int type, void *event)
{
    Ecore_X_Event_Key_Down *e;
    e = event;
    /*
       HACK !
       If e->win is the same than enna winid, don't manage this event
       ecore_evas_x will do this for us.
       But if e->win is different than enna winid event are sent to
       libplayer subwindow and we must broadcast this event to Evas
    */
    if (e->win != enna->ee_winid)
    {
        enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                 "Ecore_X_Event_Key_Down %s", e->keyname);
        evas_event_feed_key_down(enna->evas, e->keyname, e->keysymbol,
                                 e->key_compose, NULL, e->time, NULL);
    }
   return 1;
}

static Enna_Class_MediaplayerBackend class = {
    "libplayer",
    1,
    {
        _class_init,
        _class_shutdown,
        _class_file_set,
        _class_play,
        _class_seek,
        _class_stop,
        _class_pause,
        _class_position_get,
        _class_length_get,
        _class_video_resize,
        _class_event_cb_set,
        NULL,
        _class_send_key,
    }
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_BACKEND,
    "backend_libplayer"
};

void module_init(Enna_Module *em)
{
    Enna_Config_Data *cfgdata;
    char *value = NULL;

    player_type_t type = PLAYER_TYPE_MPLAYER;
    player_type_t dvd_type = PLAYER_TYPE_XINE;
    player_type_t tv_type = PLAYER_TYPE_XINE;
    player_vo_t vo = PLAYER_VO_AUTO;
    player_ao_t ao = PLAYER_AO_AUTO;
    player_verbosity_level_t verbosity = PLAYER_MSG_WARNING;

    int use_mplayer = 0;
    int use_xine = 0;

    if (!em)
        return;

    /* Load Config file values */
    cfgdata = enna_config_module_pair_get("libplayer");

    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "parameters:");

    if (cfgdata)
    {
        Eina_List *l;

        for (l = cfgdata->pair; l; l = l->next)
        {
            Config_Pair *pair = l->data;

            if (!strcmp("type", pair->key))
            {
                enna_config_value_store(&value, "type",
                                        ENNA_CONFIG_STRING, pair);
                enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, " * type: %s", value);

                if (!strcmp("gstreamer", value))
                    type = PLAYER_TYPE_GSTREAMER;
                else if (!strcmp("mplayer", value))
                    type = PLAYER_TYPE_MPLAYER;
                else if (!strcmp("vlc", value))
                    type = PLAYER_TYPE_VLC;
                else if (!strcmp("xine", value))
                    type = PLAYER_TYPE_XINE;
                else
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown type, 'mplayer' used instead");
            }
            else if (!strcmp("dvd_type", pair->key))
            {
                enna_config_value_store(&value, "dvd_type",
                                        ENNA_CONFIG_STRING, pair);
                enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, " * dvd_type: %s", value);

                if (!strcmp("gstreamer", value))
                    dvd_type = PLAYER_TYPE_GSTREAMER;
                else if (!strcmp("mplayer", value))
                    dvd_type = PLAYER_TYPE_MPLAYER;
                else if (!strcmp("vlc", value))
                    dvd_type = PLAYER_TYPE_VLC;
                else if (!strcmp("xine", value))
                    dvd_type = PLAYER_TYPE_XINE;
                else
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown dvd_type, 'xine' used instead");
            }
            else if (!strcmp("tv_type", pair->key))
            {
                enna_config_value_store(&value, "tv_type",
                                        ENNA_CONFIG_STRING, pair);
                enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, " * tv_type: %s", value);

                if (!strcmp("gstreamer", value))
                    tv_type = PLAYER_TYPE_GSTREAMER;
                else if (!strcmp("mplayer", value))
                    tv_type = PLAYER_TYPE_MPLAYER;
                else if (!strcmp("vlc", value))
                    tv_type = PLAYER_TYPE_VLC;
                else if (!strcmp("xine", value))
                    tv_type = PLAYER_TYPE_XINE;
                else
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown tv_type, 'xine' used instead");
            }
            else if (!strcmp("video_out", pair->key))
            {
                enna_config_value_store(&value, "video_out",
                                        ENNA_CONFIG_STRING, pair);
                enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                         " * video out: %s", value);

                if (!strcmp("auto", value))
                    vo = PLAYER_VO_AUTO;
                else if (!strcmp("x11", value))
                    vo = PLAYER_VO_X11;
                else if (!strcmp("xv", value))
                    vo = PLAYER_VO_XV;
                else if (!strcmp("gl", value))
                    vo = PLAYER_VO_GL;
                else if (!strcmp("fb", value))
                    vo = PLAYER_VO_FB;
                else
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown video_out, 'auto' used instead");
            }
            else if (!strcmp("audio_out", pair->key))
            {
                enna_config_value_store(&value, "audio_out",
                                        ENNA_CONFIG_STRING, pair);
                enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                         " * audio out: %s", value);

                if (!strcmp("auto", value))
                    ao = PLAYER_AO_AUTO;
                else if (!strcmp("alsa", value))
                    ao = PLAYER_AO_ALSA;
                else if (!strcmp("oss", value))
                    ao = PLAYER_AO_OSS;
                else
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown audio_out, 'auto' used instead");
            }
            else if (!strcmp("verbosity", pair->key))
            {
                enna_config_value_store(&value, "verbosity",
                                        ENNA_CONFIG_STRING, pair);
                enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                         " * verbosity level: %s", value);

                if (!strcmp("verbose", value))
                    verbosity = PLAYER_MSG_VERBOSE;
                else if (!strcmp("info", value))
                    verbosity = PLAYER_MSG_INFO;
                else if (!strcmp("warning", value))
                    verbosity = PLAYER_MSG_WARNING;
                else if (!strcmp("error", value))
                    verbosity = PLAYER_MSG_ERROR;
                else if (!strcmp("critical", value))
                    verbosity = PLAYER_MSG_CRITICAL;
                else if (!strcmp("none", value))
                    verbosity = PLAYER_MSG_NONE;
                else
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown verbosity, 'warning' used instead");
            }
        }
    }

    if (!value)
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 " * use all parameters by default");

    mod = calloc(1, sizeof(Enna_Module_libplayer));
    mod->em = em;
    mod->evas = em->evas;

    mod->key_down_event_handler =
        ecore_event_handler_add(ECORE_X_EVENT_KEY_DOWN, _x_event_key_down, NULL);
    mod->pipe = ecore_pipe_add(_pipe_read, NULL);

    if (type == PLAYER_TYPE_MPLAYER || dvd_type == PLAYER_TYPE_MPLAYER || tv_type == PLAYER_TYPE_MPLAYER) {
        use_mplayer = 1;
        mod->players[PLAYER_TYPE_MPLAYER] =
            player_init(PLAYER_TYPE_MPLAYER, ao, vo, verbosity, enna->ee_winid, _event_cb);
    }

    if (type == PLAYER_TYPE_XINE || dvd_type == PLAYER_TYPE_XINE || tv_type == PLAYER_TYPE_XINE) {
        use_xine = 1;
        mod->players[PLAYER_TYPE_XINE] =
            player_init(PLAYER_TYPE_XINE, ao, vo, verbosity, enna->ee_winid, _event_cb);
    }

    if ((use_mplayer && !mod->players[PLAYER_TYPE_MPLAYER]) || (use_xine && !mod->players[PLAYER_TYPE_XINE]))
    {
        enna_log(ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                "libplayer module initialization");
        return;
    }

    enna_mediaplayer_backend_register(&class);
    mod->uri = NULL;
    mod->label = NULL;
    mod->default_type = type;
    mod->dvd_type = dvd_type;
    mod->tv_type = tv_type;
    mod->player = mod->players[type];
    mod->player_type = type;
}

void module_shutdown(Enna_Module *em)
{
    _class_shutdown(0);
    free(mod);
}
