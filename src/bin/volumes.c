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
enna_volumes_listener_promote(Enna_Volumes_Listener *vl)
{
   Eina_List *l;

   l = eina_list_data_find_list(enna_volumes_listeners, vl);
   if (l) enna_volumes_listeners = eina_list_promote_list(enna_volumes_listeners, l);
}

void
enna_volumes_listener_demote(Enna_Volumes_Listener *vl)
{
   Eina_List *l;

   l = eina_list_data_find_list(enna_volumes_listeners, vl);
   if (l) enna_volumes_listeners  = eina_list_demote_list(enna_volumes_listeners, l);
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

    printf("Add: %s volume  listeners: %d", v->label, eina_list_count(enna_volumes_listeners));
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

    printf("Remove: %s volume  listeners: %d", v->label, eina_list_count(enna_volumes_listeners));
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
