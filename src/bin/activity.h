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

#ifndef ACTIVITY_H
#define ACTIVITY_H

typedef enum _ENNA_CLASS_TYPE ENNA_CLASS_TYPE;

typedef struct _Enna_Class_Activity Enna_Class_Activity;

struct _Enna_Class_Activity
{
    const char *name;
    int pri;
    const char *label;
    const char *icon_file;
    const char *icon;
    struct
    {
        void (*class_init)(int dummy);
        void (*class_shutdown)(int dummy);
        void (*class_show)(int dummy);
        void (*class_hide)(int dummy);
        void (*class_event)(void *event_info);
    } func;
    Eina_List *categories;
};

int enna_activity_add(Enna_Class_Activity *class);
int enna_activity_del(const char *name);
void enna_activity_del_all (void);
Eina_List *enna_activities_get(void);
int enna_activity_init(const char *name);
int enna_activity_show(const char *name);
int enna_activity_shutdown(const char *name);
int enna_activity_hide(const char *name);
int enna_activity_event(Enna_Class_Activity *act, void *event_info);

#endif /* ACTIVITY_H */
