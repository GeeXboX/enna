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

#ifndef MAINMENU_H
#define MAINMENU_H

#include "activity.h"


Evas_Object *enna_mainmenu_init(void);
void enna_mainmenu_shutdown(void);
void enna_mainmenu_append(Enna_Class_Activity *act);
void enna_mainmenu_show(void);
void enna_mainmenu_hide(void);
Eina_Bool enna_mainmenu_visible(void);
Eina_Bool enna_mainmenu_exit_visible(void);
Enna_Class_Activity *enna_mainmenu_selected_activity_get(void);
void enna_mainmenu_background_add(const char *name, const char *filename, const char *key);
void enna_mainmenu_background_select(const char *name);

#endif /* MAINMENU_H */
