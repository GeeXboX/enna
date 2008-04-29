#ifndef __ENNA_VFS_H__
#define __ENNA_VFS_H__

#include "enna.h"

typedef enum _ENNA_VFS_CAPS ENNA_VFS_CAPS;
typedef struct _Enna_Class_Vfs Enna_Class_Vfs;
typedef struct _Enna_Vfs_File Enna_Vfs_File;


enum _ENNA_VFS_CAPS  {
  ENNA_CAPS_MUSIC = 0x01,
  ENNA_CAPS_VIDEO = 0x02,
  ENNA_CAPS_PHOTO = 0x04
};

struct _Enna_Vfs_File
{
   const char *uri;
   const char *label;
   const char *icon;
   const char *icon_file;
   unsigned char is_directory : 1;
   unsigned char is_selected : 1;


};

struct _Enna_Class_Vfs
{
   const char *name;
   int pri;
   const char *label;
   const char *icon_file;
   const char *icon;
   struct
   {
      void (*class_init)(int dummy);
      void (*class_shutdown)(int dummy);
      Evas_List *(*class_browse)(const char *path);
   }func;

};

#endif
