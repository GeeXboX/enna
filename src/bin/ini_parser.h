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

#ifndef INI_PARSER_H
#define INI_PARSER_H

typedef struct ini_field_s {
    char *key;
    char *value;
} ini_field_t;

typedef struct ini_section_s {
    char *name;
    Eina_List *fields;
} ini_section_t;

typedef struct ini_s {
    char *file;
    ini_section_t *current_section;
    Eina_List *sections;
} ini_t;

/* (de)allocation */
ini_t * ini_new  (const char *file);
void    ini_free (ini_t *ini);

/* read/write */
void ini_parse (ini_t *ini);
void ini_dump  (ini_t *ini);

/* getters */
const char * ini_get_string      (ini_t *ini, const char *section,
                                  const char *key);
Eina_List *  ini_get_string_list (ini_t *ini, const char *section,
                                  const char *key);
int ini_get_int                  (ini_t *ini, const char *section,
                                  const char *key);
Eina_Bool ini_get_bool           (ini_t *ini, const char *section,
                                  const char *key);

/* setters */
void ini_set_string      (ini_t *ini, const char *section,
                          const char *key, const char *value);
void ini_set_string_list (ini_t *ini, const char *section,
                          const char *key, Eina_List *values);
void ini_set_int         (ini_t *ini, const char *section,
                          const char *key, int value);
void ini_set_bool        (ini_t *ini, const char *section,
                          const char *key, Eina_Bool b);

#endif /* INI_PARSER_H */
