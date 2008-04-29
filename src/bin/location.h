#ifndef __ENNA_LOCATION_H__
#define __ENNA_LOCATION_H__

#include "enna.h"

EAPI Evas_Object   *enna_location_add(Evas * evas);
EAPI void           enna_location_append  (Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data, void *data2), void *data, void *data2);

#endif
