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
#include <libhal.h>
#include <libhal-storage.h>

#include "enna.h"
#include "logs.h"
#include "hal_storage.h"

#define ENNA_MODULE_NAME   "hal-storage"

static const struct {
    const char *name;
    LibHalDriveBus bus;
} drv_bus_mapping[] = {
    { "Unknown",                LIBHAL_DRIVE_BUS_UNKNOWN                  },
    { "IDE",                    LIBHAL_DRIVE_BUS_IDE                      },
    { "SCSI",                   LIBHAL_DRIVE_BUS_SCSI                     },
    { "USB",                    LIBHAL_DRIVE_BUS_USB                      },
    { "FireWire",               LIBHAL_DRIVE_BUS_IEEE1394                 },
    { "CCW",                    LIBHAL_DRIVE_BUS_CCW                      },
    { NULL }
};

static const struct {
    const char *name;
    LibHalDriveType type;
} drv_type_mapping[] = {
    { "Removable Disk",         LIBHAL_DRIVE_TYPE_REMOVABLE_DISK          },
    { "Disk",                   LIBHAL_DRIVE_TYPE_DISK                    },
    { "CD-ROM",                 LIBHAL_DRIVE_TYPE_CDROM                   },
    { "Floppy",                 LIBHAL_DRIVE_TYPE_FLOPPY                  },
    { "Tape",                   LIBHAL_DRIVE_TYPE_TAPE                    },
    { "CompactFlash",           LIBHAL_DRIVE_TYPE_COMPACT_FLASH           },
    { "MemoryStick",            LIBHAL_DRIVE_TYPE_MEMORY_STICK            },
    { "SmartMedia",             LIBHAL_DRIVE_TYPE_SMART_MEDIA             },
    { "SD/MMC",                 LIBHAL_DRIVE_TYPE_SD_MMC                  },
    { "Camera",                 LIBHAL_DRIVE_TYPE_CAMERA                  },
    { "Portable Audio Player",  LIBHAL_DRIVE_TYPE_PORTABLE_AUDIO_PLAYER   },
    { "ZIP",                    LIBHAL_DRIVE_TYPE_ZIP                     },
    { "JAZ",                    LIBHAL_DRIVE_TYPE_JAZ                     },
    { "FlashKey",               LIBHAL_DRIVE_TYPE_FLASHKEY                },
    { "MagnetoOptical",         LIBHAL_DRIVE_TYPE_MO                      },
    { NULL }
};

static storage_t *
storage_new (void)
{
    storage_t *s;

    s = calloc (1, sizeof (storage_t));

    return s;
}

void
storage_free (storage_t *s)
{
    if (!s)
        return;

    if (s->drv)
        libhal_drive_free (s->drv);
    if (s->udi)
        free (s->udi);
    if (s->bus)
        free (s->bus);
    if (s->drive_type)
        free (s->drive_type);

    if (s->model)
        free (s->model);
    if (s->vendor)
        free (s->vendor);
    if (s->serial)
        free (s->serial);

    free (s);
}

static void
storage_get_properties (storage_t *s)
{
    LibHalDriveBus bus;
    int i;

    if (!s)
        return;

    bus = libhal_drive_get_bus (s->drv);
    for (i = 0; drv_bus_mapping[i].name; i++)
        if (drv_bus_mapping[i].bus == bus)
        {
            s->bus = strdup (drv_bus_mapping[i].name);
            break;
        }

    s->type = libhal_drive_get_type (s->drv);
    for (i = 0; drv_type_mapping[i].name; i++)
        if (drv_type_mapping[i].type == s->type)
        {
            s->drive_type = strdup (drv_type_mapping[i].name);
            break;
        }

    if (libhal_drive_get_model (s->drv))
        s->model = strdup (libhal_drive_get_model (s->drv));

    if (libhal_drive_get_vendor (s->drv))
        s->vendor = strdup (libhal_drive_get_vendor (s->drv));

    if (libhal_drive_get_serial (s->drv))
        s->serial = strdup (libhal_drive_get_serial (s->drv));


    s->removable = libhal_drive_uses_removable_media (s->drv);
    s->media_available = libhal_drive_is_media_detected (s->drv);
    s->media_size = libhal_drive_get_media_size (s->drv);

    s->requires_eject = libhal_drive_requires_eject (s->drv);
    s->hotpluggable = libhal_drive_is_hotpluggable (s->drv);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Adding new HAL storage:\n" \
              "  udi: %s\n" \
              "  bus: %s\n" \
              "  drive_type: %s\n" \
              "  model: %s\n" \
              "  vendor: %s\n" \
              "  serial: %s\n" \
              "\n",
              s->udi, s->bus, s->drive_type, s->model, s->vendor, s->serial);
}

storage_t *
storage_append (LibHalContext *ctx, const char *udi)
{
    storage_t *s;
    LibHalDrive *drv;

    if (!ctx || !udi)
        return NULL;

    drv = libhal_drive_from_udi (ctx, udi);
    if (!drv)
        return NULL;

    s = storage_new ();
    s->drv = drv;
    s->udi = strdup (udi);

    storage_get_properties (s);

    return s;
}

static int
storage_find_helper (storage_t *s, const char *udi)
{
    if (!s || !s->udi)
        return -1;
    return strcmp (s->udi, udi);
}

storage_t *
storage_find (Eina_List *list, const char *udi)
{
    storage_t *s = NULL;
    Eina_List *l;

    if (!udi)
        return NULL;

    EINA_LIST_FOREACH(list, l, s)
	if (!storage_find_helper(s, udi))
	     break;

    return s;
}
