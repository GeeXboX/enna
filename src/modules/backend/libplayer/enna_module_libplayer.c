/* Interface */

#include "enna.h"
#include <player.h>


static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

static void _class_init(int dummy);
static void _class_shutdown(int dummy);
static int _class_file_set(const char *uri);
static int _class_play(void);
static int _class_pause(void);
static int _class_stop(void);
static double _class_position_get();
static double _class_length_get();
static Enna_Metadata *_class_metadata_get(void);

static Enna_Class_MediaplayerBackend class =
{
  "libplayer",
  1,
  {
    _class_init,
    _class_shutdown,
    _class_file_set,
    _class_play,
    _class_pause,
    _class_stop,
    _class_position_get,
    _class_length_get,
    _class_metadata_get,
  }
};

typedef struct _Enna_Module_libplayer Enna_Module_libplayer;

struct _Enna_Module_libplayer
{
  Evas *evas;
  player_t *player;
  Enna_Module *em;
};

static Enna_Module_libplayer *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "libplayer"
};

static void _class_init(int dummy)
{
   printf("libplayer class init\n");
}

static void _class_shutdown(int dummy)
{
  player_playback_stop (mod->player);
  player_uninit (mod->player);
}

static int _class_file_set(const char *uri)
{
  player_mrl_append (mod->player, (char *) uri, NULL, PLAYER_ADD_MRL_QUEUE);
  return 0;
}

static int _class_play(void)
{
  player_playback_start (mod->player);
  return 0;
}

static int _class_stop(void)
{
  player_playback_stop (mod->player);
  return 0;
}

static int _class_pause(void)
{
  player_playback_pause (mod->player);
  return 0;
}

static double _class_position_get()
{
   /* FIXME: libplayer seems not to have position get API */
   return 0.0;
}

static double _class_length_get()
{
   /* FIXME: libplayer seems not to have length get API */
   return 0.0;
}

static Enna_Metadata *_class_metadata_get(void)
{
  mrl_t *mrl;
  Enna_Metadata *meta;

  mrl = player_get_mrl (mod->player);
  if (!mrl)
    return NULL;

  player_mrl_get_metadata (mod->player, mrl);
  if (!mrl->meta)
    return NULL;

  meta = calloc (1, sizeof (Enna_Metadata));
  meta->title = mrl->meta->title ? strdup (mrl->meta->title) : NULL;
  meta->artist = mrl->meta->artist ? strdup (mrl->meta->artist) : NULL;
  meta->album = mrl->meta->album ? strdup (mrl->meta->album) : NULL;
  meta->year = mrl->meta->year ? strdup (mrl->meta->year) : NULL;
  meta->genre = mrl->meta->genre ? strdup (mrl->meta->genre) : NULL;
  meta->comment = NULL;
  meta->discid = NULL;
  meta->track = mrl->meta->track ? strdup (mrl->meta->track) : NULL;

  return meta;
}

/* Module interface */

static int
event_cb (player_event_t e, void *data)
{
  if (e == PLAYER_EVENT_PLAYBACK_FINISHED)
    printf ("PLAYBACK FINISHED\n");

  return 0;
}

static int
em_init(Enna_Module *em)
{
   mod = calloc(1, sizeof(Enna_Module_libplayer));
   mod->em = em;
   mod->evas = em->evas;
   mod->player =
     player_init (PLAYER_TYPE_MPLAYER, PLAYER_AO_ALSA, PLAYER_VO_XV,
                  PLAYER_MSG_WARNING, event_cb);
   enna_mediaplayer_backend_register(&class);
   return 1;
}


static int
em_shutdown(Enna_Module *em)
{

   _class_shutdown(0);
   free(mod);
   return 1;
}

EAPI void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    if (!em_init(em))
        return;
}

EAPI void
module_shutdown(Enna_Module *em)
{
    em_shutdown(em);
}
