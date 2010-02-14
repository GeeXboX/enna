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

#include <stdio.h>
#include <string.h>
#include <Ecore_File.h>

#include "utils.h"
#include "xdg.h"

static Eina_Bool init_done = EINA_FALSE;
static char *enna_home;

Eina_Bool
enna_xdg_init(void)
{
    size_t len;
    char *user_home = enna_util_user_home_get();

    if (!user_home)
        return EINA_FALSE;

    len = strlen(user_home) + strlen("/.enna") + 1;
    enna_home = malloc(len);
    snprintf(enna_home, len, "%s/.enna", user_home);

    if (!ecore_file_is_dir(enna_home))
        ecore_file_mkpath(enna_home);

    free(user_home);
    init_done = EINA_TRUE;

    return init_done;
}

void
enna_xdg_shutdown(void)
{
    free(enna_home);
}

const char *
enna_config_home_get(void)
{
    if (init_done != EINA_TRUE)
        return NULL;

    return enna_home;
}

const char *
enna_data_home_get(void)
{
    if (init_done != EINA_TRUE)
        return NULL;

    return enna_home;
}

const char *
enna_cache_home_get(void)
{
    if (init_done != EINA_TRUE)
        return NULL;

    return enna_home;
}
