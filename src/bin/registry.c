
#include "registry.h"
#include "enna.h"


Evas_List *enna_registry = NULL;

EAPI void
enna_registry_init(void)
{
   /* enna_registry_activity_add("extensions", 90, _("Extensions"), NULL, "enlightenment/extensions"); */
   /* enna_registry_item_add("extensions/modules", 10, _("Modules"), NULL, "enlightenment/modules", e_int_config_modules); */
}

/* EAPI void */
/* enna_registry_item_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon, void (*func) (int dummy)) */
/* { */
/*    Evas_List *l; */
/*    char *cat; */
/*    const char *item; */
/*    E_Configure_It *eci; */

/*    /\* path is "activity/item" *\/ */
/*    cat = ecore_file_dir_get(path); */
/*    if (!cat) return; */
/*    item = ecore_file_file_get(path); */
/*    eci = E_NEW(E_Configure_It, 1); */
/*    if (!eci) goto done; */

/*    eci->item = evas_stringshare_add(item); */
/*    eci->pri = pri; */
/*    eci->label = evas_stringshare_add(label); */
/*    if (icon_file) eci->icon_file = evas_stringshare_add(icon_file); */
/*    if (icon) eci->icon = evas_stringshare_add(icon); */
/*    eci->func = func; */

/*    for (l = enna_registry; l; l = l->next) */
/*      { */
/* 	E_Configure_Cat *ecat; */

/* 	ecat = l->data; */
/* 	if (!strcmp(cat, ecat->cat)) */
/* 	  { */
/* 	     Evas_List *ll; */

/* 	     for (ll = ecat->items; ll; ll = ll->next) */
/* 	       { */
/* 		  E_Configure_It *eci2; */

/* 		  eci2 = ll->data; */
/* 		  if (eci2->pri > eci->pri) */
/* 		    { */
/* 		       ecat->items = evas_list_prepend_relative_list(ecat->items, eci, ll); */
/* 		       goto done; */
/* 		    } */
/* 	       } */
/* 	     ecat->items = evas_list_append(ecat->items, eci); */
/* 	     goto done; */
/* 	  } */
/*      } */
/*    done: */
/*    free(cat); */
/* } */

/* EAPI void */
/* enna_registry_item_del(const char *path) */
/* { */
/*    Evas_List *l; */
/*    char *cat; */
/*    const char *item; */

/*    /\* path is "activity/item" *\/ */
/*    cat = ecore_file_dir_get(path); */
/*    if (!cat) return; */
/*    item = ecore_file_file_get(path); */
/*    for (l = enna_registry; l; l = l->next) */
/*      { */
/* 	E_Configure_Cat *ecat; */

/* 	ecat = l->data; */
/* 	if (!strcmp(cat, ecat->cat)) */
/* 	  { */
/* 	     Evas_List *ll; */

/* 	     for (ll = ecat->items; ll; ll = ll->next) */
/* 	       { */
/* 		  E_Configure_It *eci; */

/* 		  eci = ll->data; */
/* 		  if (!strcmp(item, eci->item)) */
/* 		    { */
/* 		       ecat->items = evas_list_remove_list(ecat->items, ll); */
/* 		       evas_stringshare_del(eci->item); */
/* 		       evas_stringshare_del(eci->label); */
/* 		       evas_stringshare_del(eci->icon); */
/* 		       if (eci->icon_file) evas_stringshare_del(eci->icon_file); */
/* 		       free(eci); */
/* 		       goto done; */
/* 		    } */
/* 	       } */
/* 	     goto done; */
/* 	  } */
/*      } */
/*    done: */
/*    free(cat); */
/* } */

