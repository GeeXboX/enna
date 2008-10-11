#ifndef _ENNA_WALL_H
#define _ENNA_WALL_H

#include "enna.h"

Evas_Object *enna_wall_add(Evas * evas);
void enna_wall_picture_append(Evas_Object *obj,
    const char *filename,
    void (*func) (void *data, void *data2),
    void (*func_hilight) (void *data, void *data2),
    void *data,
    void *data2
    );
void *enna_wall_selected_data_get(Evas_Object *obj);
void *enna_wall_selected_data2_get(Evas_Object *obj);
void enna_wall_left_select(Evas_Object *obj);
void enna_wall_right_select(Evas_Object *obj);
void enna_wall_up_select(Evas_Object *obj);
void enna_wall_down_select(Evas_Object *obj);
#endif
