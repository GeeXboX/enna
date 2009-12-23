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
