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

#include <stdio.h>
#include <string.h>
#include <Eina.h>
#include <E_Hal.h>

#include "enna.h"
#include "logs.h"
#include "hal_storage.h"
#include "hal_volume.h"

static Eina_List *_volumes = NULL;

static int
_dbus_format_error_msg(char **buf, volume_t *v, DBusError *error)
{
    int size, vu, vm, en;
    char *tmp;

    vu = strlen(v->udi) + 1;
    vm = strlen(v->mount_point) + 1;
    en = strlen(error->name) + 1;
    size = vu + vm + en + strlen(error->message) + 1;
    tmp = *buf = malloc(size);

    strcpy(tmp, v->udi);
    strcpy(tmp += vu, v->mount_point);
    strcpy(tmp += vm, error->name);
    strcpy(tmp += en, error->message);

    return size;
}

static void
_dbus_vol_prop_mount_modified_cb(void *data, void *reply_data, DBusError *error)
{
    volume_t *v = data;
    E_Hal_Device_Get_All_Properties_Return *ret = reply_data;
    int err = 0;

    if (!ret) return;
    if (dbus_error_is_set(error))
    {
        char *buf;
        int size;

        size = _dbus_format_error_msg(&buf, v, error);
        dbus_error_free(error);
        free(buf);
        return;
    }

    v->mounted = e_hal_property_bool_get(ret, "volume.is_mounted", &err);
    if (err)  enna_log(ENNA_MSG_ERROR, "hal-volume",
                        "HAL Error : can't get volume.is_mounted property");

    if (v->mount_point) free(v->mount_point);
    v->mount_point = e_hal_property_string_get(ret, "volume.mount_point", &err);
    if (err) enna_log(ENNA_MSG_ERROR, "hal-volume",
                        "HAL Error : can't get volume.is_mounted property");

    enna_log(ENNA_MSG_EVENT, "hal-volume", "udi: %s mount_point: %s mounted: %d\n", v->udi, v->mount_point, v->mounted);

    return;
}

static void
_dbus_prop_modified_cb(void *data, DBusMessage *msg)
{
    volume_t *v;
    DBusMessageIter iter, sub, subsub;
    struct {
        const char *name;
        int added;
        int removed;
    } prop;
    int num_changes = 0, i;

    if (!(v = data)) return;

    if (dbus_message_get_error_name(msg))
    {
       enna_log(ENNA_MSG_ERROR, "hal-volume", "DBUS ERROR: %s",
                dbus_message_get_error_name(msg));
        return;
    }
    if (!dbus_message_iter_init(msg, &iter)) return;

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INT32) return;
    dbus_message_iter_get_basic(&iter, &num_changes);
    if (num_changes == 0) return;

    dbus_message_iter_next(&iter);
    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) return;
    dbus_message_iter_recurse(&iter, &sub);

    for (i = 0; i < num_changes; i++, dbus_message_iter_next(&sub))
    {
        dbus_message_iter_recurse(&sub, &subsub);

        if (dbus_message_iter_get_arg_type(&subsub) != DBUS_TYPE_STRING) break;
        dbus_message_iter_get_basic(&subsub, &(prop.name));
        if (!strcmp(prop.name, "volume.mount_point"))
        {
            e_hal_device_get_all_properties(dbus_conn, v->udi,
                                            _dbus_vol_prop_mount_modified_cb,
                                            v);
            return;
        }

        dbus_message_iter_next(&subsub);
        dbus_message_iter_next(&subsub);
    }
}

