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

#include "enna.h"
#include "volumes.h"
#include "logs.h"

struct _Enna_Volumes_Listener {
    const char *name;
    EnnaVolumesFunc add;
    EnnaVolumesFunc remove;
    void *data;
};


/* Local subsystem vars */
static Eina_List *enna_volumes_listeners = NULL;  /** List of Enna_Volumes_Listener* registered   */
static Eina_List *_volumes = NULL; /** List of volumes currently detected */


/* Externally accessible functions */

Enna_Volumes_Listener *
enna_volumes_listener_add(const char *name, EnnaVolumesFunc add, EnnaVolumesFunc remove, void *data)
{
   Enna_Volumes_Listener *vl;

   vl = ENNA_NEW(Enna_Volumes_Listener, 1);
   vl->name = eina_stringshare_add(name);
   vl->add = add;
   vl->remove = remove;
   vl->data = data;

   enna_volumes_listeners = eina_list_prepend(enna_volumes_listeners, vl);
   return vl;
}

void
enna_volumes_listener_del(Enna_Volumes_Listener *vl)
{
   if (!vl) return;

   enna_volumes_listeners = eina_list_remove(enna_volumes_listeners, vl);
   ENNA_STRINGSHARE_DEL(vl->name);
   ENNA_FREE(vl);
}


void
enna_volumes_add_emit(Enna_Volume *v)
{
    Eina_List *l;
    Enna_Volumes_Listener *vl;

    if (!v)
        return;

    enna_log(ENNA_MSG_EVENT, "volumes", "Add: %s volume  listeners: %d",
             v->label, eina_list_count(enna_volumes_listeners));
    EINA_LIST_FOREACH(enna_volumes_listeners, l, vl)
    {
        if (!vl->add) continue;
        vl->add(vl->data, v);
    }
    _volumes = eina_list_append(_volumes, v);
}

void
enna_volumes_remove_emit(Enna_Volume *v)
{
    Eina_List *l;
    Enna_Volumes_Listener *vl;

    if (!v)
        return;

    enna_log(ENNA_MSG_EVENT, "volumes","Remove: %s volume  listeners: %d",
             v->label, eina_list_count(enna_volumes_listeners));
    EINA_LIST_FOREACH(enna_volumes_listeners, l, vl)
    {
        if (!vl->remove) continue;
        vl->remove(vl->data, v);
    }
    _volumes = eina_list_remove(_volumes, v);
}

Eina_List*
enna_volumes_get()
{
    return _volumes;
}

char *
enna_volumes_icon_from_type(Enna_Volume *v)
{
    if (!v)
        return NULL;

    switch(v->type)
    {
    case VOLUME_TYPE_CAMERA:
        return strdup("icon/dev/camera");
    case VOLUME_TYPE_AUDIO_PLAYER:
        return strdup("icon/dev/ipod");
    case VOLUME_TYPE_REMOVABLE_DISK:
        return strdup("icon/dev/usbstick");
    case VOLUME_TYPE_FLASHKEY:
    case VOLUME_TYPE_COMPACT_FLASH:
    case VOLUME_TYPE_MEMORY_STICK:
    case VOLUME_TYPE_SMART_MEDIA:
    case VOLUME_TYPE_SD_MMC:
        return strdup("icon/dev/memorycard");
    case VOLUME_TYPE_HDD:
        return strdup("icon/dev/hdd");
    case VOLUME_TYPE_CD:
    case VOLUME_TYPE_VCD:
    case VOLUME_TYPE_SVCD:
        return strdup("icon/dev/cdrom");
    case VOLUME_TYPE_CDDA:
        /* FIXME why cdda2 ? */
        return strdup("icon/dev/cdda2");
    case VOLUME_TYPE_DVD:
    case VOLUME_TYPE_DVD_VIDEO:
        return strdup("icon/dev/dvd");
    case VOLUME_TYPE_NFS:
        return strdup("icon/dev/nfs");
    case VOLUME_TYPE_SMB:
        return strdup("icon/dev/sambe");

    default:
        return strdup("icon/enna");
    }
}

Enna_Volume *
enna_volume_new (void)
{
  Enna_Volume *v;

  v = ENNA_NEW (Enna_Volume, 1);
  return v;
}

void
enna_volume_free (Enna_Volume *v)
{
  if (!v)
    return;

  eina_stringshare_del (v->device_name);
  eina_stringshare_del (v->mount_point);
  eina_stringshare_del (v->label);
  ENNA_FREE (v);
}
