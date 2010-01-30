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

#ifndef HAL_VOLUME_H
#define HAL_VOLUME_H

#include <E_Hal.h>
#include <Ecore.h>

#include "hal_storage.h"

typedef struct volume_s {
    int type;
    char *udi, *uuid;
    char *label, *icon, *fstype;
    char *cd_type;
    char *cd_content_type;
    char *device;
    unsigned long long size;

    char partition;
    int partition_number;
    char *partition_label;
    char mounted;
    char *mount_point;

    char *parent;
    storage_t *storage;
    void *prop_handler;
    Eina_List *mounts;

    unsigned char validated;

    char auto_unmount;                  // unmount, when last associated fm window closed
    char first_time;                    // volume discovery in init sequence
    Ecore_Timer *guard;                 // operation guard tim
    Enna_Volume *evol;
} volume_t;

extern E_DBus_Connection *dbus_conn;

volume_t *volume_add (const char *udi, char first_time);
volume_t *volume_find (const char *udi);
void volume_del(const char *udi);

#endif /* HAL_VOLUME_H */
