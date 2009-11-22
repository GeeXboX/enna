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
#include <E_Hal.h>
#include <dbus/dbus.h>
#include <libhal.h>
#include <libhal-storage.h>

#include "enna.h"
#include "module.h"
#include "vfs.h"
#include "volumes.h"
#include "logs.h"
#include "hal_storage.h"
#include "hal_volume.h"

#define ENNA_MODULE_NAME   "hal"

#define EHAL_STORAGE_NAME  "storage"
#define EHAL_VOLUME_NAME   "volume"
#define EHAL_ACTION_ADD    "DeviceAdded"
#define EHAL_ACTION_REMOVE "DeviceRemoved"
#define EHAL_ACTION_UPDATE "NewCapability"

typedef struct _Enna_Module_Hal
{
    Evas *e;
    Enna_Module *em;
    E_DBus_Connection *dbus;
    DBusConnection *conn;
    LibHalContext *ctx;
    DBusError error;

    Eina_List *storages;
    Eina_List *volumes;
} Enna_Module_Hal;

static Enna_Module_Hal *mod;

/***********************************************/
/*          HAL Storage Management             */
/***********************************************/

static void
ehal_add_storage (char *udi)
{
    storage_t *s;

    if (!udi)
        return;

    s = storage_append (mod->ctx, udi);
    mod->storages = eina_list_append (mod->storages, s);
}

static void
ehal_remove_storage (char *udi)
{
    storage_t *s;
    Eina_List *l;
    if (!udi)
        return;

    EINA_LIST_FOREACH(mod->storages, l, s)
    {
        if (!strcmp (s->udi, udi))
            storage_free (s);
    }
}

static void
ehal_find_storages (void *data, void *reply, DBusError *error)
{
    Eina_List *l;
    E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply;
    char *udi;

    if (!ret || !ret->strings)
        return;

    if (dbus_error_is_set (error))
    {
        dbus_error_free (error);
        return;
    }

    EINA_LIST_FOREACH(ret->strings, l, udi)
        ehal_add_storage (udi);
}

static void
ehal_is_storage (void *data, void *reply, DBusError *error)
{
    E_Hal_Device_Query_Capability_Return *ret = reply;
    char *udi = data;

    if (dbus_error_is_set (error))
    {
        dbus_error_free (error);
        return;
    }

    if (ret && ret->boolean)
        ehal_add_storage (udi);
}

/***********************************************/
/*           HAL Volume Management             */
/***********************************************/

