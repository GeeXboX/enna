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

#include <Eina.h>
#include <string.h>
#include <E_Hal.h>

#include "enna.h"
#include "logs.h"
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
    enna_log(ENNA_MSG_EVENT, "hal-storage",
              "Adding new HAL storage:\n"       \
              "  udi: %s\n"                     \
              "  bus: %s\n"                     \
              "  drive_type: %s\n"              \
              "  model: %s\n"                   \
              "  vendor: %s\n"                  \
              "  serial: %s\n"                  \
              "  icon drive: %s\n"              \
              "  icon volume: %s\n",
              s->udi, s->bus, s->drive_type, s->model, s->vendor,
              s->serial, s->icon.drive, s->icon.volume);
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


