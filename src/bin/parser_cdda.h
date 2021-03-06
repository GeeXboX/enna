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

#ifndef PARSER_CDDA_H
#define PARSER_CDDA_H

typedef struct cdda_track_s {
  unsigned int min;
  unsigned int sec;
  unsigned int frame;
  char *name;
} cdda_track_t;

typedef struct cdda_s {
  unsigned long id;
  char *artist;
  char *title;
  char *ext_data;
  char *genre;
  int year;
  int length;
  unsigned int total_tracks;
  cdda_track_t **tracks;
} cdda_t;

cdda_t *cdda_parse (const char *device);
void cdda_free (cdda_t *c);

#endif /* PARSER_CDDA_H */
