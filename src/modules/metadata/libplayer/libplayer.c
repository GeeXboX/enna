/* Interface */

#include "enna.h"
#include "codecs.h"
#include <player.h>

#define ENNA_MODULE_NAME        "metadata_libplayer"
#define ENNA_GRABBER_NAME       "libplayer"
#define ENNA_GRABBER_PRIORITY   2

#define URI_TYPE_FTP  "ftp://"
#define URI_TYPE_HTTP "http://"
#define URI_TYPE_MMS  "mms://"
#define URI_TYPE_RTP  "rtp://"
#define URI_TYPE_RTSP "rtsp://"
#define URI_TYPE_SMB  "smb://"
#define URI_TYPE_TCP  "tcp://"
#define URI_TYPE_UDP  "udp://"
#define URI_TYPE_UNSV "unsv://"

typedef struct _Metadata_Module_libplayer
{
    Evas *evas;
    Enna_Module *em;
    player_t *player;
    

} Metadata_Module_libplayer;

static Metadata_Module_libplayer *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static mrl_t *
set_network_stream (const char *uri, mrl_resource_t type)
{
    mrl_t *mrl;
    mrl_resource_network_args_t *args;

    args = calloc (1, sizeof (mrl_resource_network_args_t));
    args->url = strdup (uri);
    mrl = mrl_new (mod->player, type, args);

    return mrl;
}

static mrl_t *
set_local_stream (const char *uri)
{
    mrl_t *mrl;
    mrl_resource_local_args_t *args;

    args = calloc (1, sizeof (mrl_resource_local_args_t));
    args->location = strdup (uri);
    mrl = mrl_new (mod->player, MRL_RESOURCE_FILE, args);

    return mrl;
}

