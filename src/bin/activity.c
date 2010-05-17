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

#include "enna.h"
#include "activity.h"
#include "buffer.h"
#include "logs.h"
#include "vfs.h"
#include "browser.h"

static Eina_List *_enna_activities = NULL;

static int
_sort_cb(const void *d1, const void *d2)
{
    const Enna_Class_Activity *act1 = d1;
    const Enna_Class_Activity *act2 = d2;

    if (act1->pri > act2->pri)
        return 1;
    else if (act1->pri < act2->pri)
        return -1;

    return strcasecmp(act1->name, act2->name);
}

Enna_Class_Activity *
enna_activity_get(const char *name)
{
    Eina_List *l;
    Enna_Class_Activity *act;

    if (!name)
        return NULL;

    for (l = _enna_activities; l; l = l->next)
    {
        act = l->data;
        if (!act)
            continue;

        if (act->name && !strcmp(act->name, name))
            return act;
    }

    return NULL;
}

/**
 * @brief Unregister all existing activities
 * @return -1 if error occurs, 0 otherwise
 */
void
enna_activity_del_all (void)
{
    Eina_List *l;

    for (l = _enna_activities; l; l = l->next)
    {
        Enna_Class_Activity *act = l->data;
        _enna_activities = eina_list_remove(_enna_activities, act);
    }
}

/**
 * @brief Get list of activities registred
 * @return Eina_List of activities
 */
Eina_List *
enna_activities_get(void)
{
    return _enna_activities;
}

#define ACTIVITY_FUNC(func, ...) \
int \
enna_activity_##func(const char *name) \
{ \
    Enna_Class_Activity *act; \
    \
    act = enna_activity_get(name); \
    if (!act) \
       return -1; \
    \
    ACTIVITY_CLASS(func, __VA_ARGS__); \
    return 0; \
}

ACTIVITY_FUNC(init, 0);
ACTIVITY_FUNC(quit_request, 0);
ACTIVITY_FUNC(shutdown, 0);
ACTIVITY_FUNC(show, 0);
ACTIVITY_FUNC(hide, 0);

int
enna_activity_event(Enna_Class_Activity *act, enna_input event)
{
    if (!act)
        return -1;
    ACTIVITY_CLASS(event, event);
    return 0;
}

const char *
enna_activity_request_quit_all(void)
{
    Eina_List *l;
    buffer_t *msg;
    Enna_Class_Activity *act;
    const char *tmp = NULL;
    msg = buffer_new();
    EINA_LIST_FOREACH(_enna_activities, l,  act)
    {
        if (act->func.class_quit_request)
        {
          tmp = act->func.class_quit_request (0);
          if (tmp)
            buffer_appendf(msg, "%s<t> : <hilight>%s</hilight><br>", act->label, tmp);
        }
    }
    if (msg->buf)
        tmp = strdup(msg->buf);
    buffer_free(msg);
    return tmp;
}

/**
 * @brief Register new activity
 * @param class activity class to register */

void enna_activity_register(Enna_Class_Activity *act)
{
    Enna_Vfs_File *file;
    
    if (!act)
        return;

    file = calloc(1, sizeof(Enna_Vfs_File));
    file->name = eina_stringshare_add(act->name);
    file->label = eina_stringshare_add(act->label);
    file->icon = eina_stringshare_add(act->icon);
    file->is_directory = EINA_TRUE;
    file->icon_file = eina_stringshare_add(act->bg);
    file->uri = eina_stringshare_add(act->label);
    _enna_activities = eina_list_append(_enna_activities, act);
    _enna_activities = eina_list_sort(_enna_activities,
                                      eina_list_count(_enna_activities),
                                      _sort_cb);

    return;
}

/**
 * @brief Unregister an existing activity
 * @param act the activity class to unregister
 */
void enna_activity_unregister(Enna_Class_Activity *act)
{
    Eina_List *l;

    if (!act)
        return;

    for (l = _enna_activities; l; l = l->next)
    {
        Enna_Class_Activity *a = l->data;
        if (a == act)
        {
            _enna_activities = eina_list_remove(_enna_activities, act);
            return;
        }
    }
}
