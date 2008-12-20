#ifndef _ENNA_BROWSER_H
#define _ENNA_BROWSER_H

typedef struct _Browser_Selected_File_Data Browser_Selected_File_Data;

struct _Browser_Selected_File_Data
{
    Enna_Class_Vfs *vfs;
    Enna_Vfs_File *file;
    Eina_List *files;
};


Evas_Object    *enna_browser_add(Evas * evas);
void            enna_browser_root_set(Evas_Object *obj, Enna_Class_Vfs *vfs);
void            enna_browser_event_feed(Evas_Object *obj, void *event_info);
#endif
