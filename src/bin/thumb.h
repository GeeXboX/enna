#ifndef ENNA_THUMB_H
#define ENNA_THUMB_H

#include <Evas.h>
#include <Ecore_Ipc.h>

int                   enna_thumb_init(void);
int                   enna_thumb_shutdown(void);

Evas_Object          *enna_thumb_icon_add(Evas *evas);
void                  enna_thumb_icon_file_set(Evas_Object *obj, const char *file, const char *key);
void                  enna_thumb_icon_size_set(Evas_Object *obj, int w, int h);
void                  enna_thumb_icon_begin(Evas_Object *obj);
void                  enna_thumb_icon_end(Evas_Object *obj);
void                  enna_thumb_icon_rethumb(Evas_Object *obj);

void                  enna_thumb_client_data(Ecore_Ipc_Event_Client_Data *e);
void                  enna_thumb_client_del(Ecore_Ipc_Event_Client_Del *e);

#endif

