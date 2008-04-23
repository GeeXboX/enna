#ifndef _ENNA_MAINMENU_H_
#define _ENNA_MAINMENU_H_

#include "enna.h"

EAPI Evas_Object   *enna_mainmenu_add(Evas * evas);
EAPI void           enna_mainmenu_append(Evas_Object *obj, Evas_Object *icon, const char *label,
					 void (*func) (void *data),
					 void *data);
EAPI void           enna_mainmenu_load_from_activities(Evas_Object *obj);
EAPI void           enna_mainmenu_show(Evas_Object *obj);
EAPI void           enna_mainmenu_hide(Evas_Object *obj);
EAPI unsigned char  enna_mainmenu_visible(Evas_Object *obj);

#endif
