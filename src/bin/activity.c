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

#include "enna.h"
#include "activity.h"

static Eina_List *_enna_activities = NULL;

static int _sort_cb(const void *d1, const void *d2)
{
    const Enna_Class_Activity *act1 = d1;
    const Enna_Class_Activity *act2 = d2;

    if (act1->pri > act2->pri)
        return 1;
    else if (act1->pri < act2->pri)
        return -1;
    else
        return strcasecmp(act1->name, act2->name);
}

/**
 * @brief Register new activity
 * @param em enna module
 * @return -1 if error occurs, 0 otherwise
 */
int enna_activity_add(Enna_Class_Activity *class)
{
    if (!class)
        return -1;

    _enna_activities = eina_list_append(_enna_activities, class);
    _enna_activities = eina_list_sort(_enna_activities,
        eina_list_count(_enna_activities),
        _sort_cb);

    return 0;
}

static Enna_Class_Activity * enna_get_activity(const char *name)
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
 * @brief Unregister an existing activity
 * @param em enna module
 * @return -1 if error occurs, 0 otherwise
 */
int enna_activity_del(const char *name)
{
    Enna_Class_Activity *act;

    act = enna_get_activity (name);
    if (!act)
        return -1;

    _enna_activities = eina_list_remove(_enna_activities, act);
    return 0;
}

/**
 * @brief Unregister all existing activities
 * @return -1 if error occurs, 0 otherwise
 */
void enna_activity_del_all (void)
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

int enna_activity_init(const char *name)
{
    Enna_Class_Activity *act;

    act = enna_get_activity(name);
    if (!act)
        return -1;

    if (act->func.class_init)
        act->func.class_init(0);

    return 0;
}

int enna_activity_show(const char *name)
{
    Enna_Class_Activity *act;

    act = enna_get_activity(name);
    if (!act)
        return -1;

    if (act->func.class_show)
        act->func.class_show(0);

    return 0;
}

int enna_activity_shutdown(const char *name)
{
    Enna_Class_Activity *act;

    act = enna_get_activity(name);
    if (!act)
        return -1;

    if (act->func.class_shutdown)
        act->func.class_shutdown(0);

    return 0;
}

int enna_activity_hide(const char *name)
{
    Enna_Class_Activity *act;

    act = enna_get_activity(name);
    if (!act)
        return -1;

    if (act->func.class_hide)
        act->func.class_hide(0);

    return 0;
}

int enna_activity_event(Enna_Class_Activity *act, void *event_info)
{

    if (!act)
        return -1;

    if (act->func.class_event)
        act->func.class_event(event_info);

    return 0;
}
