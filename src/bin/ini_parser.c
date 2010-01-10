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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Eina.h>

#include "enna.h"
#include "ini_parser.h"
#include "buffer.h"
#include "utils.h"

#define BUFSIZE 1024

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static ini_field_t *
ini_field_new (const char *key, const char *value)
{
    ini_field_t *f;

    if (!key || !value)
        return NULL;

    f        = malloc(sizeof(ini_field_t));
    f->key   = strdup(key);
    f->value = strdup(value);

    return f;
}

static void
ini_field_free (ini_field_t *f)
{
    if (!f)
        return;

    ENNA_FREE(f->key);
    ENNA_FREE(f->value);
    ENNA_FREE(f);
}

static ini_section_t *
ini_section_new (const char *name)
{
    ini_section_t *s;

    if (!name)
        return NULL;

    s       = calloc(1, sizeof(ini_section_t));
    s->name = strdup(name);

    return s;
}

static void
ini_section_free (ini_section_t *s)
{
    ini_field_t *f;

    if (!s)
        return;

    EINA_LIST_FREE(s->fields, f)
        ini_field_free(f);

    ENNA_FREE(s->name);
    ENNA_FREE(s);
}

static void
ini_section_append_field (ini_section_t *s, ini_field_t *f)
{
    if (!s || !f)
        return;

    s->fields = eina_list_append(s->fields, f);
}

static void
ini_append_section (ini_t *ini, ini_section_t *s)
{
    if (!ini || !s)
        return;

    ini->current_section = s;
    ini->sections = eina_list_append(ini->sections, s);
}

static void
ini_dump_section (int fd, ini_section_t *s)
{
    Eina_List *l;
    ini_field_t *f;
    buffer_t *b;
    ssize_t n;

    if (!s)
        return;

    b = buffer_new();
    buffer_appendf(b, "[%s]\n", s->name);
    EINA_LIST_FOREACH(s->fields, l, f)
        buffer_appendf(b, "%s=%s\n", f->key, f->value);
    buffer_append(b, "\n");

    n = write(fd, b->buf, b->len);
    buffer_free(b);
}

static ini_field_t *
ini_get_field (ini_section_t *s, const char *key)
{
    Eina_List *l;
    ini_field_t *f;

    if (!s || !key)
        return NULL;

    EINA_LIST_FOREACH(s->fields, l, f)
        if (!strcmp(f->key, key))
            return f;

    return NULL;
}

static ini_section_t *
ini_get_section (ini_t *ini, const char *section)
{
    Eina_List *l;
    ini_section_t *s;

    if (!ini || !section)
        return NULL;

    EINA_LIST_FOREACH(ini->sections, l, s)
        if (!strcmp(s->name, section))
            return s;

    return NULL;
}

static Eina_List *
ini_get_tuple (ini_t *ini, const char *section, const char *key)
{
    Eina_List *tuple = NULL, *l;
    ini_section_t *s;
    ini_field_t *f;

    s = ini_get_section(ini, section);
    if (!s)
        return NULL;

    if (!key)
        return NULL;

    EINA_LIST_FOREACH(s->fields, l, f)
        if (!strcmp(f->key, key))
            tuple = eina_list_append(tuple, f);

    return tuple;
}

static const char *
ini_get_value (ini_t *ini, const char *section, const char *key)
{
    ini_section_t *s;
    ini_field_t *f;

    s = ini_get_section(ini, section);
    if (!s)
        return NULL;

    f = ini_get_field(s, key);
    if (!f)
        return NULL;

    return f->value;
}

static Eina_List *
ini_get_value_list(ini_t *ini, const char *section, const char *key)
{
    Eina_List *tuple, *l, *v = NULL;
    ini_field_t *f;

    tuple = ini_get_tuple(ini, section, key);
    if (!tuple)
        return NULL;

    EINA_LIST_FOREACH(tuple, l, f)
        v = eina_list_append(v, strdup(f->value));

    return v;
}

static void
ini_field_set_value (ini_field_t *f, const char *v)
{
    if (!f || !v)
        return;

    ENNA_FREE(f->value);
    f->value = strdup(v);
}

static void
ini_set_value (ini_t *ini, const char *section, const char *key, const char *v)
{
    ini_section_t *s;
    ini_field_t *f;

    s = ini_get_section(ini, section);
    if (!s)
        return;

    f = ini_get_field(s, key);
    if (f)
    {
        /* replace field value */
        ini_field_set_value(f, v);
    }
    else
    {
        /* add a new field */
        f = ini_field_new(key, v);
        ini_section_append_field(s, f);
    }
}

