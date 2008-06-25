/*
 * enna_covers.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna_covers.c is free software copyrighted by Nicolas Aguirre.
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
 * enna_covers.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
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


EAPI char *enna_cover_album_get(const char *artist, const char *album, const char *filename)
{

   const char *cover_names[] = {"cover.png",
				    "cover.jpg",
				    "front.png",
				    "front.jpg"};
   char tmp[4096];
   char *cover_file;
   int i;

   if (!artist || !album)
     return NULL;

   if (filename)
     {

	const char *file_dir = ecore_file_dir_get(filename);
	if (ecore_file_can_read(file_dir))
	  {
	     for(i = 0; i < 4; i++)
	       {
		  snprintf(tmp, sizeof(tmp), "%s/%s", file_dir, cover_names[i]);
		  if (ecore_file_exists(tmp))
		    {
		       cover_file = strdup(tmp);
		       printf("found : %s\n", cover_file);
		       return cover_file;
		    }
	       }
	     snprintf(tmp, sizeof(tmp), "%s/.enna/covers//%s-%s", enna_util_user_home_get(), artist, album);
	     if (ecore_file_exists(tmp))
	       {
		  cover_file = strdup(tmp);
		  printf("found : %s\n", cover_file);
		  return cover_file;
	       }
	  }
	else return NULL;

     }
   else
     {
	/* Search only in home directory */
	snprintf(tmp, sizeof(tmp), "%s/.enna/covers/%s-%s.png", enna_util_user_home_get(), artist, album);
	if (ecore_file_exists(tmp))
	  {
	     cover_file = strdup(tmp);
	     printf("cover art found : %s\n", cover_file);
	     return cover_file;
	  }
     }
   return NULL;

}
