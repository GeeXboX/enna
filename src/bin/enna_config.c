/*
 * enna_config.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna_config.c is free software copyrighted by Nicolas Aguirre.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Nicolas Aguirre'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * enna_config.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Nicolas Aguirre OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "enna.h"
#include "enna_config.h"

EAPI const char *
enna_config_theme_get()
{
   return enna_config->theme_filename;
}

EAPI void
enna_config_init()
{
   Evas_List *l;

   enna_config = calloc(1, sizeof(Enna_Config));
   /* Theme config */
   enna_config->theme_filename = evas_stringshare_add(PACKAGE_DATA_DIR"/enna/theme/default.edj");
   /* Module Music config */
   l = NULL;
   Enna_Config_Root_Directories *root;
   root = malloc(sizeof(Enna_Config_Root_Directories));
   root->uri = evas_stringshare_add("file:///home/nico");
   root->label = evas_stringshare_add("Home Direcory");
   l = evas_list_append(l, root);
   root = malloc(sizeof(Enna_Config_Root_Directories));
   root->uri = evas_stringshare_add("file:///media/serveur");
   root->label = evas_stringshare_add("Server");
   l = evas_list_append(l, root);
   root = malloc(sizeof(Enna_Config_Root_Directories));
   root->uri = evas_stringshare_add("file:///home/nico/music");
   root->label = evas_stringshare_add("Local Music");
   l = evas_list_append(l, root);
   enna_config->music_local_root_directories = l;
   l = NULL;
   l = evas_list_append(l, evas_stringshare_add("mp3"));
   l = evas_list_append(l, evas_stringshare_add("ogg"));
   l = evas_list_append(l, evas_stringshare_add("flac"));
   l = evas_list_append(l, evas_stringshare_add("wma"));
   l = evas_list_append(l, evas_stringshare_add("wav"));
   enna_config->music_filters = l;
}

EAPI void
enna_config_shutdown()
{

}


