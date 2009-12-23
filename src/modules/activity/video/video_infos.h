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

#ifndef VIDEO_INFOS_H
#define VIDEO_INFOS_H

#include "enna.h"

Evas_Object *enna_panel_infos_add(Evas * evas);
void enna_panel_infos_set_text(Evas_Object *obj, Enna_Metadata *m);
void enna_panel_infos_set_cover(Evas_Object *obj, Enna_Metadata *m);
void enna_panel_infos_set_rating(Evas_Object *obj, Enna_Metadata *m);

#endif /* VIDEO_INFOS_H */
