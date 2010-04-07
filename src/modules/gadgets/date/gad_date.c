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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "weather_api.h"
#include "gadgets.h"
#include "module.h"

#define ENNA_MODULE_NAME "gadget_date"


typedef struct Smart_Data
{
    Evas_Object *obj;
}Smart_Data;

static Smart_Data *sd = NULL;


static Evas_Object *
_date_add()
{
    sd->obj = evas_object_rectangle_add(enna->evas);
    evas_object_color_set(sd->obj, 128, 128, 255, 128);

    return sd->obj;
}

static void
_date_del()
{
    ENNA_OBJECT_DEL(sd->obj);
}

static Enna_Gadget gadget =
{
    _date_add,
    _date_del,

};


/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_gadget_date
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    sd = ENNA_NEW(Smart_Data, 1);

    enna_gadgets_register(&gadget);
}

static void
module_shutdown(Enna_Module *em)
{
 
    ENNA_FREE(sd);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME,
    N_("Date Gadget"),
    NULL,
    N_("Module to show date on the desktop"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
