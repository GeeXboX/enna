/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2010 The Enna Project
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <string.h>
#include <ctype.h>

#include <Ecore.h>

#include "enna.h"
#include "input.h"
#include "logs.h"


struct _Input_Listener {
    const char *name;
    Eina_Bool (*func)(void *data, enna_input event);
    void *data;
};


/* Local Globals */
static Eina_List *_listeners = NULL;

/* Public Functions */
Eina_Bool
enna_input_event_emit(enna_input in)
{
    Input_Listener *il;
    Eina_List *l;
    Eina_Bool ret;

    enna_log(ENNA_MSG_EVENT, NULL, "Input emit: %d (listeners: %d)", in, eina_list_count(_listeners));

    enna_idle_timer_renew();
    EINA_LIST_FOREACH(_listeners, l, il)
    {
        enna_log(ENNA_MSG_EVENT, NULL, "  emit to: %s", il->name);
        if (!il->func) continue;

        ret = il->func(il->data, in);
        if (ret == ENNA_EVENT_BLOCK)
        {
            enna_log(ENNA_MSG_EVENT, NULL, "  emission stopped by: %s", il->name);
            return EINA_TRUE;
        }
    }

    return EINA_TRUE;
}

Input_Listener *
enna_input_listener_add(const char *name,
                        Eina_Bool(*func)(void *data, enna_input event),
                        void *data)
{
    Input_Listener *il;

    enna_log(ENNA_MSG_INFO, NULL, "listener add: %s", name);
    il = ENNA_NEW(Input_Listener, 1);
    if (!il) return NULL;
    il->name = eina_stringshare_add(name);
    il->func = func;
    il->data = data;

    _listeners = eina_list_prepend(_listeners, il);
    return il;
}

void
enna_input_listener_promote(Input_Listener *il)
{
    Eina_List *l;

    l = eina_list_data_find_list(_listeners, il);
    if (!l) return;

    _listeners  = eina_list_promote_list(_listeners, l);
}

void
enna_input_listener_demote(Input_Listener *il)
{
    Eina_List *l;

    l = eina_list_data_find_list(_listeners, il);
    if (!l) return;

    _listeners  = eina_list_demote_list(_listeners, l);
}

void
enna_input_listener_del(Input_Listener *il)
{
    if (!il) return;
    enna_log(ENNA_MSG_INFO, NULL, "listener del: %s", il->name);
    _listeners = eina_list_remove(_listeners, il);
    eina_stringshare_del(il->name);
    ENNA_FREE(il);
}
