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

#include <string.h>
#include <ctype.h>

#include <Ecore.h>
#include <Ecore_Data.h>

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

    enna_log(ENNA_MSG_INFO, NULL, "Input emit: %d (listeners: %d)", in, eina_list_count(_listeners));

    enna_idle_timer_renew();
    EINA_LIST_FOREACH(_listeners, l, il)
    {
        enna_log(ENNA_MSG_INFO, NULL, "  emit to: %s", il->name);
        if (!il->func) continue;

        ret = il->func(il->data, in);
        if (ret == ENNA_EVENT_BLOCK)
        {
            enna_log(ENNA_MSG_INFO, NULL, "  emission stopped by: %s", il->name);
            return EINA_TRUE;
        }
    }

    return EINA_TRUE;
}

Input_Listener *
enna_input_listener_add(const char *name, Eina_Bool(*func)(void *data, enna_input event), void *data)
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