static void
ini_set_value_list (ini_t *ini, const char *section,
                    const char *key, Eina_List *values)
{
    Eina_List *l;
    ini_section_t *s;
    ini_field_t *f;
    const char *v;

    if (!values)
        return;

    s = ini_get_section(ini, section);
    if (!s)
        return;

    f = ini_get_field(s, key);
    if (f)
    {
        s->fields = eina_list_remove(s->fields, f);
        ini_field_free(f);
    }

    /* add a new fields */
    EINA_LIST_FOREACH(values, l, v)
    {
        f = ini_field_new(key, v);
        ini_section_append_field(s, f);
    }
}

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

ini_t *
ini_new (const char *file)
{
    ini_t *ini;

    if (!file)
        return NULL;

    ini       = calloc(1, sizeof (ini_t));
    ini->file = strdup(file);

    return ini;
}

void
ini_free (ini_t *ini)
{
    ini_section_t *s;

    if (!ini)
        return;

    EINA_LIST_FREE(ini->sections, s)
        ini_section_free(s);

    ENNA_FREE(ini->file);
    ENNA_FREE(ini);
}

void
ini_parse (ini_t *ini)
{
    FILE *f;
    char *c;

    if (!ini)
        return;

    f = fopen(ini->file, "r");
    if (!f)
        return;

    do {
        char buf[BUFSIZE];
        char *d;

        c = fgets(buf, BUFSIZE, f);
        if (!c)
            continue;

        /* discard useless lines */
        if (buf[0] == ' ' || buf[0] == '#')
            continue;

        /* check for new section */
        if (buf[0] == '[')
        {
            ini_section_t *se;
            char section[64];
            char *b, *e;
            int m;

            b = buf + 1;
            e = strstr(buf, "]");
            m = MMIN(sizeof(section), e - b + 1);
            snprintf(section, m, "%s", b);

            se = ini_section_new(section);
            ini_append_section(ini, se);
        }

        /* any other key=value */
        d = strstr(buf, "=");
        if (d)
        {
            char key[BUFSIZE], val[BUFSIZE];
            ini_field_t *f;
            int len;

            len = MMIN(BUFSIZE, d - buf + 1);
            snprintf(key, len, "%s", buf);
            len = MMIN(BUFSIZE, strlen(d) - 1);
            snprintf(val, len, "%s", d + 1);

            f = ini_field_new(key, val);
            ini_section_append_field(ini->current_section, f);
        }

    } while (c);


    fclose(f);
}

void
ini_dump (ini_t *ini)
{
    Eina_List *l;
    ini_section_t *s;
    int fd;

    if (!ini || !ini->sections)
        return;

    fd = open(ini->file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (fd == -1)
        return;

    EINA_LIST_FOREACH(ini->sections, l, s)
        ini_dump_section(fd, s);

    close(fd);
}

const char *
ini_get_string (ini_t *ini, const char *section, const char *key)
{
    return ini_get_value(ini, section, key);
}

Eina_List *
ini_get_string_list (ini_t *ini, const char *section, const char *key)
{
    return ini_get_value_list(ini, section, key);
}

int
ini_get_int (ini_t *ini, const char *section, const char *key)
{
    const char *s;

    s = ini_get_value(ini, section, key);
    return s ? atoi(s) : 0;
}

Eina_Bool
ini_get_bool (ini_t *ini, const char *section, const char *key)
{
    const char *s;

    s = ini_get_value(ini, section, key);
    if (!s)
        return EINA_FALSE;

    if (!strcmp(s, "true") || !strcmp(s, "TRUE") ||
        !strcmp(s, "yes")  || !strcmp(s, "yes"))
        return EINA_TRUE;

    return EINA_FALSE;
}

void
ini_set_string (ini_t *ini, const char *section,
                const char *key, const char *value)
{
    ini_set_value(ini, section, key, value);
}

void
ini_set_string_list (ini_t *ini, const char *section,
                     const char *key, Eina_List *values)
{
    ini_set_value_list(ini, section, key, values);
}

void
ini_set_int (ini_t *ini, const char *section, const char *key, int value)
{
    char v[64];

    snprintf(v, sizeof(v), "%d", value);
    ini_set_value(ini, section, key, v);
}

void
ini_set_bool (ini_t *ini, const char *section, const char *key, Eina_Bool b)
{
    ini_set_value(ini, section, key, (b == EINA_TRUE) ? "true" : "false");
}
