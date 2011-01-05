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

#include "file.h"
#include "enna.h"


static Enna_File *
_create_inode(const char *name, const char *uri, const char *label,
              const char *icon, const char *mrl, Enna_File_Type type)
{
    Enna_File *f;

    f = calloc(1, sizeof(Enna_File));
    f->name  = name  ? eina_stringshare_add(name)  : NULL;
    f->uri   = uri   ? eina_stringshare_add(uri)   : NULL;
    f->label = label ? eina_stringshare_add(label) : NULL;
    f->icon  = icon  ? eina_stringshare_add(icon)  : NULL;
    f->mrl   = mrl   ? eina_stringshare_add(mrl)   : NULL;
    f->type = type;

    return f;
}

Enna_File *
enna_file_dup(Enna_File *file)
{
    Enna_File *f;

    if (!file)
        return NULL;

    f = calloc(1, sizeof(Enna_File));
    f->icon = eina_stringshare_add(file->icon);
    f->icon_file = eina_stringshare_add(file->icon_file);
    f->type = file->type;
    f->label = eina_stringshare_add(file->label);
    f->name = eina_stringshare_add(file->name);
    f->uri = eina_stringshare_add(file->uri);
    f->mrl = eina_stringshare_add(file->mrl);
    f->meta_class = file->meta_class;
    f->meta_data = file->meta_data;
    return f;
}

void
enna_file_free(Enna_File *f)
{
    if (!f)
        return;

    if (f->name) eina_stringshare_del(f->name);
    if (f->uri) eina_stringshare_del(f->uri);
    if (f->label) eina_stringshare_del(f->label);
    if (f->icon) eina_stringshare_del(f->icon);
    if (f->icon_file) eina_stringshare_del(f->icon_file);
    if (f->mrl) eina_stringshare_del(f->mrl);
    if (f->meta_class && f->meta_class->meta_del)
        f->meta_class->meta_del(f->meta_data);
    free(f);
}

void
enna_file_meta_add(Enna_File *f, Enna_File_Meta_Class *meta_class, void *data)
{
    if (!f || !meta_class)
        return;

    f->meta_class = meta_class;
    f->meta_data = data;
}

const char *
enna_browser_file_meta_get(Enna_File *f, const char *key)
{
    if (!f || !key || !f->meta_class || !f->meta_class->meta_get)
        return NULL;

    return f->meta_class->meta_get(f->meta_data, f, key);
}

Enna_File *
enna_browser_create_file(const char *name, const char *uri,
                         const char *mrl, const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, mrl, ENNA_FILE_FILE);
}

Enna_File *
enna_browser_create_track(const char *name, const char *uri,
                         const char *mrl, const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, mrl, ENNA_FILE_TRACK);
}

Enna_File *
enna_browser_create_directory(const char *name, const char *uri,
                              const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, NULL, ENNA_FILE_DIRECTORY);
}

Enna_File *
enna_browser_create_menu(const char *name, const char *uri,
                         const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, NULL, ENNA_FILE_MENU);
}
