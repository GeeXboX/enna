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

static const struct {
  ENNA_VOLUME_TYPE type;
  const char *icon;
} volumes_type_icon_map[] = {
  { VOLUME_TYPE_CAMERA,            "icon/dev/camera"        },
  { VOLUME_TYPE_AUDIO_PLAYER,      "icon/dev/ipod"          },
  { VOLUME_TYPE_FLASHKEY,          "icon/dev/memorycard"    },
  { VOLUME_TYPE_REMOVABLE_DISK,    "icon/dev/usbstick"      },
  { VOLUME_TYPE_COMPACT_FLASH,     "icon/dev/memorycard"    },
  { VOLUME_TYPE_MEMORY_STICK,      "icon/dev/memorycard"    },
  { VOLUME_TYPE_SMART_MEDIA,       "icon/dev/memorycard"    },
  { VOLUME_TYPE_SD_MMC,            "icon/dev/memorycard"    },
  { VOLUME_TYPE_HDD,               "icon/dev/hdd"           },
  { VOLUME_TYPE_CD,                "icon/dev/cdrom"         },
  { VOLUME_TYPE_CDDA,              "icon/dev/cdda2"         },
  { VOLUME_TYPE_DVD,               "icon/dev/dvd"           },
  { VOLUME_TYPE_DVD_VIDEO,         "icon/dev/dvd"           },
  { VOLUME_TYPE_VCD,               "icon/dev/cdrom"         },
  { VOLUME_TYPE_SVCD,              "icon/dev/cdrom"         },
  { VOLUME_TYPE_NFS,               "icon/dev/nfs"           },
  { VOLUME_TYPE_SMB,               "icon/dev/samba"         },
  { VOLUME_TYPE_UNKNOWN,           NULL                     }
};

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
    int i;

    if (!v)
        return NULL;

    for (i = 0; volumes_type_icon_map[i].icon; i++)
        if (v->type == volumes_type_icon_map[i].type)
            return strdup (volumes_type_icon_map[i].icon);

    return strdup ("icon/enna");
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
