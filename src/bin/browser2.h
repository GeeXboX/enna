
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

#ifndef BROWSER2_H
#define BROWSER2_H

#include "vfs.h"

typedef struct _Enna_Browser Enna_Browser;

Enna_Browser *enna_browser2_add( void (*add)(Enna_Vfs_File *file, void *data), void *add_data,
                                 void (*del)(Enna_Vfs_File *file, void *data), void *del_data,
                                 const char *uri);
void enna_browser2_del(Enna_Browser *b);
Enna_Vfs_File *enna_browser2_get_file(const char *uri);
const char *enna_browser2_uri_get(Enna_Browser *b);
void enna_browser2_browse_root(Enna_Browser *browser);
#endif /* BROWSER2_H */
