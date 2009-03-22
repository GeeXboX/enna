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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include "buffer.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define BUFFER_DEFAULT_CAPACITY 32768

buffer_t *
buffer_new (void)
{
    buffer_t *buffer = NULL;

    buffer = malloc (sizeof (buffer_t));
    buffer->buf = NULL;
    buffer->len = 0;
    buffer->capacity = 0;

    return buffer;
}

void
buffer_append (buffer_t *buffer, const char *str)
{
    size_t len;

    if (!buffer || !str)
        return;

    if (!buffer->buf)
    {
        buffer->capacity = BUFFER_DEFAULT_CAPACITY;
        buffer->buf = malloc (buffer->capacity);
        memset (buffer->buf, '\0', buffer->capacity);
    }

    len = buffer->len + strlen (str);
    if (len >= buffer->capacity)
    {
        buffer->capacity = MAX (len + 1, 2 * buffer->capacity);
        buffer->buf = realloc (buffer->buf, buffer->capacity);
    }

    strcat (buffer->buf, str);
    buffer->len += strlen (str);
}

void
buffer_appendf (buffer_t *buffer, const char *format, ...)
{
    char str[BUFFER_DEFAULT_CAPACITY];
    int size;
    va_list va;

    if (!buffer || !format)
        return;

    va_start (va, format);
    size = vsnprintf (str, BUFFER_DEFAULT_CAPACITY, format, va);
    if (size >= BUFFER_DEFAULT_CAPACITY)
    {
        char *dynstr = malloc (size + 1);
        vsnprintf (dynstr, size + 1, format, va);
        buffer_append (buffer, dynstr);
        free (dynstr);
    }
    else
        buffer_append (buffer, str);
    va_end (va);
}

void
buffer_free (buffer_t *buffer)
{
    if (!buffer)
        return;

    if (buffer->buf)
        free (buffer->buf);
    free (buffer);
}
