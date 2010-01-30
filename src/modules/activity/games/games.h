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

#ifndef GAMES_H
#define GAMES_H

typedef struct _Games_Service {
    const char *label;
    const char *bg;
    const char *icon;
    Eina_Bool (*init)(void);
    Eina_Bool (*shutdown)(void);
    void (*show)(Evas_Object *edje);
    void (*hide)(Evas_Object *edje);
    Eina_Bool (*event) (Evas_Object *edje, enna_input event);
} Games_Service;


void games_service_image_show(const char *file);
void games_service_title_show(const char *title);
void games_service_total_show(int tot);
void games_service_list_show(Evas_Object *list);
void games_service_exec(const char *cmd, const char *text);


Evas_Object *enna_util_message_show(const char *text);

#endif /* GAMES_H */
