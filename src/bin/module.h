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

#ifndef MODULE_H
#define MODULE_H

#include <Ecore_Data.h>

#include "enna.h"

#define ENNA_MODULE_VERSION 1

typedef struct _Enna_Module Enna_Module;
typedef struct _Enna_Module_Api Enna_Module_Api;

typedef enum {
    ENNA_MODULE_UNKNOWN,
    ENNA_MODULE_ACTIVITY,
    ENNA_MODULE_BACKEND,
    ENNA_MODULE_BROWSER,
    ENNA_MODULE_METADATA,
    ENNA_MODULE_VOLUME,
    ENNA_MODULE_INPUT,
} _Enna_Module_Type;

struct _Enna_Module
{
    const char *name;
    struct
    {
        void * (*init)(Enna_Module *m);
        int (*shutdown)(Enna_Module *m);
    } func;

    Enna_Module_Api *api;
    unsigned char enabled;
    Ecore_Plugin *plugin;
    Evas *evas;
    void *mod;
};

struct _Enna_Module_Api
{
    int version;
    _Enna_Module_Type type;
    const char *name;
};

int enna_module_init(void);
int enna_module_shutdown(void);
void enna_module_load_all (Evas *evas);
Enna_Module *enna_module_open(const char *name, _Enna_Module_Type type, Evas *evas);
int enna_module_enable(Enna_Module *m);
int enna_module_disable(Enna_Module *m);

#endif /* MODULE_H */