static void
vfs_add_volume_entry (volume_t *v)
{
    char name[256], tmp[4096];
    ENNA_VOLUME_TYPE type = VOLUME_TYPE_HDD;
    const char *uri = NULL;
    Enna_Volume *evol;

    if (!v)
        return;

    switch (v->type)
    {
        /* discard unknown volumes */
    case HAL_VOLUME_TYPE_UNKNOWN:
        return;

    case HAL_VOLUME_TYPE_HDD:
        /* discarded un-accessible HDDs */
        if (!v->mounted)
            return;

        switch (v->s->type)
        {
        case LIBHAL_DRIVE_TYPE_CAMERA:
            type = VOLUME_TYPE_CAMERA;
            break;

        case LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER:
            type = VOLUME_TYPE_AUDIO_PLAYER;
            break;

        case LIBHAL_DRIVE_TYPE_FLASHKEY:
            type = VOLUME_TYPE_FLASHKEY;
            break;
        case LIBHAL_DRIVE_TYPE_REMOVABLE_DISK:
            type = VOLUME_TYPE_REMOVABLE_DISK;
            break;

        case LIBHAL_DRIVE_TYPE_COMPACT_FLASH:
            type = VOLUME_TYPE_COMPACT_FLASH;
            break;
        case LIBHAL_DRIVE_TYPE_MEMORY_STICK:
            type = VOLUME_TYPE_MEMORY_STICK;
            break;
        case LIBHAL_DRIVE_TYPE_SMART_MEDIA:
            type = VOLUME_TYPE_SMART_MEDIA;
            break;
        case LIBHAL_DRIVE_TYPE_SD_MMC:
            type = VOLUME_TYPE_SD_MMC;
            break;
        default:
            type = VOLUME_TYPE_HDD;
            break;
        }
        snprintf(tmp, sizeof(tmp), "file://%s", v->mount_point);
        uri = eina_stringshare_add(tmp);
        break;
    case HAL_VOLUME_TYPE_CD:
        type = VOLUME_TYPE_CD;
        snprintf(tmp, sizeof(tmp), "file://%s", v->mount_point);
        uri = eina_stringshare_add(tmp);
        break;
    case HAL_VOLUME_TYPE_CDDA:
        type =  VOLUME_TYPE_CDDA;
        uri  =  eina_stringshare_add("cdda://");
        break;
    case HAL_VOLUME_TYPE_DVD:
        type =  VOLUME_TYPE_DVD;
        snprintf(tmp, sizeof(tmp), "file://%s", v->mount_point);
        uri = eina_stringshare_add(tmp);
        break;
    case HAL_VOLUME_TYPE_DVD_VIDEO:
        type =  VOLUME_TYPE_DVD_VIDEO;
        uri  =  eina_stringshare_add("dvd://");
        break;
    case HAL_VOLUME_TYPE_VCD:
        type = VOLUME_TYPE_VCD;
        uri =  eina_stringshare_add("vcd://");
        break;
    case HAL_VOLUME_TYPE_SVCD:
        type = VOLUME_TYPE_SVCD;
        uri =  eina_stringshare_add("vcd://");
        break;
    }

    /* get volume displayed name/label */
    memset (name, '\0', sizeof (name));
    if (v->partition_label)
        snprintf (name, sizeof (name), "%s", v->partition_label);
    else if (v->label)
        snprintf (name, sizeof (name), "%s", v->label);
    else if (v->s)
        snprintf (name, sizeof (name), "%s %s", v->s->vendor, v->s->model);

    if (!v->name)
        v->name = strdup (name);

    evol = enna_volume_new ();
    evol->label = eina_stringshare_add(name);
    evol->type = type;
    evol->mount_point = eina_stringshare_add(uri);
    evol->device_name = eina_stringshare_add(v->device);
    /* FIXME add this property correctly */
    evol->eject = NULL;
    evol->is_ejectable = EINA_FALSE;
    v->enna_volume = evol;
    enna_log(ENNA_MSG_EVENT, "hal", "Add mount point [%s] %s", v->label, v->mount_point);
    enna_volumes_add_emit(evol);
}

static void
vfs_remove_volume_entry (volume_t *v)
{
    enna_log(ENNA_MSG_EVENT, "hal", "Add mount point [%s] %s", v->label, v->mount_point);
    enna_volumes_remove_emit(v->enna_volume);
    enna_volume_free(v->enna_volume);
}

static void
ehal_add_volume (char *udi)
{
    volume_t *v;

    if (!udi)
        return;

    v = volume_append (mod->ctx, udi);
    if (v && v->parent)
    {
        storage_t *s;
        s = storage_find (mod->storages, v->parent);
        if (s)
            v->s = s;
    }

    mod->volumes = eina_list_append (mod->volumes, v);
    vfs_add_volume_entry (v);
}

static void
ehal_remove_volume (char *udi)
{
    volume_t *v;
    Eina_List *l, *l_next;

    if (!udi)
        return;

    EINA_LIST_FOREACH_SAFE(mod->volumes, l, l_next, v);
    {
        if (v && v->udi && !strcmp (v->udi, udi))
        {
            vfs_remove_volume_entry(v);
            mod->volumes = eina_list_remove (mod->volumes, v);
            volume_free(v);
        }
    }
}