static int
set_mrl (const char *uri)
{
    mrl_t *mrl = NULL;

    /* try network streams */
    if (!strncmp (uri, URI_TYPE_FTP, strlen(URI_TYPE_FTP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_FTP);
    else if (!strncmp (uri, URI_TYPE_HTTP, strlen(URI_TYPE_HTTP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_HTTP);
    else if (!strncmp (uri, URI_TYPE_MMS, strlen(URI_TYPE_MMS)))
        mrl = set_network_stream (uri, MRL_RESOURCE_MMS);
    else if (!strncmp (uri, URI_TYPE_RTP, strlen(URI_TYPE_RTP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_RTP);
    else if (!strncmp (uri, URI_TYPE_RTSP, strlen(URI_TYPE_RTSP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_RTSP);
    else if (!strncmp (uri, URI_TYPE_SMB, strlen(URI_TYPE_SMB)))
        mrl = set_network_stream (uri, MRL_RESOURCE_SMB);
    else if (!strncmp (uri, URI_TYPE_TCP, strlen(URI_TYPE_TCP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_TCP);
    else if (!strncmp (uri, URI_TYPE_UDP, strlen(URI_TYPE_UDP)))
        mrl = set_network_stream (uri, MRL_RESOURCE_UDP);
    else if (!strncmp (uri, URI_TYPE_UNSV, strlen(URI_TYPE_UNSV)))
        mrl = set_network_stream (uri, MRL_RESOURCE_UNSV);

    /* default is local files */
    if (!mrl)
        mrl = set_local_stream (uri);

    if (!mrl)
        return 1;

    player_mrl_set (mod->player, mrl);
    return 0;
}

static void
libplayer_grab (Enna_Metadata *meta, int caps)
{
    char *track_nb;
    int frameduration = 0;
    char *codec_id;

    if (!meta || !meta->uri)
        return;

    set_mrl (meta->uri);

    if (!meta->size)
        meta->size = mrl_get_size (mod->player, NULL);

    if (!meta->length)
        meta->length =
            mrl_get_property (mod->player, NULL, MRL_PROPERTY_LENGTH);

    if (!meta->title)
        meta->title = mrl_get_metadata (mod->player, NULL, MRL_METADATA_TITLE);

    if (caps & ENNA_GRABBER_CAP_AUDIO)
    {
        if (!meta->music->artist)
            meta->music->artist = mrl_get_metadata (mod->player, NULL,
                                                    MRL_METADATA_ARTIST);
        if (!meta->music->album)
            meta->music->album = mrl_get_metadata (mod->player, NULL,
                                                   MRL_METADATA_ALBUM);
        
        if (!meta->music->year)
            meta->music->year = mrl_get_metadata (mod->player, NULL,
                                                  MRL_METADATA_YEAR);
        if (!meta->music->genre)
            meta->music->genre = mrl_get_metadata (mod->player, NULL,
                                                   MRL_METADATA_GENRE);
        if (!meta->music->comment)
            meta->music->comment = mrl_get_metadata (mod->player, NULL,
                                                     MRL_METADATA_COMMENT);

        if (!meta->music->track)
        {
            track_nb =
                mrl_get_metadata (mod->player, NULL, MRL_METADATA_TRACK);
            if (track_nb)
            {
                meta->music->track = atoi (track_nb);
                free (track_nb);
            }
        }

        if (!meta->music->codec)
        {
            codec_id = mrl_get_audio_codec (mod->player, NULL);
            meta->music->codec = get_codec_name (codec_id);
            free (codec_id);
        }

        if (!meta->music->bitrate)
            meta->music->bitrate =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_AUDIO_BITRATE);

        if (!meta->music->channels)
            meta->music->channels =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_AUDIO_CHANNELS);

        if (!meta->music->samplerate)
            meta->music->samplerate =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_AUDIO_SAMPLERATE);
    }

    if (caps & ENNA_GRABBER_CAP_VIDEO)
    {
        if (!meta->video->codec)
        {
            codec_id = mrl_get_video_codec (mod->player, NULL);
            meta->video->codec = get_codec_name (codec_id);
            free (codec_id);
        }

        if (!meta->video->width)
            meta->video->width =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_VIDEO_WIDTH);

        if (!meta->video->height)
            meta->video->height =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_VIDEO_HEIGHT);

        if (!meta->video->channels)
            meta->video->channels =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_VIDEO_CHANNELS);

        if (!meta->video->streams)
            meta->video->streams =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_VIDEO_STREAMS);

        if (!meta->video->framerate)
        {
            frameduration =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_VIDEO_FRAMEDURATION);
            if (frameduration)
                meta->video->framerate =
                    PLAYER_VIDEO_FRAMEDURATION_RATIO_DIV / frameduration;
        }

        if (!meta->video->bitrate)
            meta->video->bitrate =
                mrl_get_property (mod->player, NULL,
                                  MRL_PROPERTY_VIDEO_BITRATE);
    }
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    ENNA_GRABBER_CAP_AUDIO | ENNA_GRABBER_CAP_VIDEO,
    libplayer_grab,
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_METADATA,
    ENNA_MODULE_NAME
};

void module_init(Enna_Module *em)
{
    Enna_Config_Data *cfgdata;
    player_type_t type = PLAYER_TYPE_MPLAYER;
    player_verbosity_level_t verbosity = PLAYER_MSG_WARNING;
    char *value = NULL;
    
    if (!em)
        return;

    cfgdata = enna_config_module_pair_get("libplayer");
    if (cfgdata)
    {
        Eina_List *l;

        for (l = cfgdata->pair; l; l = l->next)
        {
            Config_Pair *pair = l->data;

            if (!strcmp ("type", pair->key))
            {
                enna_config_value_store (&value, "type",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
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
                    enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                              "   - unknown type, 'mplayer' used instead");
            }
            else if (!strcmp ("verbosity", pair->key))
            {
                enna_config_value_store (&value, "verbosity",
                                         ENNA_CONFIG_STRING, pair);
                enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
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
                    enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                             "   - unknown verbosity, 'warning' used instead");
            }
        }
    }
    
    mod = calloc (1, sizeof (Metadata_Module_libplayer));

    mod->em = em;
    mod->evas = em->evas;

    mod->player = player_init (type, PLAYER_AO_NULL, PLAYER_VO_NULL,
                               verbosity, 0, NULL);
    if (!mod->player)
    {
        enna_log (ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                  "libplayer metadata module initialization");
        return;
    }

    enna_metadata_add_grabber (&grabber);
}

void module_shutdown(Enna_Module *em)
{
    //enna_metadata_remove_grabber (ENNA_GRABBER_NAME);
    player_playback_stop (mod->player);
    player_uninit (mod->player);
    free(mod);
}
