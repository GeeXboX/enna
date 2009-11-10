/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
    const char *device_name;
    const char *mount_point;
    const char *label;
    EnnaVolumeEject eject;
    Eina_Bool is_ejectable : 1;
};

Enna_Volume *enna_volume_new (void);
void enna_volume_free (Enna_Volume *v);

Enna_Volumes_Listener * enna_volumes_listener_add(const char *name, EnnaVolumesFunc add, EnnaVolumesFunc remove, void *data);
void enna_volumes_listener_del(Enna_Volumes_Listener *vl);
void enna_volumes_add_emit(Enna_Volume *v);
void enna_volumes_remove_emit(Enna_Volume *v);
char *enna_volumes_icon_from_type(Enna_Volume *v);
Eina_List* enna_volumes_get();

#endif /* ENNA_VOLUMES_H */
