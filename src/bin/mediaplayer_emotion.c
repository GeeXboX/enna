#include <Emotion.h>
#include "utils.h"
#include "logs.h"
#include "mediaplayer.h"

#define SEEK_STEP_DEFAULT         10 /* seconds */
#define VOLUME_STEP_DEFAULT       5 /* percent */

typedef struct _Enna_Mediaplayer Enna_Mediaplayer;

struct _Enna_Mediaplayer
{
    PLAY_STATE play_state;
    Evas_Object *player;
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
    Enna_Playlist *cur_playlist;
};

static Enna_Mediaplayer *mp = NULL;



/* externally accessible functions */
int
enna_mediaplayer_supported_uri_type(enna_mediaplayer_uri_type_t type __UNUSED__)
{
    return 1;
}


void
enna_mediaplayer_cfg_register (void)
{

}

int
enna_mediaplayer_init(void)
{
    mp = calloc(1, sizeof(Enna_Mediaplayer));

    mp->uri = NULL;
    mp->label = NULL;

    mp->player = emotion_object_add(evas_object_evas_get(enna->layout));
    emotion_object_init(mp->player, "xine");
    evas_object_layer_set(mp->player, -1);
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
}

void
enna_mediaplayer_shutdown(void)
{
    if (!mp)
        return;

    ENNA_FREE(mp->uri);
    ENNA_FREE(mp->label);
    emotion_object_play_set(mp->player, EINA_FALSE);
    if (mp->player)
        evas_object_del(mp->player);
    ENNA_FREE(mp);
}

Evas_Object *enna_mediaplayer_obj_get(void)
{
    return mp->player;
}

Enna_File *
enna_mediaplayer_current_file_get(void)
{
    Enna_File *item;

    if (!mp->cur_playlist || mp->play_state != PLAYING)
        return NULL;

    item = eina_list_nth(mp->cur_playlist->playlist, mp->cur_playlist->selected);
    if (!item)
        return NULL;

    return item;
}

const char *
enna_mediaplayer_get_current_uri(void)
{
  Enna_File *item;

  if (!mp->cur_playlist || mp->play_state != PLAYING)
      return NULL;

  item = eina_list_nth(mp->cur_playlist->playlist, mp->cur_playlist->selected);
  if (!item->uri)
    return NULL;

  return strdup(item->uri);
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
        emotion_object_play_set(mp->player, EINA_FALSE);
        if (item && item->uri)
            emotion_object_file_set(mp->player, item->mrl);
        emotion_object_play_set(mp->player, EINA_TRUE);
        if (item && item->type == ENNA_FILE_FILM)
        {
            evas_object_show(mp->player);
            evas_object_hide(enna->layout);
        }
        mp->play_state = PLAYING;
        ecore_event_add(ENNA_EVENT_MEDIAPLAYER_START, NULL, NULL, NULL);
        break;
    }
    case PLAYING:
        enna_mediaplayer_pause();
        break;
    case PAUSE:
        emotion_object_play_set(mp->player, EINA_TRUE);
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
    emotion_object_play_set(mp->player, EINA_FALSE);
    emotion_object_position_set(mp->player, 0);
    mp->play_state = STOPPED;
    evas_object_hide(mp->player);
    evas_object_show(enna->layout);
    ecore_event_add(ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);

    return 0;
}

int
enna_mediaplayer_pause(void)
{
    emotion_object_play_set(mp->player, EINA_FALSE);
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
    printf("TODO : position get\n");
/*
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
        mp_position_get(): 0.0;
*/
    return 0.0;
}

int
enna_mediaplayer_position_percent_get(void)
{
    /*
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
            mp_position_percent_get() : 0;
    */
    printf("TODO : position_percent_get\n");
    return 0;
}

double
enna_mediaplayer_length_get(void)
{
    return (mp->play_state == PAUSE || mp->play_state == PLAYING) ?
        emotion_object_play_length_get(mp->player) : 0.0;
}

static void
enna_mediaplayer_seek(int value __UNUSED__, SEEK_TYPE type __UNUSED__)
{
    printf("TODO : seek\n");
    /* TODO : seek ! */
    /*
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
        player_playback_seek(mp->player, value, pl_seek[type]);
        }*/
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
    printf("Resize %d %d %d %d\n", x, y, w, h);
    evas_object_resize(mp->player, w, h);
    evas_object_move(mp->player, x, y);
}

int
enna_mediaplayer_playlist_load(const char *filename __UNUSED__)
{
    return 0;
}

int
enna_mediaplayer_playlist_save(const char *filename __UNUSED__)
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

    if (item->uri)
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
    emotion_object_play_set(mp->player, EINA_FALSE);
    emotion_object_position_set(mp->player, 0);
    mp->play_state = STOPPED;
    ecore_event_add(ENNA_EVENT_MEDIAPLAYER_STOP, NULL, NULL, NULL);
}

void
enna_mediaplayer_send_input(enna_input event __UNUSED__)
{
}

int
enna_mediaplayer_volume_get(void)
{
    return (mp && mp->player) ? emotion_object_audio_volume_get(mp->player) * 100 : 0;
}

void
enna_mediaplayer_volume_set(int value)
{
    emotion_object_audio_volume_set(mp->player, value / 100.0);
}

void
enna_mediaplayer_default_increase_volume(void)
{
    int vol = enna_mediaplayer_volume_get();
    vol = MMIN(vol + VOLUME_STEP_DEFAULT, 100);
    emotion_object_audio_volume_set(mp->player, vol);
}

void
enna_mediaplayer_default_decrease_volume(void)
{
    int vol = enna_mediaplayer_volume_get();
    vol = MMAX(vol - VOLUME_STEP_DEFAULT, 0);
    emotion_object_audio_volume_set(mp->player, vol);
}

void
enna_mediaplayer_mute(void)
{
    Eina_Bool m;

    m = emotion_object_audio_mute_get(mp->player);
    emotion_object_audio_mute_set(mp->player, m ?
                          EINA_FALSE : EINA_TRUE);
}

int
enna_mediaplayer_mute_get(void)
{
    Eina_Bool m;

    if (!mp)
      return 0;

    m = emotion_object_audio_mute_get(mp->player);
    
    return m;
}

void
enna_mediaplayer_audio_previous(void)
{

}

void
enna_mediaplayer_audio_next(void)
{

}

void
enna_mediaplayer_audio_increase_delay(void)
{

}

void
enna_mediaplayer_audio_decrease_delay(void)
{

}

void
enna_mediaplayer_subtitle_set_visibility(void)
{

}

void
enna_mediaplayer_subtitle_previous(void)
{

}

void
enna_mediaplayer_subtitle_next(void)
{

}

void
enna_mediaplayer_subtitle_set_alignment(void)
{
    
}

void
enna_mediaplayer_subtitle_increase_position(void)
{
    
}

void
enna_mediaplayer_subtitle_decrease_position(void)
{
    
}

void
enna_mediaplayer_subtitle_increase_scale(void)
{
    
}

void
enna_mediaplayer_subtitle_decrease_scale(void)
{
    
}

void
enna_mediaplayer_subtitle_increase_delay(void)
{
    /* Nothing to do for emotion */
}

void
enna_mediaplayer_subtitle_decrease_delay(void)
{
    /* Nothing to do for emotion */
}

void
enna_mediaplayer_set_framedrop(void)
{
    /* Nothing to do for emotion */
}
