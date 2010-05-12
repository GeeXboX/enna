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

#include "enna.h"
#include "vfs.h"

static Eina_List *_enna_vfs_music = NULL;
static Eina_List *_enna_vfs_video = NULL;
static Eina_List *_enna_vfs_photo = NULL;
static Eina_List *_enna_vfs = NULL;
/* local subsystem functions */

static int
_sort_cb(const void *d1, const void *d2)
{
    const Enna_Class_Vfs *vfs1 = d1;
    const Enna_Class_Vfs *vfs2 = d2;

    if (vfs1->pri > vfs2->pri)
        return 1;
    else if (vfs1->pri < vfs2->pri)
        return -1;
    else
        return strcasecmp(vfs1->name, vfs2->name);
}

/* externally accessible functions */
int
enna_vfs_init(Evas *evas)
{
    return 0;
}

void
enna_vfs_register(Enna_Class2_Vfs *vfs)
{
    _enna_vfs = eina_list_append(_enna_vfs, vfs);
}

int
enna_vfs_append(const char *name, unsigned char type, Enna_Class_Vfs *vfs)
{
    if (!vfs)
        return -1;

    if (type & ENNA_CAPS_MUSIC)
    {
        _enna_vfs_music = eina_list_append(_enna_vfs_music, vfs);
        _enna_vfs_music = eina_list_sort(
            _enna_vfs_music,
            eina_list_count(_enna_vfs_music),
            _sort_cb);
    }

    if (type & ENNA_CAPS_VIDEO)
    {
        _enna_vfs_video = eina_list_append(_enna_vfs_video, vfs);
        _enna_vfs_video = eina_list_sort(
            _enna_vfs_video,
            eina_list_count(_enna_vfs_video),
            _sort_cb);
    }

    if (type & ENNA_CAPS_PHOTO)
    {
        _enna_vfs_photo = eina_list_append(_enna_vfs_photo, vfs);
        _enna_vfs_photo = eina_list_sort(
            _enna_vfs_photo,
            eina_list_count(_enna_vfs_photo),
            _sort_cb);
    }

    return 0;
}

void
enna_vfs_class_remove(const char *name, unsigned char type)
{
    Eina_List *tmp;

    if (!name)
        return;

    tmp = enna_vfs_get (type);
    tmp = eina_list_nth_list (tmp, 0);
    do {
        Enna_Class_Vfs *class = (Enna_Class_Vfs *) tmp->data;
        if (class && !strcmp (class->name, name))
            tmp = eina_list_remove (tmp, class);
    } while ((tmp = eina_list_next (tmp)));
}

Eina_List *
enna_vfs_get(ENNA_VFS_CAPS type)
{
//     if (type == ENNA_CAPS_MUSIC)
//         return _enna_vfs_music;
//     else if (type == ENNA_CAPS_VIDEO)
//         return _enna_vfs_video;
//     else if (type == ENNA_CAPS_PHOTO)
//         return _enna_vfs_photo;

    return _enna_vfs;
}

static Enna_Vfs_File *
enna_vfs_create_inode(const char *uri, const char *label,
                      const char *icon, const char *icon_file, int dir)
{
    Enna_Vfs_File *f;

    f = calloc(1, sizeof(Enna_Vfs_File));
    f->uri = uri ? strdup(uri) : NULL;
    f->label = label ? strdup(label) : NULL;
    f->icon = icon ? strdup(icon) : NULL;
    f->icon_file = icon_file ? strdup(icon_file) : NULL;

    if (dir == 1)
        f->is_directory = 1;
    else if (dir == 2)
        f->is_menu = 1;

    return f;
}

Enna_Vfs_File *
enna_vfs_dup_file(const Enna_Vfs_File *file)
{
    Enna_Vfs_File *n;

    n = calloc(1, sizeof(Enna_Vfs_File));
    if (!n)
        return NULL;

    n->uri       = file->uri       ? strdup(file->uri)       : NULL;
    n->label     = file->label     ? strdup(file->label)     : NULL;
    n->icon      = file->icon      ? strdup(file->icon)      : NULL;
    n->icon_file = file->icon_file ? strdup(file->icon_file) : NULL;

    n->is_directory = file->is_directory;
    n->is_menu = file->is_menu;

    return n;
}

Enna_Vfs_File *
enna_vfs_create_file(const char *uri, const char *label,
                     const char *icon, const char *icon_file)
{
    return enna_vfs_create_inode(uri, label, NULL, NULL, 0);
}

Enna_Vfs_File *
enna_vfs_create_directory(const char *uri, const char *label,
                          const char *icon, const char *icon_file)
{
    return enna_vfs_create_inode(uri, label, icon, icon_file, 1);
}

Enna_Vfs_File *
enna_vfs_create_menu(const char *uri, const char *label,
                     const char *icon, const char *icon_file)
{
    return enna_vfs_create_inode(uri, label, icon, icon_file, 2);
}

void
enna_vfs_remove(Enna_Vfs_File *f)
{
    if (!f)
        return;

    ENNA_FREE(f->uri);
    ENNA_FREE(f->label);
    ENNA_FREE(f->icon);
    ENNA_FREE(f->icon_file);
    ENNA_FREE(f);
}
