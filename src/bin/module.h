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

#ifndef MODULE_H
#define MODULE_H

#include <Ecore_Data.h>

#include "enna.h"

#define ENNA_MODULE_VERSION 3

#define MOD_PREFIX module /* default name for dynamic linking */
#define MOD_APPEND_API(prefix)           prefix##_api
#define MOD_APPEND_INIT(prefix)          prefix##_init
#define MOD_APPEND_SHUTDOWN(prefix)      prefix##_shutdown
#define ENNA_MOD_PREFIX_API(prefix)      MOD_APPEND_API(prefix)
#define ENNA_MOD_PREFIX_INIT(prefix)     MOD_APPEND_INIT(prefix)
#define ENNA_MOD_PREFIX_SHUTDOWN(prefix) MOD_APPEND_SHUTDOWN(prefix)
/* Entries to use in the modules. */
#define ENNA_MODULE_API                  ENNA_MOD_PREFIX_API(MOD_PREFIX)
#define ENNA_MODULE_INIT                 ENNA_MOD_PREFIX_INIT(MOD_PREFIX)
#define ENNA_MODULE_SHUTDOWN             ENNA_MOD_PREFIX_SHUTDOWN(MOD_PREFIX)

typedef struct _Enna_Module Enna_Module;
typedef struct _Enna_Module_Api Enna_Module_Api;

struct _Enna_Module
{
    const char *name;
    struct
    {
        void (*init)(Enna_Module *m);
        void (*shutdown)(Enna_Module *m);
    } func;

    Enna_Module_Api *api;
    unsigned char enabled;
    Ecore_Plugin *plugin;
    void *mod;
};

struct _Enna_Module_Api
{
    int version;
    const char *name;
    const char *title;
    const char *icon;
    const char *short_desc;
    const char *long_desc;
};

int          enna_module_init(void);
void         enna_module_shutdown(void);
void         enna_module_load_all(void);
Enna_Module *enna_module_open(const char *name);
int          enna_module_enable(Enna_Module *m);
int          enna_module_disable(Enna_Module *m);

#endif /* MODULE_H */
