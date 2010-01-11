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

#include <basedir.h>
#include <stdio.h>
#include <string.h>
#include <Ecore_File.h>

#include "xdg.h"

static xdgHandle xdg;
static Eina_Bool init_done = EINA_FALSE;

static const char *config_home;
static const char *data_home;
static const char *cache_home;

static const char * 
_makedir(const char * dir)
{
   size_t len = strlen (dir) + strlen("/enna") + 1;
   const char * ret = malloc (len);
   
   snprintf(ret, len, "%s/enna", dir);
   if (!ecore_file_is_dir(ret))
     ecore_file_mkpath(ret);
     
   return ret;
}

Eina_Bool 
enna_xdg_init(void)
{
  if (xdgInitHandle (&xdg))
    init_done = EINA_TRUE;

  return init_done;
}

void 
enna_xdg_shutdown(void)
{
  if (init_done == EINA_TRUE)
  {
    xdgWipeHandle (&xdg);
    free (config_home);
    free (data_home);
    free (cache_home);
  }
}

const char *
enna_config_home_get(void)
{
  if (init_done != EINA_TRUE)
    return NULL;

  if (!config_home)
    config_home = _makedir(xdgConfigHome(&xdg));

  return config_home;
}

const char *
enna_data_home_get(void)
{
  if (init_done != EINA_TRUE)
    return NULL;

  if (!data_home)
    data_home = _makedir(xdgDataHome(&xdg));
 
  return data_home;
}

const char *
enna_cache_home_get(void)
{
  if (init_done != EINA_TRUE)
    return NULL;

  if (!cache_home)
    cache_home = _makedir(xdgCacheHome(&xdg));
 
  return cache_home;
}
