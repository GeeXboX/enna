#ifndef ENNA_LIST_H
#define ENNA_LIST_H

#include "enna.h"

Evas_Object *enna_list_add (Evas *evas);
void enna_list_append(Evas_Object *obj, Elm_Genlist_Item_Class *class, void * class_data, void (*func) (void *data), void *data);
void enna_list_selected_set(Evas_Object *obj, int n);
int enna_list_selected_get(Evas_Object *obj);
void * enna_list_selected_data_get(Evas_Object *obj);
int enna_list_jump_label(Evas_Object *obj, const char *label);
void enna_list_jump_nth(Evas_Object *obj, int n);
void enna_list_event_key_down(Evas_Object *obj, void *event_info);
#endif

