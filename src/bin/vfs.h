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

#ifndef VFS_H
#define VFS_H

#include "enna.h"
#include "browser.h"

typedef enum _ENNA_VFS_CAPS ENNA_VFS_CAPS;
typedef struct _Enna_Vfs_Class Enna_Vfs_Class;

enum _ENNA_VFS_CAPS
{
    ENNA_CAPS_NONE = 0x00,
    ENNA_CAPS_MUSIC = 0x01,
    ENNA_CAPS_VIDEO = 0x02,
    ENNA_CAPS_PHOTO = 0x04
};

struct _Enna_Vfs_Class
{
    const char *name;
    int pri;
    const char *label;
    const char *icon_file;
    const char *icon;
    struct
    {
        void *(* add)(Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps);
        void  (* get_children)(void *priv, Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps);
        void  (* del)(void *priv);
    } func;
    void *cookie;

};

int enna_vfs_init(Evas *evas);
int enna_vfs_append(const char *name, unsigned char type,
        Enna_Vfs_Class *vfs);
void enna_vfs_register(Enna_Vfs_Class *class, ENNA_VFS_CAPS type);
void enna_vfs_class_remove(const char *name, unsigned char type);
Eina_List *enna_vfs_get(ENNA_VFS_CAPS type);
Enna_File *enna_vfs_dup_file(const Enna_File* file);
Enna_File *enna_vfs_create_file (const char *uri, const char *label, const char *icon, const char *icon_file);
Enna_File *enna_vfs_create_directory (const char *uri, const char *label, const char *icon, const char *icon_file);
Enna_File * enna_vfs_create_menu(const char *uri, const char *label, const char *icon, const char *icon_file);
void enna_vfs_remove(Enna_File *f);
#endif /* VFS_H */
