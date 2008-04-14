#ifndef _ENNA_REGISTRY_H_
#define _ENNA_REGISTRY_H_

#include "enna.h"

typedef struct _Enna_Registry_Activity Enna_Registry_Activity;
typedef struct _Enna_Registry_Item Enna_Registry_Item;

struct _Enna_Registry_Activity
{
   const char *activity;
   int         pri;
   const char *label;
   const char *icon_file;
   const char *icon;
   Evas_List  *items;
};

struct _Enna_Registry_Item
{
   const char        *item;
   int                pri;
   const char        *label;
   const char        *icon_file;
   const char        *icon;
   void (*func) (int dummy);
};

/* EAPI void       enna_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, void (*func) (int dummy)); */
/* EAPI void       enna_registry_item_del(const char *path); */
EAPI void       enna_registry_activity_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon);
EAPI void       enna_registry_activity_del(const char *path);
EAPI Evas_List *enna_registry_activities_get(void);
/* EAPI void       enna_registry_call(const char *path, E_Container *con, const char *params); */
/* EAPI int        enna_registry_exists(const char *path); */
/* EAPI Evas_List *enna_registry_activity_get(const char *path); */
/* EAPI void       enna_registry_init(void); */


#endif
