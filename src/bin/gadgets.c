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

#include "gadgets.h"

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "box.h"

#define DEFAULT_NB_ROWS 4
#define DEFAULT_NB_COLUMNS 4
#define DEFAULT_PADDING 4

typedef struct _Smart_Data Smart_Data;

static int _gadgets_init_count = -1;
static Eina_List *_gadgets;

void
enna_gadgets_register(Enna_Gadget *gad)
{
    _gadgets = eina_list_append(_gadgets, gad);
    enna_gadgets_show();
}

void
enna_gadgets_show()
{
    Enna_Gadget *gad;
    Eina_List *l;

    EINA_LIST_FOREACH(_gadgets, l, gad)
    {
        if (gad && gad->add)
        {
            gad->add(enna->layout);
        }
    }
}

void
enna_gadgets_hide()
{
    Enna_Gadget *gad;
    Eina_List *l;

    EINA_LIST_FOREACH(_gadgets, l, gad)
    {
        /* if (gad && gad->del) */
        /*     gad->del(); */
    }

}

int
enna_gadgets_init()
{
    /* Prevent multiple loads */
    if (_gadgets_init_count > 0)
        return ++_gadgets_init_count;

    _gadgets_init_count = 1;
    return 1;
}

int
enna_gadgets_shutdown()
{
    _gadgets_init_count--;
    if (_gadgets_init_count == 0)
    {
        enna_gadgets_hide();
        eina_list_free(_gadgets);
    }

    return _gadgets_init_count;
}