EAPI void
enna_registry_activity_add(const char *path, int pri, const char *label, const char *icon_file, const char *icon)
{
   Enna_Registry_Activity *eact;
   Evas_List *l;

   eact = calloc(1, sizeof(Enna_Registry_Activity));
   if (!eact) return;

   eact->activity = evas_stringshare_add(path);
   eact->pri = pri;
   eact->label = evas_stringshare_add(label);
   if (icon_file) eact->icon_file = evas_stringshare_add(icon_file);
   if (icon) eact->icon = evas_stringshare_add(icon);
   for (l = enna_registry; l; l = l->next)
     {
	Enna_Registry_Activity *eact2;

	eact2 = l->data;
	if (eact2->pri > eact->pri)
	  {
	     enna_registry = evas_list_prepend_relative_list(enna_registry, eact, l);
	     return;
	  }
     }
   enna_registry = evas_list_append(enna_registry, eact);
}

EAPI void
enna_registry_activity_del(const char *path)
{
   Evas_List *l;
   char *cat;

   cat = ecore_file_dir_get(path);
   if (!cat) return;
   for (l = enna_registry; l; l = l->next)
     {
	Enna_Registry_Activity *eact;

	eact = l->data;
        if (!strcmp(cat, eact->activity))
	  {
	     if (eact->items) goto done;
	     enna_registry = evas_list_remove_list(enna_registry, l);
	     evas_stringshare_del(eact->activity);
	     evas_stringshare_del(eact->label);
	     if (eact->icon) evas_stringshare_del(eact->icon);
	     if (eact->icon_file) evas_stringshare_del(eact->icon_file);
	     free(eact);
	     goto done;
	  }
     }
   done:
   free(cat);
}

/* EAPI void */
/* enna_registry_call(const char *path, E_Container *con, const char *params) */
/* { */
/*    Evas_List *l; */
/*    char *cat; */
/*    const char *item; */

/*    /\* path is "activity/item" *\/ */
/*    cat = ecore_file_dir_get(path); */
/*    if (!cat) return; */
/*    item = ecore_file_file_get(path); */
/*    for (l = enna_registry; l; l = l->next) */
/*      { */
/* 	E_Configure_Cat *ecat; */

/* 	ecat = l->data; */
/* 	if (!strcmp(cat, ecat->cat)) */
/* 	  { */
/* 	     Evas_List *ll; */

/* 	     for (ll = ecat->items; ll; ll = ll->next) */
/* 	       { */
/* 		  E_Configure_It *eci; */

/* 		  eci = ll->data; */
/* 		  if (!strcmp(item, eci->item)) */
/* 		    { */
/* 		       if (eci->func) eci->func(con, params); */
/* 		       goto done; */
/* 		    } */
/* 	       } */
/* 	     goto done; */
/* 	  } */
/*      } */
/*    done: */
/*    free(cat); */
/* } */

/* EAPI int */
/* enna_registry_exists(const char *path) */
/* { */
/*    Evas_List *l; */
/*    char *cat; */
/*    const char *item; */
/*    int ret = 0; */

/*    /\* path is "activity/item" *\/ */
/*    cat = ecore_file_dir_get(path); */
/*    if (!cat) return 0; */
/*    item = ecore_file_file_get(path); */
/*    for (l = enna_registry; l; l = l->next) */
/*      { */
/* 	E_Configure_Cat *ecat; */

/* 	ecat = l->data; */
/* 	if (!strcmp(cat, ecat->cat)) */
/* 	  { */
/* 	     Evas_List *ll; */

/* 	     if (!item) */
/* 	       { */
/* 		  ret = 1; */
/* 		  goto done; */
/* 	       } */
/* 	     for (ll = ecat->items; ll; ll = ll->next) */
/* 	       { */
/* 		  E_Configure_It *eci; */

/* 		  eci = ll->data; */
/* 		  if (!strcmp(item, eci->item)) */
/* 		    { */
/* 		       ret = 1; */
/* 		       goto done; */
/* 		    } */
/* 	       } */
/* 	     goto done; */
/* 	  } */
/*      } */
/*    done: */
/*    free(cat); */
/*    return ret; */
/* } */
