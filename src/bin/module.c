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

#include <Eina.h>

#include "enna.h"
#include "module.h"
#include "input.h"
#include "enna_config.h"
#include "view_list2.h"
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

void enna_module_load_all(void)
{
    Eina_List *l;
    char *name;

    EINA_LIST_FOREACH(ecore_plugin_available_get (path_group), l, name)
    {
        Enna_Module *em;

        em = enna_module_open(name);
        enna_module_enable(em);
    }
}

/**
 * @brief Free all modules registered and delete Ecore_Path_Group
 */
void enna_module_shutdown(void)
{
    Enna_Module *m;

    enna_config_panel_unregister(_config_panel);

    EINA_LIST_FREE(_enna_modules, m)
    {
        enna_log(ENNA_MSG_INFO, NULL, "Disable module : %s", m->name);
        if (m->enabled)
        {
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
}

int enna_module_enable(Enna_Module *m)
{
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
enna_module_open(const char *name)
{
    Ecore_Plugin *plugin;
    Enna_Module *m;

    if (!name) return NULL;

    if (!path_group)
    {
        enna_log (ENNA_MSG_ERROR, NULL,
                  "enna Module should be Init before call this function");
        return NULL;
    }

    enna_log(ENNA_MSG_INFO, NULL, "Loading module: %s", name);

    plugin = ecore_plugin_load(path_group, name, NULL);
    if (!plugin)
    {
        enna_log (ENNA_MSG_WARNING, NULL, "Unable to load module %s", name);
        return NULL;
    }

    m = ENNA_NEW(Enna_Module, 1);
    if (!m)
    {
        ecore_plugin_unload(plugin);
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
        ENNA_FREE(m);
        return NULL;
    }

    m->func.init = ecore_plugin_symbol_get(plugin, "module_init");
    m->func.shutdown = ecore_plugin_symbol_get(plugin, "module_shutdown");
    m->name = m->api->name;
    m->enabled = 0;
    m->plugin = plugin;
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

void _button_cb (void *data)
{
    printf("Button clicked...\n");
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
        Elm_Genlist_Item *item;
        item = enna_list2_append(o_list, m->name,
                          m->enabled ? "Module enabled. press to disable" :
                          "Module disabled. press to enable",
                          "icon/video", _list_selected_cb, m); //TODO fixme

        enna_list2_item_button_add(item, "icon/podcast", "info",
                                    _button_cb , NULL);
        enna_list2_item_button_add(item, "icon/photo", "just for test",
                                    _button_cb , NULL);
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
