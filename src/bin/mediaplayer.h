#ifndef __ENNA_MEDIAPLAYER_H__
#define __ENNA_MEDIAPLAYER_H__

#include "enna.h"

typedef enum {
  ENNA_BACKEND_EMOTION,
  ENNA_BACKEND_LIBPLAYER,
} enna_mediaplayer_backend_t;

extern enna_mediaplayer_backend_t enna_backend;

typedef struct _Enna_Class_MediaplayerBackend Enna_Class_MediaplayerBackend;
typedef struct _Enna_Metadata Enna_Metadata;

struct _Enna_Metadata
{
   const char *title;
   const char *artist;
   const char *album;
   const char *year;
   const char *genre;
   const char *comment;
   const char *discid;
   const char *track;
};

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
      double (*class_position_get)(void);
      double (*class_length_get) (void);
      Enna_Metadata *(*class_metadata_get)(void);
   }func;
};

EAPI int            enna_mediaplayer_init(void);
EAPI void           enna_mediaplayer_shutdown(void);
EAPI void           enna_mediaplayer_uri_append(const char *uri);
EAPI int            enna_mediaplayer_select_nth(int n);
EAPI Enna_Metadata *enna_mediaplayer_metadata_get(void);
EAPI int            enna_mediaplayer_play(void);
EAPI int            enna_mediaplayer_stop(void);
EAPI int            enna_mediaplayer_pause(void);
EAPI int            enna_mediaplayer_next(void);
EAPI int            enna_mediaplayer_prev(void);
EAPI double         enna_mediaplayer_position_get(void);
EAPI double         enna_mediaplayer_length_get(void);
EAPI int            enna_mediaplayer_seek(double percent);
EAPI int            enna_mediaplayer_playlist_load(const char *filename);
EAPI int            enna_mediaplayer_playlist_save(const char *filename);
EAPI void           enna_mediaplayer_playlist_clear(void);
EAPI int            enna_mediaplayer_playlist_count(void);
EAPI int            enna_mediaplayer_backend_register(Enna_Class_MediaplayerBackend *class);
#endif
