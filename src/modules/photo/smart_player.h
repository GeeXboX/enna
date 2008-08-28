#ifndef __ENNA_SMART_PLAYER_H__
#define __ENNA_SMART_PLAYER_H__

#include "enna.h"

EAPI Evas_Object   *enna_smart_player_add(Evas * evas);
EAPI void           enna_smart_player_image_append(Evas_Object *obj, const char *filename);
EAPI int            enna_smart_player_next(Evas_Object * obj);
EAPI void           enna_smart_player_play(Evas_Object * obj);
#endif
