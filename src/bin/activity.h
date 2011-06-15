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

#ifndef ACTIVITY_H
#define ACTIVITY_H

#include "input.h"
#include "vfs.h"

typedef struct _Enna_Class_Activity Enna_Class_Activity;

struct _Enna_Class_Activity
{
    const char *name;
    int pri;
    const char *label;
    const char *icon_file;
    const char *icon;
    const char *bg;
    ENNA_VFS_CAPS caps;
    struct
    {
        void (*class_init)(void);
        const char*  (*class_quit_request)(void);
        void (*class_shutdown)(void);
        void (*class_show)(void);
        void (*class_hide)(void);
        void (*class_event)(enna_input event);
    } func;
};

void enna_activity_register(Enna_Class_Activity *act);
void enna_activity_unregister(Enna_Class_Activity *act);

#endif /* ACTIVITY_H */
