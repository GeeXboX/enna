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

#ifndef VOLUMES_H
#define VOLUMES_H

#include "enna.h"

typedef struct _Enna_Volume Enna_Volume;

struct _Enna_Volume
{
    const char *name;
    const char *label;
    const char *icon; /* edje or file icon*/
    const char *uri;  /* Uri of root : file:///media/disk-1 or cdda:// */
    const char *type; /* Uri type : cdda:// dvdnav:// file:// ...*/
    const char *device; /* Physical device : /dev/sda1 ... */
};

extern int ENNA_EVENT_VOLUME_ADDED;
extern int ENNA_EVENT_VOLUME_REMOVED;
extern int ENNA_EVENT_REFRESH_BROWSER;

void enna_volumes_init(void);
void enna_volumes_shutdown(void);
void enna_volumes_append(const char *type, Enna_Volume *v);
void enna_volumes_remove(const char *type, Enna_Volume *v);
Eina_List *enna_volumes_get(const char *type);

#endif /* VOLUMES_H */
