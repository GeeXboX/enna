/*
 *  Enna
 *
 *  Copyright (C) 2005-2009 The Enna Project (see AUTHORS)
 *
 *  License LGPL-2.1, see COPYING file at project folder.
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
   if (!vl) return NULL;
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
        vl->add(v, vl->data);
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
        vl->remove(v, vl->data);
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
    case VOLUME_TYPE_AUDIO_PLAYER:
    case VOLUME_TYPE_FLASHKEY:
    case VOLUME_TYPE_REMOVABLE_DISK:
        return strdup("icon/usb");
    case VOLUME_TYPE_NFS:
        return strdup("icon/dev/nfs");
    case VOLUME_TYPE_SMB:
        return strdup("icon/dev/sambe");
    default:
        return strdup("icon/enna");
    }
}
