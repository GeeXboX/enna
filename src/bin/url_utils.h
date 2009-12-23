/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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

#ifndef URL_UTILS_H
#define URL_UTILS_H

typedef struct url_data_s
{
    int status;
    char *buffer;
    size_t size;
} url_data_t;

typedef void * url_t;

url_t url_new(void);

void url_free(url_t url);

void url_global_init(void);

void url_global_uninit(void);

url_data_t url_get_data(url_t handler, char *url);

char *url_escape_string(url_t handler, const char *buf);

void url_save_to_disk(url_t handler, char *src, char *dst);

#endif /* URL_UTILS_H */
