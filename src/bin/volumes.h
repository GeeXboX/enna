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
typedef enum _ENNA_VOLUME_TYPE ENNA_VOLUME_TYPE;

typedef void (*EnnaVolumesFunc)(void *data, Enna_Volume *volume);
typedef void (*EnnaVolumeEject)(void *data);


enum _ENNA_VOLUME_TYPE
{
    VOLUME_TYPE_CAMERA,
    VOLUME_TYPE_AUDIO_PLAYER,
    VOLUME_TYPE_FLASHKEY,
    VOLUME_TYPE_REMOVABLE_DISK,
    VOLUME_TYPE_COMPACT_FLASH,
    VOLUME_TYPE_MEMORY_STICK,
    VOLUME_TYPE_SMART_MEDIA,
    VOLUME_TYPE_SD_MMC,
    VOLUME_TYPE_HDD,
    VOLUME_TYPE_CD,
    VOLUME_TYPE_CDDA,
    VOLUME_TYPE_DVD,
    VOLUME_TYPE_DVD_VIDEO,
    VOLUME_TYPE_VCD,
    VOLUME_TYPE_SVCD,
    VOLUME_TYPE_NFS,
    VOLUME_TYPE_SMB,
};

struct _Enna_Volume
{
    ENNA_VOLUME_TYPE type;
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
