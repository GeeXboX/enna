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

#ifndef KBDNAV_H
#define KBDNAV_H

#include <Evas.h>

#include "enna.h"
#include "input.h"

typedef struct _Enna_Kbdnav Enna_Kbdnav;
typedef struct _Enna_Kbdnav_Class Enna_Kbdnav_Class;

struct _Enna_Kbdnav_Class
{
    const Evas_Object *(*object_get)(void *item_data, void *user_data);
    void  (*select_set)(void *item_data, void *user_data);
    void  (*activate_set)(void *item_data, void *user_data);
};

Enna_Kbdnav *enna_kbdnav_add();
void enna_kbdnav_del(Enna_Kbdnav *nav);
void enna_kbdnav_item_add(Enna_Kbdnav *nav, void *obj, Enna_Kbdnav_Class *class, void *user_data);
void enna_kbdnav_item_del(Enna_Kbdnav *nav, void *obj);

Eina_Bool enna_kbdnav_current_set(Enna_Kbdnav *nav, void *obj);
void *enna_kbdnav_current_get(Enna_Kbdnav *nav);
/* Eina_Bool enna_kbdnav_current_select(Enna_Kbdnav *nav); */
/* Eina_Bool enna_kbdnav_current_unselect(Enna_Kbdnav *nav); */
/* Eina_Bool enna_kbdnav_current_activate(Enna_Kbdnav *nav); */

Eina_Bool enna_kbdnav_up(Enna_Kbdnav *nav);
Eina_Bool enna_kbdnav_down(Enna_Kbdnav *nav);
Eina_Bool enna_kbdnav_left(Enna_Kbdnav *nav);
Eina_Bool enna_kbdnav_right(Enna_Kbdnav *nav);
void enna_kbdnav_activate(Enna_Kbdnav *nav);


#endif /* KBDNAV_H */
