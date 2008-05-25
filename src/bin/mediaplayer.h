#ifndef __ENNA_MEDIAPLAYER_H__
#define __ENNA_MEDIAPLAYER_H__

#include "enna.h"

typedef struct _Enna_Class_MediaplayerBackend Enna_Class_MediaplayerBackend;

struct _Enna_Class_MediaplayerBackend
{
   const char *name;
   int pri;
   struct
   {
      void (*class_init)(int dummy);
      void (*class_shutdown)(int dummy);
      int  (*class_file_set)(const char *uri);
      int  (*class_play)(void);
      int  (*class_stop)(void);
      int  (*class_pause)(void);

   }func;
};

EAPI int          enna_mediaplayer_init(void);
EAPI void         enna_mediaplayer_shutdown(void);
EAPI void         enna_mediaplayer_uri_append(const char *uri);
EAPI int          enna_mediaplayer_select_nth(int n);
EAPI int          enna_mediaplayer_play(void);
EAPI int          enna_mediaplayer_stop(void);
EAPI int          enna_mediaplayer_pause(void);
EAPI int          enna_mediaplayer_next(void);
EAPI int          enna_mediaplayer_prev(void);
EAPI int          enna_mediaplayer_seek(double percent);
EAPI int          enna_mediaplayer_playlist_load(const char *filename);
EAPI int          enna_mediaplayer_playlist_save(const char *filename);
EAPI void         enna_mediaplayer_playlist_clear(void);
EAPI int          enna_mediaplayer_playlist_count(void);
EAPI int          enna_mediaplayer_backend_register(Enna_Class_MediaplayerBackend *class);
#endif
