/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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

#include <Eina.h>
#include <string.h>
#include <E_Hal.h>

#include "enna.h"
#include "logs.h"
#include "volumes.h"
#include "hal_storage.h"
#include "hal_volume.h"

static Eina_List *stores = NULL;

static void
_storage_free(storage_t *s)
{
    volume_t *v;
    EINA_LIST_FREE(s->volumes, v)
    {
        //volume_free(v);
    }
    if (s->udi) free(s->udi);
    if (s->bus) free(s->bus);
    if (s->drive_type) free(s->drive_type);
    if (s->model) free(s->model);
    if (s->vendor) free(s->vendor);
    if (s->serial) free(s->serial);
    if (s->icon.drive) free(s->icon.drive);
    if (s->icon.volume) free(s->icon.volume);
    free(s);
}

static void
_dbus_store_prop_cb(void *data, void *reply_data, DBusError *error)
{
    storage_t *s = data;
    E_Hal_Properties *ret = reply_data;
    int err = 0;
    char *str = NULL;

    if (!ret) goto error;

    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        goto error;
    }

    s->bus = e_hal_property_string_get(ret, "storage.bus", &err);
    if (err) goto error;
    s->drive_type = e_hal_property_string_get(ret, "storage.drive_type", &err);
    if (err) goto error;
    s->model = e_hal_property_string_get(ret, "storage.model", &err);
    if (err) goto error;
    s->vendor = e_hal_property_string_get(ret, "storage.vendor", &err);
    if (err) goto error;
    s->serial = e_hal_property_string_get(ret, "storage.serial", &err);
    if (err)  enna_log(ENNA_MSG_ERROR, "hal-storage",
                       "Error getting serial for %s\n", s->udi);

    s->removable = e_hal_property_bool_get(ret, "storage.removable", &err);

    if (s->removable)
    {
        s->media_available = e_hal_property_bool_get(ret, "storage.removable.media_available", &err);
        s->media_size = e_hal_property_uint64_get(ret, "storage.removable.media_size", &err);
    }

    s->requires_eject = e_hal_property_bool_get(ret, "storage.requires_eject", &err);
    s->hotpluggable = e_hal_property_bool_get(ret, "storage.hotpluggable", &err);
    s->media_check_enabled = e_hal_property_bool_get(ret, "storage.media_check_enabled", &err);

    s->icon.drive = e_hal_property_string_get(ret, "storage.icon.drive", &err);
    s->icon.volume = e_hal_property_string_get(ret, "storage.icon.volume", &err);

    str = e_hal_property_string_get(ret, "storage.drive_type", &err);

    if (str)
    {
		if (!strcmp (str, "cdrom"))
            s->type = VOLUME_TYPE_CD;
        else if (!strcmp (str, "optical"))
			s->type = VOLUME_TYPE_CD;
        else if (!strcmp (str, "disk"))
        {
			if (s->removable)
				s->type = VOLUME_TYPE_REMOVABLE_DISK;
			else
				s->type = VOLUME_TYPE_HDD;
		}
        else if (!strcmp (str, "compact_flash"))
			s->type = VOLUME_TYPE_COMPACT_FLASH;
        else if (!strcmp (str, "memory_stick"))
			s->type = VOLUME_TYPE_MEMORY_STICK;
        else if (!strcmp (str, "smart_media"))
			s->type = VOLUME_TYPE_SMART_MEDIA;
        else if (!strcmp (str, "sd_mmc"))
			s->type = VOLUME_TYPE_SD_MMC;
        else if (!strcmp (str, "flashkey"))
			s->type = VOLUME_TYPE_FLASHKEY;
        else
            s->type = VOLUME_TYPE_HDD;
	}

    enna_log(ENNA_MSG_EVENT, "hal-storage",
             "Adding new HAL storage:\n"        \
             "  udi: %s\n"                      \
             "  bus: %s\n"                      \
             "  drive_type: %s\n"               \
             "  model: %s\n"                    \
             "  vendor: %s\n"                   \
             "  serial: %s\n"                   \
             "  icon drive: %s\n"               \
             "  icon volume: %s\n"              \
             "  drive_type: %s\n"               \
             "  removable: %d\n",
             s->udi, s->bus, s->drive_type, s->model, s->vendor,
             s->serial, s->icon.drive, s->icon.volume, str, s->removable);
    free(str);
    s->validated = 1;
    return;

error:
    enna_log(ENNA_MSG_ERROR, "hal-storage",
             "Error %s\n", s->udi);
    storage_del(s->udi);
}

storage_t *
storage_find(const char *udi)
{
    Eina_List *l;
    storage_t  *s;

    EINA_LIST_FOREACH(stores, l, s)
    {
        if (!strcmp(udi, s->udi)) return s;
    }
    return NULL;
}

storage_t *
storage_add(const char *udi)
{
    storage_t *s;

    if (!udi) return NULL;
    if (storage_find(udi)) return NULL;
    s = ENNA_NEW(storage_t, 1);
    s->udi = strdup(udi);
    stores = eina_list_append(stores, s);
    e_hal_device_get_all_properties(dbus_conn, s->udi,
                                    _dbus_store_prop_cb, s);
    return s;
}

void
storage_del(const char *udi)
{
    storage_t *s;

    s = storage_find(udi);
    if (!s) return;
    stores = eina_list_remove(stores, s);
    _storage_free(s);
}


