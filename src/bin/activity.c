#include "enna.h"
#include "activity.h"


static Evas_List *_enna_activities = NULL;

/**
 * @brief Register new activity
 * @param em enna module
 * @return -1 if error occurs, 0 otherwise
 */
EAPI int
enna_activity_add(Enna_Class_Activity *class)
{
  Evas_List *l;
  Enna_Class_Activity *act;

  if (!class) return -1;
  for (l = _enna_activities; l; l = l->next)
    {
       act = l->data;
       if (act->pri > class->pri)
	 {
	    _enna_activities = evas_list_prepend_relative_list(_enna_activities, class, l);
	    return 0;
	 }
    }
  _enna_activities = evas_list_append(_enna_activities, class);
  return 0;
}

/**
 * @brief Register new activity
 * @param em enna module
 * @return -1 if error occurs, 0 otherwise
 */
EAPI int
enna_activity_del(char *name)
{
  Evas_List *l;
  Enna_Class_Activity *act;

  if (!name) return -1;

  for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
   	_enna_activities = evas_list_remove(_enna_activities, act);
	return 0;
     }
  return -1;
}


/**
 * @brief Get list of activities registred
 * @return Evas_List of activities
 */
EAPI Evas_List *
enna_activities_get(void)
{
  return _enna_activities;
}


EAPI int
enna_activity_init(char *name)
{
  Evas_List *l;
  Enna_Class_Activity *act;

  if (!name) return -1;

  for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
	if (!strcmp(act->name, name))
	  {
              if (act->func.class_init)
                  act->func.class_init(0);
	  }
     }
  return 0;
}

EAPI int
enna_activity_show(char *name)
{
  Evas_List *l;
  Enna_Class_Activity *act;

  if (!name) return -1;

  for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
	if (!strcmp(act->name, name))
	  {
	    if (act->func.class_show)
	      act->func.class_show(0);
	  }
     }
  return 0;
}

EAPI int
enna_activity_shutdown(char *name)
{
  Evas_List *l;
  Enna_Class_Activity *act;

  if (!name) return -1;

  for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
	if (!strcmp(act->name, name))
	  {
	    if (act->func.class_shutdown)
                act->func.class_shutdown(0);
	  }
     }
  return 0;
}

EAPI int
enna_activity_hide(char *name)
{
  Evas_List *l;
  Enna_Class_Activity *act;

  if (!name) return -1;

  for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
	if (!strcmp(act->name, name))
	  {
	    if (act->func.class_hide)
	      act->func.class_hide(0);
	  }
     }
  return 0;
}

EAPI int
enna_activity_category_add(const char *name, Enna_Class_Filesystem *class)
{
   Enna_Class_Activity *act;
   Evas_List *l, *l2;


   if (!name || !class) return -1;
   for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
	if (!strcmp(act->name, name))
	  {
	     Enna_Class_Filesystem *cat;
	     for (l2 = act->categories; l2; l2 = l2->next)
	       {
		  cat = l2->data;
		  if (cat->pri > class->pri)
		    {
		       act->categories = evas_list_prepend_relative_list(act->categories, class, l2);
		       return 0;
		    }
	       }
	     act->categories = evas_list_append(act->categories, class);
	     return 0;
	  }
     }
   return -1;
}

EAPI Evas_List *enna_activity_categories_get(const char *name)
{
   Enna_Class_Activity *act;
   Evas_List *l;

   if (!name) return NULL;

   for (l = _enna_activities; l; l = l->next)
     {
	act = l->data;
	if (!strcmp(act->name, name))
	  {
	     return act->categories;
	  }
     }
   return NULL;
}
