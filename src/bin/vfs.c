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

#include "enna.h"
#include "vfs.h"

static Eina_List *_enna_vfs_music = NULL;
static Eina_List *_enna_vfs_video = NULL;
static Eina_List *_enna_vfs_photo = NULL;

/* local subsystem functions */

static int _sort_cb(const void *d1, const void *d2)
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
int enna_vfs_init(Evas *evas)
{
    return 0;
}

int enna_vfs_append(const char *name, unsigned char type,
        Enna_Class_Vfs *vfs)
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

void enna_vfs_class_remove(const char *name, unsigned char type)
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
    if (type == ENNA_CAPS_MUSIC)
        return _enna_vfs_music;
    else if (type == ENNA_CAPS_VIDEO)
        return _enna_vfs_video;
    else if (type == ENNA_CAPS_PHOTO)
        return _enna_vfs_photo;

    return NULL;
}

static Enna_Vfs_File * enna_vfs_create_inode(const char *uri, const char *label,
        const char *icon, const char *icon_file, int dir)
{
    Enna_Vfs_File *f;

    f = calloc(1, sizeof(Enna_Vfs_File));
    f->uri = uri ? strdup(uri) : NULL;
    f->label = label ? strdup(label) : NULL;
    f->icon = icon ? strdup(icon) : NULL;
    f->icon_file = icon_file ? strdup(icon_file) : NULL;
    f->is_directory = dir;

    return f;
}

Enna_Vfs_File * enna_vfs_create_file(const char *uri, const char *label, const char *icon,
        const char *icon_file)
{
    return enna_vfs_create_inode(uri, label, icon, icon_file, 0);
}

Enna_Vfs_File * enna_vfs_create_directory(const char *uri, const char *label, const char *icon,
        const char *icon_file)
{
    return enna_vfs_create_inode(uri, label, icon, icon_file, 1);
}

void enna_vfs_remove(Enna_Vfs_File *f)
{
    if (!f)
        return;

    ENNA_FREE(f->uri);
    ENNA_FREE(f->label);
    ENNA_FREE(f->icon);
    ENNA_FREE(f->icon_file);
    ENNA_FREE(f);
}
