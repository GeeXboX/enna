/*
 * enna_vfs.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna_vfs.c is free software copyrighted by Nicolas Aguirre.
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
 * enna_vfs.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
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
#include "vfs.h"



static Evas_List *_enna_vfs_music = NULL;
static Evas_List *_enna_vfs_video = NULL;
static Evas_List *_enna_vfs_photo = NULL;

/* local subsystem functions */



/* externally accessible functions */
EAPI int
enna_vfs_init(Evas *evas)
{

   return 0;
}

EAPI int
enna_vfs_append(const char *name, unsigned char type, Enna_Class_Vfs *vfs)
{
   if (!vfs) return -1;

   if (type |= ENNA_CAPS_MUSIC)
     {
	printf("CAPS Music\n");
	_enna_vfs_music = evas_list_append(_enna_vfs_music, vfs);
     }
   if (type |= ENNA_CAPS_VIDEO)
     {
	printf("CAPS Video\n");
	_enna_vfs_video = evas_list_append(_enna_vfs_video, vfs);
     }
   if (type |= ENNA_CAPS_PHOTO)
     {
	printf("CAPS Photo\n");
	_enna_vfs_photo = evas_list_append(_enna_vfs_photo, vfs);
     }

   return 0;
}

EAPI Evas_List *
enna_vfs_get(ENNA_VFS_CAPS type)
{

  if (type == ENNA_CAPS_MUSIC)
    {
       printf("Get Music caps\n");
       return _enna_vfs_music;
    }
  else if (type == ENNA_CAPS_VIDEO)
    {
       printf(" Get Video Caps\n");
       return _enna_vfs_video;
    }
  else if (type == ENNA_CAPS_PHOTO)
    {
       printf("Get Photo caps\n");
       return _enna_vfs_photo;
    }
  else
    return NULL;
}




