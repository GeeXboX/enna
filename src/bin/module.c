/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>

#include <Eina.h>
#include <Ecore_Data.h>

#include "enna.h"
#include "module.h"
#include "input.h"
#include "enna_config.h"
#include "view_list.h"
#include "view_list2.h"
#include "input.h"
#include "logs.h"

static Eina_List *_enna_modules = NULL;
static Ecore_Path_Group *path_group = NULL;
static Enna_Config_Panel *_config_panel = NULL;
static Evas_Object *o_list = NULL;
static Input_Listener *_listener = NULL;

static Evas_Object *_config_panel_show(void *data);
static void _config_panel_hide(void *data);


/**
 * @brief Init Module, Save create Ecore_Path_Group and add default module path
 * @return 1 if Initilisation is done correctly, 0 otherwise or if init is called more then twice
 */

int enna_module_init(void)
{
    Eina_List *mod, *l;
    char *p;

    if (path_group)
        return -1;

    path_group = ecore_path_group_new();

    ecore_path_group_add(path_group, PACKAGE_LIB_DIR"/enna/modules/");
    enna_log (ENNA_MSG_INFO, NULL,
              "Plugin Directory: %s", PACKAGE_LIB_DIR"/enna/modules/");
    mod = ecore_plugin_available_get(path_group);
    enna_log(ENNA_MSG_INFO, NULL, "Available Plugins:");
    EINA_LIST_FOREACH(mod, l, p)
        enna_log(ENNA_MSG_INFO, NULL, "\t * %s", p);

    _config_panel = enna_config_panel_register(_("Modules"), "icon/video",
                                  _config_panel_show, _config_panel_hide, NULL);

    return 0;
}

static const struct {
    const char *type_name;
    _Enna_Module_Type type;
} module_class_mapping[] = {
    { "activity",       ENNA_MODULE_ACTIVITY  },
    { "browser",        ENNA_MODULE_BROWSER   },
    { "metadata",       ENNA_MODULE_METADATA  },
    { "volume",         ENNA_MODULE_VOLUME    },
    { "input",          ENNA_MODULE_INPUT     },
    { NULL,             ENNA_MODULE_UNKNOWN   }
};

void enna_module_load_all (Evas *evas)
{
    Eina_List *mod, *l;
    char *p;

    if (!evas)
        return;

    mod = ecore_plugin_available_get (path_group);
    EINA_LIST_FOREACH (mod, l, p) {
        Enna_Module *em;
        _Enna_Module_Type type = ENNA_MODULE_UNKNOWN;
        char tp[64], name[128];
        int res, i;

        if (!p)
            continue;

        res = sscanf (p, "%[^_]_%s", tp, name);
        if (res != 2)
            continue;

        for (i = 0; module_class_mapping[i].type_name; i++)
            if (!strcmp (tp, module_class_mapping[i].type_name))
            {
                type = module_class_mapping[i].type;
                break;
            }

        em = enna_module_open (name, type, enna->evas);
        enna_module_enable (em);
    }
}

/**
 * @brief Free all modules registered and delete Ecore_Path_Group
 * @return 1 if succes 0 otherwise
 */

int enna_module_shutdown(void)
{
    Eina_List *l;

    enna_config_panel_unregister(_config_panel);

    for (l = _enna_modules; l; l = eina_list_remove(l, l->data))
    {
        Enna_Module *m;
        m = l->data;

        if (m->enabled)
        {
            enna_log(ENNA_MSG_EVENT, NULL, "disable module : %s", m->name);
            enna_module_disable(m);
        }
        ecore_plugin_unload(m->plugin);
        free(m);
    }

    if (path_group)
    {
        ecore_path_group_del(path_group);
        path_group = NULL;
    }
    return 0;
}

int enna_module_enable(Enna_Module *m)
{
    printf("ENABLE MODULE: %s\n", m->name);
    if (!m)
        return -1;
    if (m->enabled)
        return 0;
    if (m->func.init)
        m->func.init(m);
    m->enabled = 1;
    return 0;
}

