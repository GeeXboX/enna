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

#ifndef __ENNA_VFS_H__
#define __ENNA_VFS_H__

#include "enna.h"

typedef enum _ENNA_VFS_CAPS ENNA_VFS_CAPS;
typedef struct _Enna_Class_Vfs Enna_Class_Vfs;
typedef struct _Enna_Vfs_File Enna_Vfs_File;

enum _ENNA_VFS_CAPS
{
    ENNA_CAPS_MUSIC = 0x01,
    ENNA_CAPS_VIDEO = 0x02,
    ENNA_CAPS_PHOTO = 0x04
};

struct _Enna_Vfs_File
{
    char *uri;
    char *label;
    char *icon;
    char *icon_file;
    unsigned char is_directory : 1;
    unsigned char is_selected : 1;

};

struct _Enna_Class_Vfs
{
    const char *name;
    int pri;
    const char *label;
    const char *icon_file;
    const char *icon;
    struct
    {
        void (*class_init)(int dummy, void *cookie);
        void (*class_shutdown)(int dummy, void *cookie);
        Eina_List *(*class_browse_up)(const char *path, void *cookie);
        Eina_List *(*class_browse_down)(void *cookie);
        Enna_Vfs_File *(*class_vfs_get)(void *cookie);
    } func;
    void *cookie;

};
int enna_vfs_init(Evas *evas);
int enna_vfs_append(const char *name, unsigned char type,
        Enna_Class_Vfs *vfs);
void enna_vfs_class_remove(const char *name, unsigned char type);
Eina_List *enna_vfs_get(ENNA_VFS_CAPS type);
Enna_Vfs_File *enna_vfs_create_file (const char *uri, const char *label, const char *icon, const char *icon_file);
Enna_Vfs_File *enna_vfs_create_directory (const char *uri, const char *label, const char *icon, const char *icon_file);
void enna_vfs_remove(Enna_Vfs_File *f);
#endif
