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
#include "metadata.h"
#include "logs.h"

typedef struct _Enna_File_Callback Enna_File_Callback;
struct _Enna_File_Callback
{
   void (*func) (void *data, Enna_File *file);
   void *func_data;
};

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
    f->refcount++;

    return f;
}

const char *
_meta_get_default(Enna_File *file, const char *key)
{
    Enna_Metadata *m;
    const char *str;

    m = enna_metadata_meta_new(file->mrl);
    str = enna_metadata_meta_get(m, key, 0);
    enna_metadata_meta_free(m);
    return str;

}

void
_meta_default_set(Enna_File *file, const char *key, const char *data)
{
    Enna_Metadata *m;

    m = enna_metadata_meta_new(file->mrl);
    enna_metadata_meta_set(m, file, key, data);
    enna_metadata_meta_free(m);
    return;

}

Enna_File *
enna_file_dup(Enna_File *file)
{
    Enna_File *f;

    Eina_List *l;
    Enna_File_Callback *cb;
    Enna_File_Callback *cb_cp;

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
    f->callbacks = eina_list_clone(file->callbacks);

    EINA_LIST_FOREACH(file->callbacks, l, cb)
    {
        cb_cp = calloc(1, sizeof(Enna_File_Callback));
        cb_cp->func = cb->func;
        cb_cp->func_data = cb->func_data;
        f->callbacks = eina_list_append(f->callbacks, cb_cp);
    }
    f->refcount++;

    return f;
}

Enna_File *
enna_file_ref(Enna_File *file)
{
    if (!file)
        return NULL;

    file->refcount++;

    return file;
}

void
enna_file_free(Enna_File *f)
{
    Enna_File_Callback *cb;

    if (!f)
        return;

    f->refcount--;

    if (f->refcount > 0)
    {
        return;
    }

    if (f->name) eina_stringshare_del(f->name);
    if (f->uri) eina_stringshare_del(f->uri);
    if (f->label) eina_stringshare_del(f->label);
    if (f->icon) eina_stringshare_del(f->icon);
    if (f->icon_file) eina_stringshare_del(f->icon_file);
    if (f->mrl) eina_stringshare_del(f->mrl);
    if (f->meta_class && f->meta_class->meta_del)
        f->meta_class->meta_del(f->meta_data);
    if (f->callbacks)
        EINA_LIST_FREE(f->callbacks, cb)
            free(cb);

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
enna_file_meta_get(Enna_File *f, const char *key)
{
    if (!f || !key)
        return NULL;

    if (!f->meta_class || !f->meta_class->meta_get)
        return _meta_get_default(f, key);

    return f->meta_class->meta_get(f->meta_data, f, key);
}

void
enna_file_meta_set(Enna_File *f, const char *key, const void *data)
{
    if (!f || !key || !data)
        return;

    if (!f->meta_class || !f->meta_class->meta_get)
        _meta_default_set(f, key, data);

    f->meta_class->meta_set(f->meta_data, f, key, data);
}

Enna_File *
enna_file_file_add(const char *name, const char *uri,
                   const char *mrl, const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, mrl, ENNA_FILE_FILE);
}

Enna_File *
enna_file_track_add(const char *name, const char *uri,
                    const char *mrl, const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, mrl, ENNA_FILE_TRACK);
}

Enna_File *
enna_file_film_add(const char *name, const char *uri,
                    const char *mrl, const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, mrl, ENNA_FILE_FILM);
}

Enna_File *
enna_file_directory_add(const char *name, const char *uri,
                        const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, NULL, ENNA_FILE_DIRECTORY);
}

Enna_File *
enna_file_menu_add(const char *name, const char *uri,
                   const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, NULL, ENNA_FILE_MENU);
}

Enna_File *
enna_file_volume_add(const char *name, const char *uri,
                     const char *label, const char *icon)
{
    return _create_inode(name, uri, label, icon, NULL, ENNA_FILE_VOLUME);
}

void
enna_file_meta_callback_add(Enna_File *file, Enna_File_Update_Cb func, void *data)
{
    Enna_File_Callback *cb;

    if (!file || !func)
        return;

    cb = calloc(1, sizeof(Enna_File_Callback));
    cb->func = func;
    cb->func_data = data;
    file->callbacks = eina_list_prepend(file->callbacks, cb);
    if (!file->meta_class)
        enna_metadata_ondemand_add(file);
}

void *
enna_file_meta_callback_del(Enna_File *file, Enna_File_Update_Cb func)
{
    Eina_List *l;
    Enna_File_Callback *cb;

    if (!file || !func)
        return NULL;

    EINA_LIST_FOREACH(file->callbacks, l, cb)
    {
        if (cb->func == func)
        {
            void *data;

            data = cb->func_data;
            file->callbacks = eina_list_remove(file->callbacks, cb);
            free(cb);
            return data;
        }
    }

    if (!file->meta_class)
        enna_metadata_ondemand_del(file);

    return NULL;
}

void
enna_file_meta_callback_call(Enna_File *file)
{
    Enna_File_Callback *cb;
    Eina_List *l;

    if (!file)
        return;

    EINA_LIST_FOREACH(file->callbacks, l, cb)
    {
        cb->func(cb->func_data, file);
    }
}
