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

#ifndef LOGS_H
#define LOGS_H

#include "enna.h"

int enna_log_init(const char *filename);
void enna_log_print(int level, const char *module, char *file, int line,
        const char *format, ...);
void enna_log_shutdown(void);

#define enna_log(level,module,fmt,arg...) \
        enna_log_print(level,module,__FILE__,__LINE__,fmt,##arg)

#endif /* LOGS_H */
