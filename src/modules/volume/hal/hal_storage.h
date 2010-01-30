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

#ifndef HAL_STORAGE_H
#define HAL_STORAGE_H

typedef struct storage_s {

    int type;
    char *udi, *bus;
    char *drive_type;

    char *model, *vendor, *serial;

    char removable, media_available;
    unsigned long long media_size;

    char requires_eject, hotpluggable;
    char media_check_enabled;

    struct 
    {
        char *drive, *volume;
    } icon;

    Eina_List *volumes;

    unsigned char validated;
    unsigned char trackable;
} storage_t;

extern E_DBus_Connection *dbus_conn;

storage_t *storage_add(const char *udi);
void       storage_del(const char *udi);
storage_t *storage_find(const char *udi);

#endif /* HAL_STORAGE_H */
