
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

#ifndef BROWSER_H
#define BROWSER_H


typedef struct _Enna_Browser Enna_Browser;
typedef struct _Enna_File Enna_File;
typedef enum _Enna_File_Type Enna_File_Type;
typedef struct _Enna_File_Meta_Class Enna_File_Meta_Class;


struct _Enna_File_Meta_Class
{
    const char *(* meta_get)(void *data, Enna_File *file, const char *key);
    void  (* meta_del)(void *data);
};


#define ENNA_FILE_IS_BROWSABLE(file) \
    ((file->type == ENNA_FILE_MENU) || (file->type == ENNA_FILE_DIRECTORY) || (file->type == ENNA_FILE_VOLUME))

enum _Enna_File_Type
{
    ENNA_FILE_MENU,
    ENNA_FILE_DIRECTORY,
    ENNA_FILE_VOLUME,
    ENNA_FILE_FILE,
    ENNA_FILE_ARTIST,
    ENNA_FILE_ALBUM,
    ENNA_FILE_TRACK,
    ENNA_FILE_SERIE,
    ENNA_FILE_SERIE_EPISODE,
    ENNA_FILE_FILM
};

struct _Enna_File
{
    const char *name;
    const char *uri;
    const char *label;
    const char *icon;
    const char *icon_file;
    const char *mrl;
    Enna_File_Type type;
    Enna_File_Meta_Class *meta_class;
    void *meta_data;
};

Enna_Browser *enna_browser_add(void (*add)(void *data, Enna_File *file), void *add_data,
                               void (*del)(void *data, Enna_File *file), void *del_data,
                               void (*update)(void *data, Enna_File *file), void *update_data,
                               const char *uri);
void enna_browser_browse(Enna_Browser *b);
void enna_browser_del(Enna_Browser *b);
void enna_browser_file_add(Enna_Browser *b, Enna_File *file);
void enna_browser_file_del(Enna_Browser *b, Enna_File *file);
Enna_File *enna_browser_file_update(Enna_Browser *b, Enna_File *file);
Enna_File *enna_browser_get_file(const char *uri);
const char *enna_browser_uri_get(Enna_Browser *b);
Eina_List *enna_browser_files_get(Enna_Browser *b);
int enna_browser_level_get(Enna_Browser *b);
Enna_File *enna_browser_file_dup(Enna_File *file);
void enna_browser_file_free(Enna_File *f);
void enna_browser_file_meta_add(Enna_File *f, Enna_File_Meta_Class *meta_class, void *data);
const char * enna_browser_file_meta_get(Enna_File *f, const char *key);

Enna_File *enna_browser_create_file(const char *name, const char *uri,
                                    const char *mrl, const char *label,
                                    const char *icon);
Enna_File *enna_browser_create_track(const char *name, const char *uri,
                                    const char *mrl, const char *label,
                                    const char *icon);
Enna_File *enna_browser_create_directory(const char *name, const char *uri,
                                         const char *label, const char *icon);
Enna_File *enna_browser_create_menu(const char *name, const char *uri,
                                    const char *label, const char *icon);
void enna_browser_filter(Enna_Browser *b, const char *filter);
#endif /* BROWSER_H */
