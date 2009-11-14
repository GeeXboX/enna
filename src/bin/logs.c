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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "enna.h"

#define DEFAULT_MODULE_NAME "Enna"

#define NORMAL   "\033[0m"
#define COLOR(x) "\033[" #x ";1m"
#define BOLD     COLOR(1)
#define F_RED    COLOR(31)
#define F_GREEN  COLOR(32)
#define F_YELLOW COLOR(33)
#define F_BLUE   COLOR(34)
#define B_RED    COLOR(41)


static FILE *fp = NULL;
static int refcount = 0;

int
enna_log_init(const char *filename)
{

    if (refcount > 1)
        return 0;

    if (filename)
    {
        fp = fopen(filename, "w");
        if (!fp)
            return 0;
    }

    refcount++;
    return 1;
}

void
enna_log_print(int level, const char *module,
               char *file, int line, const char *format, ...)
{
    FILE *f;
    static const char const *c[] =
    {
        [ENNA_MSG_EVENT]    = F_BLUE,
        [ENNA_MSG_INFO]     = F_GREEN,
        [ENNA_MSG_WARNING]  = F_YELLOW,
        [ENNA_MSG_ERROR]    = F_RED,
        [ENNA_MSG_CRITICAL] = B_RED,
    };

    static const char const *l[] =
    {
        [ENNA_MSG_EVENT]    = "Event",
        [ENNA_MSG_INFO]     = "Info",
        [ENNA_MSG_WARNING]  = "Warn",
        [ENNA_MSG_ERROR]    = "Err",
        [ENNA_MSG_CRITICAL] = "Crit",
    };

    va_list va;
    int verbosity;
    const char *prefix = NULL;

    if (!format)
        return;

    if (enna) verbosity = enna->lvl;
    else verbosity = ENNA_MSG_INFO;

    /* do we really want loging ? */
    if (verbosity == ENNA_MSG_NONE)
        return;

    if (level < verbosity)
        return;

    va_start (va, format);

    if (!module)
        module = DEFAULT_MODULE_NAME;
    else
        prefix = DEFAULT_MODULE_NAME "/";

    if (!fp)
    {
        f = stderr;
        fprintf (f, "[" BOLD "%s%s" NORMAL "] [%s:%d] %s%s" NORMAL ": ",
            prefix ? prefix : "", module, file, line, c[level], l[level]);
    }
    else
    {
        f = fp;
        fprintf (f, "[%s%s] [%s:%d] %s: ",
            prefix ? prefix : "", module, file, line, l[level]);
    }

    vfprintf (f, format, va);
    fprintf (f, "\n");
    va_end (va);
}

void
enna_log_shutdown(void)
{
    if (fp)
    {
        fclose(fp);
    }
    refcount--;
}
