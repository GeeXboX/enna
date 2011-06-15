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
//static int refcount = 0;

int
enna_log_init(const char *filename)
{
//    if (refcount > 1)
//        return 0;

    if (filename)
    {
        fp = fopen(filename, "w");
        if (!fp)
            return 0;
    }

//    refcount++;
    return 1;
}

void
enna_log_print(int level, const char *module,
               char *file, int line, const char *format, ...)
{
    FILE *f;
    static const char *const c[] =
    {
        [ENNA_MSG_EVENT]    = F_BLUE,
        [ENNA_MSG_INFO]     = F_GREEN,
        [ENNA_MSG_WARNING]  = F_YELLOW,
        [ENNA_MSG_ERROR]    = F_RED,
        [ENNA_MSG_CRITICAL] = B_RED,
    };

    static const char *const l[] =
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

    int n;
    if (!fp)
    {
        f = stderr;
        n = fprintf (f, "[" BOLD "%s%s" NORMAL "] [%s:%d] %s%s" NORMAL ": ",
            prefix ? prefix : "", module, file, line, c[level], l[level]);
    }
    else
    {
        f = fp;
        n = fprintf (f, "[%s%s] [%s:%d] %s: ",
            prefix ? prefix : "", module, file, line, l[level]);
    }

    vfprintf (f, format, va);
    fprintf (f, "\n");
    va_end (va);
}

void
enna_log_shutdown(void)
{
//    if (refcount > 0) refcount--;

//    if (refcount)
//	return;
//    // refcount == 0

    if (fp)
    {
        fclose(fp);
    }
}
