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

EAPI char          *
enna_config_theme_get()
{
   return enna_config->theme_filename;
}

/* EAPI void */
/* enna_config_theme_set(char *theme_name) */
/* { */
/*    char                tmp[4096]; */

/*    if (!theme_filename) */
/*      { */
/* 	if (theme_name) */
/* 	  { */
/* 	     sprintf(tmp, PACKAGE_DATA_DIR "/enna/theme/%s.edj", theme_name); */
/* 	     if (!ecore_file_exists(tmp)) */
/* 	       { */
/* 		  char               *theme; */

/* 		  theme = */
/* 		     enna_config_get_conf_value_or_default("theme", "name", */
/* 							   "default"); */
/* 		  sprintf(tmp, PACKAGE_DATA_DIR "/enna/theme/%s.edj", theme); */
/* 		  if (!ecore_file_exists(tmp)) */
/* 		    { */
/* 		       theme_filename = */
/* 			  strdup(PACKAGE_DATA_DIR "/enna/theme/default.edj"); */
/* 		       dbg("ERROR : theme define in enna.cfg (%s) does not exists on your system (%s)!\n", theme, tmp); */
/* 		       dbg("ERROR : Default theme is used instead.\n"); */
/* 		    } */
/* 		  else */
/* 		     theme_filename = strdup(tmp); */
/* 	       } */
/* 	     else */
/* 		theme_filename = strdup(tmp); */
/* 	  } */
/* 	else */
/* 	  { */
/* 	     char               *theme; */

/* 	     theme = */
/* 		enna_config_get_conf_value_or_default("theme", "name", */
/* 						      "default"); */
/* 	     sprintf(tmp, PACKAGE_DATA_DIR "/enna/theme/%s.edj", theme); */
/* 	     if (!ecore_file_exists(tmp)) */
/* 	       { */
/* 		  theme_filename = */
/* 		     strdup(PACKAGE_DATA_DIR "/enna/theme/default.edj"); */
/* 		  dbg("ERROR : theme define in enna.cfg (%s) does not exists on your system (%s)!\n", theme, tmp); */
/* 		  dbg("ERROR : Default theme is used instead.\n"); */
/* 	       } */
/* 	     else */
/* 		theme_filename = strdup(tmp); */
/* 	  } */
/*      } */
/*    else */
/*       dbg("Warning try to define new theme, but another theme is already in use : %s\n", theme_filename); */

/*    dbg("Using theme : %s\n", theme_filename); */

/* } */

/* EAPI Evas_List     * */
/* enna_config_theme_available_get(void) */
/* { */
/*    return NULL; */
/* } */

/* EAPI Evas_List     * */
/* enna_config_extensions_get(char *type) */
/* { */

/*    if (!type) */
/*       return NULL; */

/*    if (!strcmp(type, "music")) */
/*       return config.music_extensions; */
/*    else if (!strcmp(type, "video")) */
/*       return config.video_extensions; */
/*    else if (!strcmp(type, "radio")) */
/*       return config.radio_extensions; */
/*    else if (!strcmp(type, "photo")) */
/*       return config.photo_extensions; */
/*    else */
/*       return NULL; */
/* } */

/* EAPI void */
/* enna_config_extensions_set(char *type, Evas_List * ext) */
/* { */
/* } */

/* static struct conf_section * */
/* enna_config_load_conf(char *conffile, int size) */
/* { */
/*    struct conf_section *current_section = NULL; */
/*    struct conf_section *sections = NULL; */
/*    char               *current_line = conffile; */

/*    while (current_line < conffile + size) */
/*      { */
/* 	char               *eol = strchr(current_line, '\n'); */

/* 	if (eol) */
/* 	   *eol = 0; */
/* 	else			// Maybe the end of file */
/* 	   eol = conffile + size; */

/* 	// Removing the leading spaces */
/* 	while (*current_line && *current_line == ' ') */
/* 	   current_line++; */

/* 	// Maybe an empty line */
/* 	if (!(*current_line)) */
/* 	  { */
/* 	     current_line = eol + 1; */
/* 	     continue; */
/* 	  } */

/* 	// Maybe a comment line */
/* 	if (*current_line == '#') */
/* 	  { */
/* 	     current_line = eol + 1; */
/* 	     continue; */
/* 	  } */

