/*
 * mediaplayer.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * mediaplayer.c is free software copyrighted by Nicolas Aguirre.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Nicolas Aguirre'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * mediaplayer.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Nicolas Aguirre OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "enna.h"

#ifdef BUILD_EMOTION_BACKEND
enna_mediaplayer_backend_t enna_backend = ENNA_BACKEND_EMOTION;
#else
enna_mediaplayer_backend_t enna_backend = ENNA_BACKEND_LIBPLAYER;
#endif

static Evas_List *_playlist;



typedef struct _Enna_Mediaplayer Enna_Mediaplayer;
typedef enum _PLAY_STATE PLAY_STATE;

enum _PLAY_STATE  {
    PLAYING,
    PAUSE,
    STOPPED
  };

struct _Enna_Mediaplayer
{
   PLAY_STATE  play_state;
   int selected;
   Enna_Class_MediaplayerBackend *class;
};

static Enna_Mediaplayer *_mediaplayer;

static void _event_cb(void *data, enna_mediaplayer_event_t event);

/* externally accessible functions */
EAPI int
enna_mediaplayer_init(void)
{
   Enna_Module *em;
   char *backend_name;

   /* FIXME enna_backend should be a config parameter */
   switch (enna_backend)
   {
#ifdef BUILD_EMOTION_BACKEND
   case ENNA_BACKEND_EMOTION:
   {
     printf("Using Emotion Backend\n");
     backend_name = "emotion";
     break;
   }
#endif
#ifdef BUILD_LIPLAYER_BACKEND
   case ENNA_BACKEND_LIBPLAYER:
   {
     printf("Using libplayer Backend\n");
     backend_name = "libplayer";
     break;
   }
#endif
   default:
     return -1;
   }

   _playlist = NULL;
   _mediaplayer = calloc(1, sizeof(Enna_Mediaplayer));
   em = enna_module_open(backend_name, enna->evas);
   enna_module_enable(em);
   _mediaplayer->play_state = STOPPED;
   _mediaplayer->selected = 0;

   /* Create Ecore Event ID */
   ENNA_EVENT_MEDIAPLAYER_EOS = ecore_event_type_new();
   ENNA_EVENT_MEDIAPLAYER_METADATA_UPDATE = ecore_event_type_new();

   return 0;
}
EAPI void
enna_mediaplayer_shutdown()
{
   free(_mediaplayer);
   evas_list_free(_playlist);
}


EAPI void
enna_mediaplayer_uri_append(const char *uri)
{
   _playlist = evas_list_append(_playlist, uri);
}


EAPI int
enna_mediaplayer_play(void)
{

   switch(_mediaplayer->play_state)
     {
      case STOPPED:
	{
	   const char *uri;
	   uri = evas_list_nth(_playlist, _mediaplayer->selected);
	   _mediaplayer->class->func.class_stop();
	   _mediaplayer->class->func.class_file_set(uri);
	   printf("Play\n");
	   _mediaplayer->class->func.class_play();
	   _mediaplayer->play_state = PLAYING;
	}
	break;
      case PLAYING:
	 enna_mediaplayer_pause();
	 break;
      case PAUSE:
	 _mediaplayer->class->func.class_play();
	 _mediaplayer->play_state = PLAYING;
	 break;
      default:
	 break;

     }


   return 0;
}

EAPI int
enna_mediaplayer_select_nth(int n)
{
   const char *uri;
   if (n < 0 || n > evas_list_count(_playlist) - 1) return -1;
   uri = evas_list_nth(_playlist, n);
   printf("select %d\n", n);
   if (uri)
     _mediaplayer->class->func.class_file_set(uri);
   _mediaplayer->selected = n;
   return 0;
}

EAPI int
enna_mediaplayer_stop(void)
{
   if (_mediaplayer->class)
     {
	_mediaplayer->class->func.class_stop();
	_mediaplayer->play_state = STOPPED;
     }
   return 0;
}

EAPI int
enna_mediaplayer_pause(void)
{
   if (_mediaplayer->class)
     {
	_mediaplayer->class->func.class_pause();
	_mediaplayer->play_state = PAUSE;
     }
   return 0;
}

EAPI int
enna_mediaplayer_next(void)
{
   const char *uri;

   _mediaplayer->selected++;
   if( _mediaplayer->selected > evas_list_count(_playlist) - 1)
     {
	_mediaplayer->selected--;
	return -1;
     }
   uri = evas_list_nth(_playlist, _mediaplayer->selected);
   printf("select %d\n", _mediaplayer->selected);
   if (uri)
     {
	enna_mediaplayer_stop();
	_mediaplayer->class->func.class_file_set(uri);
	enna_mediaplayer_play();
     }

   return 0;
}

EAPI int
enna_mediaplayer_prev(void)
{
   const char *uri;

   _mediaplayer->selected--;
   if (_mediaplayer->selected < 0)
     {
	_mediaplayer->selected = 0;
	return -1;
     }
   uri = evas_list_nth(_playlist, _mediaplayer->selected);
   printf("select %d\n", _mediaplayer->selected);
   if (uri)
     {
	enna_mediaplayer_stop();
	_mediaplayer->class->func.class_file_set(uri);
	enna_mediaplayer_play();
     }
   return 0;
}

EAPI double
enna_mediaplayer_position_get(void)
{
   if (_mediaplayer->play_state == PAUSE || _mediaplayer->play_state == PLAYING)
     {
	return _mediaplayer->class->func.class_position_get();
     }
   return 0.0;
}

EAPI double
enna_mediaplayer_length_get(void)
{
   if (_mediaplayer->play_state == PAUSE || _mediaplayer->play_state == PLAYING)
     {
	return _mediaplayer->class->func.class_length_get();
     }
   return 0.0;
}

EAPI int
enna_mediaplayer_seek(double percent)
{
   printf ("Seeking to: %d\%\n", (int) (100 * percent));
   return 0;
}

EAPI int
enna_mediaplayer_playlist_load(const char *filename)
{
   return 0;
}

EAPI int
enna_mediaplayer_playlist_save(const char *filename)
{
   return 0;
}

EAPI void
enna_mediaplayer_playlist_clear(void)
{
   evas_list_free(_playlist);
   _playlist = NULL;
   if (_mediaplayer->class)
     {
	_mediaplayer->class->func.class_stop();
	_mediaplayer->selected = 0;
	_mediaplayer->play_state = STOPPED;
     }

}

EAPI Enna_Metadata *
enna_mediaplayer_metadata_get(void)
{
   if (_mediaplayer->class)
     return _mediaplayer->class->func.class_metadata_get();
   else
     return NULL;
}

EAPI int
enna_mediaplayer_playlist_count(void)
{
   return evas_list_count(_playlist);
}

EAPI int
enna_mediaplayer_backend_register(Enna_Class_MediaplayerBackend *class)
{
   if (!class) return -1;
   _mediaplayer->class = class;
   class->func.class_init(0);

   if (class->func.class_event_cb_set)
     class->func.class_event_cb_set(_event_cb, NULL);

   return 0;
}

static void
_event_cb(void *data, enna_mediaplayer_event_t event)
{
   switch(event)
     {
      case ENNA_MP_EVENT_EOF:
	 printf("End of stream\n");
	 ecore_event_add(ENNA_EVENT_MEDIAPLAYER_EOS, NULL, NULL, NULL);
	 break;
      default:
	 break;
     }
}