static void
_dbus_vol_prop_cb(void *data, void *reply_data, DBusError *error)
{
    volume_t *v = data;
    storage_t *s = NULL;
    E_Hal_Device_Get_All_Properties_Return *ret = reply_data;
    int err = 0;
    char *str = NULL;

    if (!ret) goto error;
    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        goto error;
    }

    /* skip volumes with volume.ignore set */
    if (e_hal_property_bool_get(ret, "volume.ignore", &err) || err)
        goto error;

    /* skip volumes that aren't filesystems */

    //str = e_hal_property_string_get(ret, "volume.fsusage", &err);
    //if (err || !str) goto error;
    //if (strcmp(str, "filesystem")) goto error;
    //free(str);
    //str = NULL;

    v->uuid = e_hal_property_string_get(ret, "volume.uuid", &err);
    if (err) goto error;

    v->label = e_hal_property_string_get(ret, "volume.label", &err);

    v->fstype = e_hal_property_string_get(ret, "volume.fstype", &err);

    v->size = e_hal_property_uint64_get(ret, "volume.size", &err);

    v->mounted = e_hal_property_bool_get(ret, "volume.is_mounted", &err);
    if (err) goto error;

    v->partition = e_hal_property_bool_get(ret, "volume.is_partition", &err);
    if (err) goto error;

    v->mount_point = e_hal_property_string_get(ret, "volume.mount_point", &err);
    if (err) goto error;

    if (v->partition)
    {
        v->partition_number = e_hal_property_int_get(ret, "volume.partition.number", NULL);
        v->partition_label = e_hal_property_string_get(ret, "volume.partition.label", NULL);
    }

    v->parent = e_hal_property_string_get(ret, "info.parent", &err);
    if ((!err) && (v->parent))
    {
        s = storage_find(v->parent);
        if (s)
        {
            v->storage = s;
            s->volumes = eina_list_append(s->volumes, v);
        }
    }

    enna_log (ENNA_MSG_EVENT, "hal-volume",
              "Adding new HAL volume:\n" \
              "  udi: %s\n" \
              "  uuid: %s\n" \
              "  fstype: %s\n" \
              "  size: %llu\n" \
              "  label: %s\n" \
              "  partition: %d\n" \
              "  partition_number: %d\n" \
              "  partition_label: %s\n" \
              "  mounted: %d\n" \
              "  mount_point: %s\n",
              v->udi, v->uuid, v->fstype, v->size, v->label,
              v->partition, v->partition_number,
              v->partition ? v->partition_label : "(not a partition)",
              v->mounted, v->mount_point);
    v->validated = 1;
    {
        /* ENNA_VOLUME_TYPE type = VOLUME_TYPE_UNKNOWN; */

        /* if (!strcmp(v->storage->drive_type, "disk")) */
        /* { */
        /*     if (v->storage->removable == 0) */
        /*         type = VOLUME_TYPE_HDD; */
        /*     else */
        /*         icon = VOLUME_TYPE_REMOVABLE_DISK; */
        /* } */
        /* else if (!strcmp(v->storage->drive_type, "cdrom")) */
        /*     icon = "drive-optical"; */
        /* else if (!strcmp(v->storage->drive_type, "floppy")) */
        /*     icon = "media-floppy"; */
        /* else if (!strcmp(v->storage->drive_type, "tape")) */
        /*     icon = "media-tape"; */
        /* else if (!strcmp(v->storage->drive_type, "compact_flash") */
        /*          || !strcmp(v->storage->drive_type, "memory_stick") */
        /*          || !strcmp(v->storage->drive_type, "smart_media") */
        /*          || !strcmp(v->storage->drive_type, "sd_mmc")) */
        /*     icon = "media-flash"; */

        /* evol = enna_volume_new (); */
        /* evol->label = eina_stringshare_add(name); */
        /* evol->type = type; */
        /* evol->mount_point = eina_stringshare_add(uri); */
        /* evol->device_name = eina_stringshare_add(v->device); */
        /* /\* FIXME add this property correctly *\/ */
        /* evol->eject = NULL; */
        /* evol->is_ejectable = EINA_FALSE; */
        /* enna_log(ENNA_MSG_EVENT, "hal", "Add mount point [%s] %s", v->label, v->mount_point); */
        /* enna_volumes_add_emit(evol); */
        //}
    }
    return;

error:
    volume_del(v->udi);
    return;
}

static void
_volume_free(volume_t *v)
{
    if (v->storage)
    {
        v->storage->volumes = eina_list_remove(v->storage->volumes, v);
        v->storage = NULL;
    }
    if (v->udi) free(v->udi);
    if (v->uuid) free(v->uuid);
    if (v->label) free(v->label);
    if (v->icon) free(v->icon);
    if (v->fstype) free(v->fstype);
    if (v->partition_label) free(v->partition_label);
    if (v->mount_point) free(v->mount_point);
    if (v->parent) free(v->parent);
    free(v);
}

volume_t *
volume_find(const char *udi)
{
    Eina_List *l;
    volume_t *v;

    EINA_LIST_FOREACH(_volumes, l, v)
    {
        if (!strcmp(udi, v->udi)) return v;
    }
    return NULL;
}

volume_t *
volume_add(const char *udi, char first_time)
{
    volume_t *v;

    if (!udi) return NULL;
    if (volume_find(udi)) return NULL;
    v = ENNA_NEW(volume_t, 1);
    if (!v) return NULL;
    v->udi = strdup(udi);
    v->first_time = first_time;
    _volumes = eina_list_append(_volumes, v);
    e_hal_device_get_all_properties(dbus_conn, v->udi,
                                    _dbus_vol_prop_cb, v);
    v->prop_handler = e_dbus_signal_handler_add(dbus_conn,
                                                "org.freedesktop.Hal",
                                                udi,
                                                "org.freedesktop.Hal.Device",
                                                "PropertyModified",
                                                _dbus_prop_modified_cb, v);
    return v;
}

void
volume_del(const char *udi)
{
    volume_t *v;

    v = volume_find(udi);
    if (!v) return;
    if (v->guard)
    {
        ecore_timer_del(v->guard);
        v->guard = NULL;
    }
    if (v->prop_handler) e_dbus_signal_handler_del(dbus_conn, v->prop_handler);
    if (v->validated)
    {
        enna_log (ENNA_MSG_EVENT, "hal-volume",
                  "Remove %s\n", v->udi);
    }
    _volumes = eina_list_remove(_volumes, v);
    _volume_free(v);
}