int enna_module_disable(Enna_Module *m)
{
    printf("DISABLE MODULE: %s\n", m->name);
    if (!m)
        return -1;
    if (!m->enabled)
        return 0;
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
 * @note Module music can be loaded like this :
 *  enna_module_open("activity_music") this
 *       module in loaded from file /usr/lib/enna/modules/activity_music.so
 */
Enna_Module *
enna_module_open(const char *name, _Enna_Module_Type type, Evas *evas)
{
    Ecore_Plugin *plugin;
    Enna_Module *m;
    char module_name[4096];
    const char *module_class = NULL;
    int i;

    if (!name || !evas) return NULL;

    m = calloc(1,sizeof(Enna_Module));

    if (!path_group)
    {
        enna_log (ENNA_MSG_ERROR, NULL,
                  "enna Module should be Init before call this function");
        return NULL;
    }

    for (i = 0; module_class_mapping[i].type_name; i++)
        if (type == module_class_mapping[i].type)
        {
            module_class = module_class_mapping[i].type_name;
            break;
        }

    snprintf(module_name, sizeof(module_name), "%s_%s", module_class, name);
    enna_log (ENNA_MSG_EVENT, NULL, "Try to load %s", module_name);
    plugin = ecore_plugin_load(path_group, module_name, NULL);
    if (!plugin)
    {
        enna_log (ENNA_MSG_WARNING, NULL, "Unable to load module %s", name);
        return NULL;
    }

    m->api = ecore_plugin_symbol_get(plugin, "module_api");
    if (!m->api || m->api->version != ENNA_MODULE_VERSION )
    {
        /* FIXME: popup error message */
        /* Module version doesn't match enna version */
        enna_log (ENNA_MSG_WARNING, NULL,
                  "Bad module version, unload %s module", m->api->name);
        ecore_plugin_unload(plugin);
        return NULL;
    }
    m->func.init = ecore_plugin_symbol_get(plugin, "module_init");
    m->func.shutdown = ecore_plugin_symbol_get(plugin, "module_shutdown");
    m->name = m->api->name;
    enna_log (ENNA_MSG_INFO, NULL,
              "Module \'%s\' loaded succesfully", m->api->name);
    m->enabled = 0;
    m->plugin = plugin;
    m->evas = evas;
    _enna_modules = eina_list_append(_enna_modules, m);
    return m;
}

/******************************************************************************/
/*                     Config Panel Stuff                                     */
/******************************************************************************/
static void
_list_selected_cb(void *data)
{
    Enna_Module *m;

    m = data;
    if (!m) return;

    printf("SELECTED MOD: %s\n", m->name);
    if (m->enabled)
        enna_module_disable(m);
    else
        enna_module_enable(m);
}

static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    //~ printf("INPUT.. to module! %d\n", event);
    return enna_list2_input_feed(o_list, event);
}

static Evas_Object *
_config_panel_show(void *data)
{
    Enna_Module *m;
    Eina_List *l;

    printf("** Modules Panel Show **\n");
    if (o_list) return o_list;
    
    // create list
    o_list = enna_list2_add(enna->evas);
    evas_object_size_hint_align_set(o_list, -1.0, -1.0);
    evas_object_size_hint_weight_set(o_list, 1.0, 1.0);
    evas_object_show(o_list);

    // populate list
    EINA_LIST_FOREACH(_enna_modules, l, m)
    {
        enna_list2_append(o_list, m->name,
                          m->enabled ? "Module enabled. press to disable" :
                          "Module disabled. press to enable",
                          "icon/video", _list_selected_cb, m); //TODO fixme
    }

    if (!_listener)
        _listener = enna_input_listener_add("configuration/modules",
                                            _input_events_cb, NULL);

    return o_list;
}

static void
_config_panel_hide(void *data)
{
    printf("** Modules Panel Hide **\n");
    enna_input_listener_del(_listener);
    _listener = NULL;
    ENNA_OBJECT_DEL(o_list);
}


    
