/** @file enna_module.c */


#include "module.h"
#include "enna.h"

static Evas_List *_enna_modules = NULL;
static Evas_List *_enna_activities = NULL;
static Ecore_Path_Group *path_group = NULL;

/**
 * @brief Init Module, Save create Ecore_Path_Group and add default module path
 * @return 1 if Initilisation is done correctly, 0 otherwise or if init is called more then twice
 */

EAPI int
enna_module_init(void)
{
  if (!path_group)
    {
      path_group = ecore_path_group_new();
      ecore_path_group_add(path_group, PACKAGE_LIB_DIR"/enna/modules/");
      return 0;
    }
  
  return -1;
}

/**
 * @brief Free all modules registered and delete Ecore_Path_Group
 * @return 1 if succes 0 otherwise
 */

EAPI int
enna_module_shutdown(void)
{
    Evas_List *l;

    for (l = _enna_modules; l; l = l->next)
        {
            Enna_Module *m;
            m = l->data;
            if (m->enabled)
                {
                    m->func.shutdown(m);
                    m->enabled = 0;
                }
            ecore_plugin_unload(m->plugin);
            ENNA_FREE(m);
            _enna_modules = evas_list_remove(_enna_modules, m);
        }


    if (path_group)
        {
            ecore_path_group_del(path_group);
            path_group = NULL;
        }

    return 0;
}

EAPI int
enna_module_enable(Enna_Module *m)
{
  if (!m) return -1;
  if(m->enabled) return 0;
  if (m->func.init)
    {
      m->func.init(m);
      m->enabled = 1;
      return 0;
    }
  return -1;
}

EAPI int
enna_module_disable(Enna_Module *m)
{
    if (!m ) return -1;
    if (!m->enabled) return 0;
    if (m->func.shutdown)
        {
            m->func.shutdown(m);
            m->enabled = 0;
            return 0;
        }
    return -1;
}

/**
 * @brief Open a module
 * @param name the module name
 * @return E_Module loaded
 * @note Module music can be loaded like this : enna_module_open("music") this module in loaded from file /usr/lib/enna/modules/music.so
 */
EAPI Enna_Module *
enna_module_open(const char *name, Evas *evas)
{
    const char *modpath;
    char module_name[4096];
    Ecore_Plugin *plugin;
    Enna_Module *m;
    if (!name || !evas) return NULL;
    m = malloc(sizeof(Enna_Module));


    if (!path_group)
        {
            dbg("Error : enna Module should be Init before call this function\n");
            return NULL;
        }

    snprintf(module_name, sizeof(module_name), "enna_module_%s", name);
    printf("Try to load %s\n", module_name);
    plugin = ecore_plugin_load(path_group, module_name, NULL);

    if (plugin)
        {
            m->api = ecore_plugin_symbol_get(plugin, "module_api");
            printf("version : %i\n", m->api->version);

            if (!m->api || m->api->version != ENNA_MODULE_VERSION )
                {
                    /* FIXME: popup error message */
                    /* Module version doesn't match enna version */
                    printf("Error : Bad module version, unload %s module\n", m->api->name);
                    ecore_plugin_unload(plugin);
                    return NULL;
                }
            m->func.init = ecore_plugin_symbol_get(plugin, "module_init");
            m->func.shutdown = ecore_plugin_symbol_get(plugin, "module_shutdown");
            m->name = m->api->name;
            printf("Module \'%s\' loaded succesfull\n", m->api->name);
            m->enabled = 0;
            m->plugin = plugin;
            m->evas = evas;
            _enna_modules = evas_list_append(_enna_modules, m);
            return m;
        }
    else
        printf ("Unable to load module %s\n", name);

    return NULL;
}

/**
 * @brief Register new activity
 * @param em enna module
 * @return -1 if error occurs, 0 otherwise
 */
EAPI int
enna_module_activity_add(Enna_Module *em, Enna_Module_Class *class)
{
  Evas_List *l; 
  Enna_Module_Class *act;

  if (!em || !class) return -1;
  em->class = class;
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
enna_module_activity_del(char *name)
{
  Evas_List *l; 
  Enna_Module_Class *act;

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
enna_module_activities_get()
{
  return _enna_activities;
}


EAPI int
enna_module_activity_init(char *name)
{
  Evas_List *l; 
  Enna_Module_Class *act;

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
enna_module_activity_show(char *name)
{
  Evas_List *l; 
  Enna_Module_Class *act;

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
enna_module_activity_shutdown(char *name)
{
  Evas_List *l; 
  Enna_Module_Class *act;

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
enna_module_activity_hide(char *name)
{
  Evas_List *l; 
  Enna_Module_Class *act;

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

