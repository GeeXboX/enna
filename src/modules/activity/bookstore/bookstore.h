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

#ifndef BOOKSTORE_H
#define BOOKSTORE_H

typedef struct _Bookstore_Service {
    const char *label;
    const char *bg;
    const char *icon;
    void (*show)(Evas_Object *edje);
    void (*hide)(Evas_Object *edje);
    Eina_Bool (*event) (Evas_Object *edje, enna_input event);
    void (*prev)(void *data, Evas_Object *obj, void *ev);
    void (*next)(void *data, Evas_Object *obj, void *ev);
} Bookstore_Service;

void bs_service_page_show (const char *file);

#endif /* BOOKSTORE_H */
