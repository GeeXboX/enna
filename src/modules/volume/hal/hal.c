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
#include <E_Hal.h>


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

    Eina_List *storages;
    Eina_List *volumes;
} Enna_Module_Hal;

static Enna_Module_Hal *mod;

E_DBus_Connection *dbus_conn;

static void
ehal_store_is_cb(void *user_data, void *reply_data, DBusError *error)
{
    char *udi = user_data;
    E_Hal_Device_Query_Capability_Return *ret = reply_data;

    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        goto error;
    }

    if (ret && ret->boolean)
    {
        storage_add(udi);
    }

error:
    free(udi);
}

static void
ehal_volume_is_cb(void *user_data, void *reply_data, DBusError *error)
{
    char *udi = user_data;
    E_Hal_Device_Query_Capability_Return *ret = reply_data;

    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        goto error;
    }
    if (ret && ret->boolean)
    {
        volume_add(udi, 0);
    }

error:
    free(udi);
}

static void
ehal_get_all_dev_cb(void *user_data, void *reply_data, DBusError *error)
{
    E_Hal_Manager_Get_All_Devices_Return *ret = reply_data;
    Eina_List *l;
    char *udi;

    if (!ret || !ret->strings) return;

    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        return;
    }

    EINA_LIST_FOREACH(ret->strings, l, udi)
    {
        e_hal_device_query_capability(dbus_conn, udi, EHAL_STORAGE_NAME,
                                      ehal_store_is_cb, strdup(udi));
        e_hal_device_query_capability(dbus_conn, udi, EHAL_VOLUME_NAME,
                                      ehal_volume_is_cb, strdup(udi));
    }
}

static void
ehal_dev_store_cb(void *user_data, void *reply_data, DBusError *error)
{
    E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply_data;
    Eina_List *l;
    char *device;

    if (!ret || !ret->strings) return;

    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        return;
    }

    EINA_LIST_FOREACH(ret->strings, l, device)
    {
        storage_add(device);
    }
}

static void
ehal_dev_vol_cb(void *user_data, void *reply_data, DBusError *error)
{
    E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply_data;
    Eina_List *l;
    char *device;

    if (!ret || !ret->strings) return;

    if (dbus_error_is_set(error))
    {
        dbus_error_free(error);
        return;
    }

    EINA_LIST_FOREACH(ret->strings, l, device)
    {
        volume_add(device, 1);
    }
}

static void
ehal_dev_added_cb(void *data, DBusMessage *msg)
{
    DBusError err;
    char *udi = NULL;

    dbus_error_init(&err);
    dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
    if (!udi) return;
    e_hal_device_query_capability(dbus_conn, udi, "storage",
                                  ehal_store_is_cb, strdup(udi));
    e_hal_device_query_capability(dbus_conn, udi, "volume",
                                  ehal_volume_is_cb, strdup(udi));
}

static void
ehal_dev_removed_cb(void *data, DBusMessage *msg)
{
    DBusError err;
    char *udi;

    dbus_error_init(&err);

    dbus_message_get_args(msg,
                          &err, DBUS_TYPE_STRING,
                          &udi, DBUS_TYPE_INVALID);
    storage_del(udi);
    volume_del(udi);
}

static void
ehal_dev_update_cb(void *data, DBusMessage *msg)
{
    DBusError err;
    char *udi, *capability;

    dbus_error_init(&err);

    dbus_message_get_args(msg,
                          &err, DBUS_TYPE_STRING,
                          &udi, DBUS_TYPE_STRING,
                          &capability, DBUS_TYPE_INVALID);
    if (!strcmp(capability, "storage"))
    {
        storage_add(udi);
    }
}

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_volume_hal
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Hal));
    mod->em = em;
    em->mod = mod;

    e_dbus_init();
    e_hal_init();

    dbus_conn = e_dbus_bus_get (DBUS_BUS_SYSTEM);
    if (!dbus_conn)
        goto dbus_error;

    mod->storages = NULL;

    mod->volumes = NULL;

    e_hal_manager_get_all_devices(dbus_conn, ehal_get_all_dev_cb, mod);

	e_hal_manager_find_device_by_capability(dbus_conn, EHAL_STORAGE_NAME,
                                            ehal_dev_store_cb, mod);
    e_hal_manager_find_device_by_capability(dbus_conn, EHAL_VOLUME_NAME,
                                            ehal_dev_vol_cb, mod);

    e_dbus_signal_handler_add(dbus_conn, E_HAL_SENDER, E_HAL_MANAGER_PATH,
                              E_HAL_MANAGER_INTERFACE, EHAL_ACTION_ADD,
                              ehal_dev_added_cb, mod);
    e_dbus_signal_handler_add(dbus_conn, E_HAL_SENDER, E_HAL_MANAGER_PATH,
                              E_HAL_MANAGER_INTERFACE, EHAL_ACTION_REMOVE,
                              ehal_dev_removed_cb, mod);
    e_dbus_signal_handler_add(dbus_conn, E_HAL_SENDER, E_HAL_MANAGER_PATH,
                              E_HAL_MANAGER_INTERFACE, EHAL_ACTION_UPDATE,
                              ehal_dev_update_cb, mod);
    return;

dbus_error:
    if (dbus_conn)
        e_dbus_connection_close (dbus_conn);
    e_hal_shutdown();
    e_dbus_shutdown ();
    return;
}

static void
module_shutdown(Enna_Module *em)
{
    Enna_Module_Hal *mod;

    mod = em->mod;;
    if (dbus_conn)
        e_dbus_connection_close (dbus_conn);
    e_hal_shutdown();
    e_dbus_shutdown ();
}

Enna_Module_Api ENNA_MODULE_API = {
    ENNA_MODULE_VERSION,
    "volume_hal",
    N_("Volumes from HAL"),
    NULL,
    N_("This module provide support for removable volumes"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