/* 	// We are at a section definition */
/* 	if (*current_line == '[') */
/* 	  { */
/* 	     // ']' must be the last char of this line */
/* 	     char               *end_of_section_name = */
/* 		strchr(current_line + 1, ']'); */
/* 	     if (end_of_section_name[1] != 0) */
/* 	       { */
/* 		  dbg("malformed section name %s\n", current_line); */
/* 		  return NULL; */
/* 	       } */
/* 	     current_line++; */
/* 	     *end_of_section_name = '\0'; */

/* 	     // Building the section */
/* 	     current_section = malloc(sizeof(*current_section)); */
/* 	     current_section->section_name = strdup(current_line); */
/* 	     current_section->values = NULL; */
/* 	     current_section->next_section = NULL; */
/* 	     if (sections) */
/* 	       { */
/* 		  current_section->next_section = sections; */
/* 		  sections = current_section; */
/* 	       } */
/* 	     else */
/* 		sections = current_section; */

/* 	     current_line = eol + 1; */
/* 	     continue; */

/* 	  } */

/* 	// Must be in a section to provide a key/value pair */
/* 	if (!current_section) */
/* 	  { */
/* 	     dbg("No section for this line %s\n", current_line); */
/* 	     return NULL; */
/* 	  } */

/* 	// Building the key/value string pair */
/* 	char               *key = current_line; */
/* 	char               *value = strchr(current_line, '='); */

/* 	if (!value) */
/* 	  { */
/* 	     dbg("Malformed line %s\n", current_line); */
/* 	     return NULL; */
/* 	  } */
/* 	*value = '\0'; */
/* 	value++; */

/* 	// Building the key/value pair */
/* 	struct conf_pair   *newpair = malloc(sizeof(*newpair)); */

/* 	newpair->key = strdup(key); */
/* 	newpair->value = strdup(value); */
/* 	newpair->next_pair = current_section->values; */
/* 	current_section->values = newpair; */

/* 	current_line = eol + 1; */
/*      } */
/*    free(conffile); */

/*    return sections; */
/* } */

/* static struct conf_section * */
/* enna_config_load_conf_file(char *filename) */
/* { */
/*    int                 fd; */
/*    FILE               *f; */
/*    struct stat         st; */
/*    char                tmp[4096]; */

/*    if (stat(filename, &st)) */
/*      { */
/* 	dbg("Cannot stat file %s\n", filename); */
/* 	sprintf(tmp, "%s/.enna", enna_util_user_home_get()); */
/* 	if (!ecore_file_is_dir(tmp)) */
/* 	   ecore_file_mkdir(tmp); */

/* 	if (!(f = fopen(filename, "w"))) */
/* 	   return NULL; */
/* 	else */
/* 	  { */
/* 	     fprintf(f, "[tv_module]\n" */
/* 		     "used=0\n\n" */
/* 		     "[video_module]\n" */
/* 		     "base_path=%s\n" */
/* 		     "used=1\n\n" */
/* 		     "[music_module]\n" */
/* 		     "base_path=%s\n" */
/* 		     "used=1\n\n" */
/* 		     "[photo_module]\n" */
/* 		     "base_path=%s\n" */
/* 		     "used=1\n\n" */
/* 		     "[playlist_module]\n" */
/* 		     "base_path=%s\n" */
/* 		     "used=1\n\n" */
/* 		     "[theme]\n" */
/* 		     "name=default\n", */
/* 		     enna_util_user_home_get(), */
/* 		     enna_util_user_home_get(), */
/* 		     enna_util_user_home_get(), enna_util_user_home_get()); */
/* 	     fclose(f); */
/* 	  } */

/*      } */
/*    if (stat(filename, &st)) */
/*      { */
/* 	dbg("Cannot stat file %s\n", filename); */
/* 	return NULL; */
/*      } */

/*    char               *conffile = malloc(st.st_size); */

/*    if (!conffile) */
/*      { */
/* 	dbg("Cannot malloc %d bytes\n", (int)st.st_size); */
/* 	return NULL; */
/*      } */

/*    if ((fd = open(filename, O_RDONLY)) < 0) */
/*      { */
/* 	dbg("Cannot open file\n"); */
/* 	return NULL; */
/*      } */

/*    int                 ret = read(fd, conffile, st.st_size); */

/*    if (ret != st.st_size) */
/*      { */
/* 	dbg("Cannot read conf file entirely, read only %d bytes\n", ret); */
/* 	return NULL; */
/*      } */

/*    return enna_config_load_conf(conffile, st.st_size); */
/* } */

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


