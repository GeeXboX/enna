/*
 *  Enna
 *
 *  Copyright (C) 2005-2009 The Enna Project (see AUTHORS)
 *
 *  License LGPL-2.1, see COPYING file at project folder.
 *
 */
#ifndef ENNA_VOLUMES_H
#define ENNA_VOLUMES_H

#include "enna.h"

typedef struct _Enna_Volumes_Listener Enna_Volumes_Listener;
typedef struct _Enna_Volume Enna_Volume;
typedef enum _VOLUME_TYPE VOLUME_TYPE;

typedef void (*EnnaVolumesFunc)(void *data, Enna_Volume *volume);
typedef void (*EnnaVolumeEject)(void *data);


enum _VOLUME_TYPE
{
    MOUNT_POINT_TYPE_CAMERA,
    MOUNT_POINT_TYPE_AUDIO_PLAYER,
    MOUNT_POINT_TYPE_FLASHKEY,
    MOUNT_POINT_TYPE_REMOVABLE_DISK
};

struct _Enna_Volume
{
    VOLUME_TYPE type;
    const char *mount_point;
    const char *label;
    EnnaVolumeEject eject;
    Eina_Bool is_ejectable : 1;
};

Enna_Volumes_Listener * enna_volumes_listener_add(const char *name, EnnaVolumesFunc add, EnnaVolumesFunc remove, void *data);
void enna_volumes_listener_del(Enna_Volumes_Listener *vl);
void enna_volumes_add_emit(Enna_Volume *v);
void enna_volumes_remove_emit(Enna_Volume *v);
char *enna_volumes_icon_from_type(Enna_Volume *v);
Eina_List* enna_volumes_get();

#endif /* ENNA_VOLUMES_H */