static void
ehal_find_volumes (void *data, void *reply, DBusError *error)
{
    Eina_List *l;
    E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply;
    char *udi;

    if (!ret || !ret->strings)
        return;

    if (dbus_error_is_set (error))
    {
        dbus_error_free (error);
        return;
    }

    EINA_LIST_FOREACH(ret->strings, l, udi)
        ehal_add_volume (udi);
}

static void
ehal_is_volume (void *data, void *reply, DBusError *error)
{
    E_Hal_Device_Query_Capability_Return *ret = reply;
    char *udi = data;

    if (dbus_error_is_set (error))
    {
        dbus_error_free (error);
        return;
    }

    if (ret && ret->boolean)
      ehal_add_volume (udi);
}

/***********************************************/
/*           HAL Device Management             */
/***********************************************/

static void
ehal_device_added (void *data, DBusMessage *msg)
{
    Enna_Module_Hal *mod = data;
    DBusError err;
    char *udi;

    dbus_error_init (&err);
    dbus_message_get_args (msg, &err,
                           DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Adding newly discovered device: %s", udi);

    e_hal_device_query_capability (mod->dbus, udi, EHAL_STORAGE_NAME,
                                   ehal_is_storage, udi);
    e_hal_device_query_capability (mod->dbus, udi, EHAL_VOLUME_NAME,
                                   ehal_is_volume, udi);
}

static void
ehal_device_removed (void *data, DBusMessage *msg)
{
    DBusError err;
    char *udi;

    dbus_error_init (&err);

    dbus_message_get_args (msg, &err,
                           DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Removing newly disconnected device: %s", udi);

    ehal_remove_volume (udi);
    ehal_remove_storage (udi);
}

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_volume_hal
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API = {
    ENNA_MODULE_VERSION,
    "volume_hal",
    N_("Volumes from HAL"),
    NULL,
    N_("This module provide support for removable volumes"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Hal));
    mod->em = em;
    em->mod = mod;

    e_dbus_init();
    e_hal_init();

    mod->dbus = e_dbus_bus_get (DBUS_BUS_SYSTEM);
    if (!mod->dbus)
        goto edbus_error;

    dbus_error_init (&mod->error);
    mod->conn = dbus_bus_get (DBUS_BUS_SYSTEM, &mod->error);
    if (!mod->conn)
        goto dbus_error;

    mod->ctx = libhal_ctx_new ();
    if (!mod->ctx)
        goto hal_error;

    libhal_ctx_set_dbus_connection (mod->ctx, mod->conn);
    libhal_ctx_init (mod->ctx, &mod->error);

    mod->storages = NULL;

    mod->volumes = NULL;

    e_hal_manager_find_device_by_capability (mod->dbus,
                                             EHAL_STORAGE_NAME,
                                             ehal_find_storages, mod);
    e_hal_manager_find_device_by_capability (mod->dbus,
                                             EHAL_VOLUME_NAME,
                                             ehal_find_volumes, mod);

    e_dbus_signal_handler_add (mod->dbus, E_HAL_SENDER, E_HAL_MANAGER_PATH,
                               E_HAL_MANAGER_INTERFACE, EHAL_ACTION_ADD,
                               ehal_device_added, mod);

    e_dbus_signal_handler_add (mod->dbus, E_HAL_SENDER, E_HAL_MANAGER_PATH,
                               E_HAL_MANAGER_INTERFACE, EHAL_ACTION_REMOVE,
                               ehal_device_removed, NULL);

    return;

 hal_error:
    dbus_connection_unref (mod->conn);
    dbus_error_free (&mod->error);

 dbus_error:
    e_dbus_connection_close (mod->dbus);
    e_dbus_shutdown ();

 edbus_error:
    return;
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Module_Hal *mod;

    mod = em->mod;;

    libhal_ctx_shutdown (mod->ctx, &mod->error);
    libhal_ctx_free (mod->ctx);

    dbus_connection_unref (mod->conn);
    dbus_error_free (&mod->error);

    e_dbus_connection_close (mod->dbus);
    e_dbus_shutdown ();
}
