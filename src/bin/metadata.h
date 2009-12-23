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

#ifndef METADATA_H
#define METADATA_H

typedef void * Enna_Metadata;

void enna_metadata_init(void);
void enna_metadata_shutdown(void);
void *enna_metadata_get_db(void);

Enna_Metadata *enna_metadata_meta_new(const char *file);
char *enna_metadata_meta_get(Enna_Metadata *meta, const char *name, int max);
char *enna_metadata_meta_get_all(Enna_Metadata *meta);
void  enna_metadata_meta_free(Enna_Metadata *meta);
void enna_metadata_set_position(Enna_Metadata *meta, double position);
void enna_metadata_ondemand(const char *file);
char *enna_metadata_meta_duration_get(Enna_Metadata *m);

#endif /* METADATA_H */
