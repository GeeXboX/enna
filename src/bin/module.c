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

#ifdef USE_STATIC_MODULES
#define ENNA_MOD_EXTERN(name)                                 \
    extern void enna_mod_##name##_init(Enna_Module *em);      \
    extern void enna_mod_##name##_shutdown(Enna_Module *em);  \
    extern Enna_Module_Api enna_mod_##name##_api;

#define ENNA_MOD_REG(name)                      \
    {                                           \
        .init     = enna_mod_##name##_init,     \
        .shutdown = enna_mod_##name##_shutdown, \
        .api      = &enna_mod_##name##_api,     \
    },

#ifdef BUILD_ACTIVITY_BOOKSTORE
ENNA_MOD_EXTERN(activity_bookstore)
#endif /* BUILD_ACTIVITY_BOOKSTORE */
#ifdef BUILD_ACTIVITY_CONFIGURATION
ENNA_MOD_EXTERN(activity_configuration)
#endif /* BUILD_ACTIVITY_CONFIGURATION */
#ifdef BUILD_ACTIVITY_GAMES
ENNA_MOD_EXTERN(activity_games)
#endif /* BUILD_ACTIVITY_GAMES */
#ifdef BUILD_ACTIVITY_MUSIC
ENNA_MOD_EXTERN(activity_music)
#endif /* BUILD_ACTIVITY_MUSIC */
#ifdef BUILD_ACTIVITY_PHOTO
ENNA_MOD_EXTERN(activity_photo)
#endif /* BUILD_ACTIVITY_PHOTO */
#ifdef BUILD_ACTIVITY_TV
ENNA_MOD_EXTERN(activity_tv)
#endif /* BUILD_ACTIVITY_TV */
#ifdef BUILD_ACTIVITY_VIDEO
ENNA_MOD_EXTERN(activity_video)
#endif /* BUILD_ACTIVITY_VIDEO */
#ifdef BUILD_ACTIVITY_WEATHER
ENNA_MOD_EXTERN(activity_weather)
#endif /* BUILD_ACTIVITY_WEATHER */
#ifdef BUILD_BROWSER_CDDA
ENNA_MOD_EXTERN(browser_cdda)
#endif /* BUILD_BROWSER_CDDA */
#ifdef BUILD_BROWSER_DVD
ENNA_MOD_EXTERN(browser_dvd)
#endif /* BUILD_BROWSER_DVD */
#ifdef BUILD_BROWSER_LOCALFILES
ENNA_MOD_EXTERN(browser_localfiles)
#endif /* BUILD_BROWSER_LOCALFILES */
#ifdef BUILD_BROWSER_NETSTREAMS
ENNA_MOD_EXTERN(browser_netstreams)
#endif /* BUILD_BROWSER_NETSTREAMS */
#ifdef BUILD_BROWSER_PODCASTS
ENNA_MOD_EXTERN(browser_podcasts)
#endif /* BUILD_BROWSER_PODCASTS */
#ifdef BUILD_BROWSER_SHOUTCAST
ENNA_MOD_EXTERN(browser_shoutcast)
#endif /* BUILD_BROWSER_SHOUTCAST */
#ifdef BUILD_BROWSER_UPNP
ENNA_MOD_EXTERN(browser_upnp)
#endif /* BUILD_BROWSER_UPNP */
#ifdef BUILD_BROWSER_VALHALLA
ENNA_MOD_EXTERN(browser_valhalla)
#endif /* BUILD_BROWSER_VALHALLA */
#ifdef BUILD_INPUT_KEYB
ENNA_MOD_EXTERN(input_keyb)
#endif /* BUILD_INPUT_KEYB */
#ifdef BUILD_INPUT_LIRC
ENNA_MOD_EXTERN(input_lirc)
#endif /* BUILD_INPUT_LIRC */
#ifdef BUILD_VOLUME_HAL
ENNA_MOD_EXTERN(volume_hal)
#endif /* BUILD_VOLUME_HAL */
#ifdef BUILD_VOLUME_MTAB
ENNA_MOD_EXTERN(volume_mtab)
#endif /* BUILD_VOLUME_MTAB */

struct _Static_Mod_List
{
    void (*init)(Enna_Module *m);
    void (*shutdown)(Enna_Module *m);
    Enna_Module_Api *api;
};

static struct _Static_Mod_List _static_mod_list[] =
{
#ifdef BUILD_ACTIVITY_BOOKSTORE
ENNA_MOD_REG(activity_bookstore)
#endif /* BUILD_ACTIVITY_BOOKSTORE */
#ifdef BUILD_ACTIVITY_CONFIGURATION
ENNA_MOD_REG(activity_configuration)
#endif /* BUILD_ACTIVITY_CONFIGURATION */
#ifdef BUILD_ACTIVITY_GAMES
ENNA_MOD_REG(activity_games)
#endif /* BUILD_ACTIVITY_GAMES */
#ifdef BUILD_ACTIVITY_MUSIC
ENNA_MOD_REG(activity_music)
#endif /* BUILD_ACTIVITY_MUSIC */
#ifdef BUILD_ACTIVITY_PHOTO
ENNA_MOD_REG(activity_photo)
#endif /* BUILD_ACTIVITY_PHOTO */
#ifdef BUILD_ACTIVITY_TV
ENNA_MOD_REG(activity_tv)
#endif /* BUILD_ACTIVITY_TV */
#ifdef BUILD_ACTIVITY_VIDEO
ENNA_MOD_REG(activity_video)
#endif /* BUILD_ACTIVITY_VIDEO */
#ifdef BUILD_ACTIVITY_WEATHER
ENNA_MOD_REG(activity_weather)
#endif /* BUILD_ACTIVITY_WEATHER */
#ifdef BUILD_BROWSER_CDDA
ENNA_MOD_REG(browser_cdda)
#endif /* BUILD_BROWSER_CDDA */
#ifdef BUILD_BROWSER_DVD
ENNA_MOD_REG(browser_dvd)
#endif /* BUILD_BROWSER_DVD */
#ifdef BUILD_BROWSER_LOCALFILES
ENNA_MOD_REG(browser_localfiles)
#endif /* BUILD_BROWSER_LOCALFILES */
#ifdef BUILD_BROWSER_NETSTREAMS
ENNA_MOD_REG(browser_netstreams)
#endif /* BUILD_BROWSER_NETSTREAMS */
#ifdef BUILD_BROWSER_PODCASTS
ENNA_MOD_REG(browser_podcasts)
#endif /* BUILD_BROWSER_PODCASTS */
#ifdef BUILD_BROWSER_SHOUTCAST
ENNA_MOD_REG(browser_shoutcast)
#endif /* BUILD_BROWSER_SHOUTCAST */
#ifdef BUILD_BROWSER_UPNP
ENNA_MOD_REG(browser_upnp)
#endif /* BUILD_BROWSER_UPNP */
#ifdef BUILD_BROWSER_VALHALLA
ENNA_MOD_REG(browser_valhalla)
#endif /* BUILD_BROWSER_VALHALLA */
#ifdef BUILD_INPUT_KEYB
ENNA_MOD_REG(input_keyb)
#endif /* BUILD_INPUT_KEYB */
#ifdef BUILD_INPUT_LIRC
ENNA_MOD_REG(input_lirc)
#endif /* BUILD_INPUT_LIRC */
#ifdef BUILD_VOLUME_HAL
ENNA_MOD_REG(volume_hal)
#endif /* BUILD_VOLUME_HAL */
#ifdef BUILD_VOLUME_MTAB
ENNA_MOD_REG(volume_mtab)
#endif /* BUILD_VOLUME_MTAB */
    { NULL, NULL, NULL }
};
#endif /* USE_STATIC_MODULES */


/**
 * @brief Init Module, Save create Ecore_Path_Group and add default module path
 * @return 1 if Initilisation is done correctly, 0 otherwise or if init is called more then twice
 */

int
enna_module_init(void)
{
    const char *p;

#ifdef USE_STATIC_MODULES
    struct _Static_Mod_List *mod;

    enna_log(ENNA_MSG_INFO, NULL, "Available Plugins (static):");
    for (mod = _static_mod_list; mod->api; mod++)
    {
        p = mod->api->name;
#else
    Eina_List *mod, *l;

    if (path_group)
        return -1;

    path_group = ecore_path_group_new();

    ecore_path_group_add(path_group, PACKAGE_LIB_DIR"/enna/modules/");
    enna_log (ENNA_MSG_INFO, NULL,
              "Plugin Directory: %s", PACKAGE_LIB_DIR"/enna/modules/");
    mod = ecore_plugin_available_get(path_group);
    enna_log(ENNA_MSG_INFO, NULL, "Available Plugins (dynamic):");
    EINA_LIST_FOREACH(mod, l, p)
    {
#endif /* USE_STATIC_MODULES */
        enna_log(ENNA_MSG_INFO, NULL, "\t * %s", p);
    }

    _config_panel = enna_config_panel_register(_("Modules"), "icon/module",
                                  _config_panel_show, _config_panel_hide, NULL);

    return 0;
}

void
enna_module_load_all(void)
{
    const char *name;
#ifdef USE_STATIC_MODULES
    struct _Static_Mod_List *mod;
#else
    Eina_List *l;
#endif /* USE_STATIC_MODULES */

#ifdef USE_STATIC_MODULES
    for (mod = _static_mod_list; mod->api; mod++)
    {
        name = mod->api->name;
#else
    EINA_LIST_FOREACH(ecore_plugin_available_get (path_group), l, name)
    {
#endif /* USE_STATIC_MODULES */
        Enna_Module *em;

        em = enna_module_open(name);
        enna_module_enable(em);
    }
}

/**
 * @brief Free all modules registered and delete Ecore_Path_Group
 */
void
enna_module_shutdown(void)
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
#ifndef USE_STATIC_MODULES
        ecore_plugin_unload(m->plugin);
#endif /* USE_STATIC_MODULES */
        free(m);
    }

    if (path_group)
    {
        ecore_path_group_del(path_group);
        path_group = NULL;
    }
}

int
enna_module_enable(Enna_Module *m)
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

int
enna_module_disable(Enna_Module *m)
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
    Ecore_Plugin *plugin = NULL;
    Enna_Module *m;
#ifdef USE_STATIC_MODULES
    struct _Static_Mod_List *mod;
#endif /* USE_STATIC_MODULES */

    if (!name) return NULL;

#ifndef USE_STATIC_MODULES
    if (!path_group)
    {
        enna_log (ENNA_MSG_ERROR, NULL,
                  "enna Module should be Init before call this function");
        return NULL;
    }
#endif /* USE_STATIC_MODULES */

    enna_log(ENNA_MSG_INFO, NULL, "Loading module: %s", name);

#ifndef USE_STATIC_MODULES
    plugin = ecore_plugin_load(path_group, name, NULL);
    if (!plugin)
    {
        enna_log (ENNA_MSG_WARNING, NULL, "Unable to load module %s", name);
        return NULL;
    }
#endif /* USE_STATIC_MODULES */

    m = ENNA_NEW(Enna_Module, 1);
    if (!m)
    {
#ifndef USE_STATIC_MODULES
        ecore_plugin_unload(plugin);
#endif /* USE_STATIC_MODULES */
        return NULL;
    }

#ifdef USE_STATIC_MODULES
    for (mod = _static_mod_list; mod->api; mod++)
        if (!strcmp(mod->api->name, name))
        {
            m->func.init = mod->init;
            m->func.shutdown = mod->shutdown;
            m->api = mod->api;
            break;
        }
    if (!m->api)
    {
        ENNA_FREE(m);
        return NULL;
    }
#else
    m->api = ecore_plugin_symbol_get(plugin, ENNA_STRINGIFY(MOD_PREFIX) "_api");
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

    m->func.init =
        ecore_plugin_symbol_get(plugin, ENNA_STRINGIFY(MOD_PREFIX) "_init");
    m->func.shutdown =
        ecore_plugin_symbol_get(plugin, ENNA_STRINGIFY(MOD_PREFIX) "_shutdown");
#endif /* USE_STATIC_MODULES */
    m->name = m->api->name;
    m->enabled = 0;
    m->plugin = plugin;
    _enna_modules = eina_list_append(_enna_modules, m);
    return m;
}

/******************************************************************************/
/*                     Config Panel Stuff                                     */
/******************************************************************************/


static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    //~ printf("INPUT.. to module! %d\n", event);
    return enna_list2_input_feed(o_list, event);
}


void
_info_button_cb(void *data)
{
    //~ Enna_Module *m = data;

    printf("Info clicked...TODO show module info\n");
}

void
_enable_button_cb(void *data)
{
    Enna_Module *m = data;

    if (!m) return;
    printf("Button clicked...\n");

    printf("SELECTED MOD: %s\n", m->name);
    if (m->enabled)
        enna_module_disable(m);
    else
        enna_module_enable(m);
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
        item = enna_list2_append(o_list,
                                 m->api->title ? _(m->api->title) : _(m->name),
                                 m->api->short_desc ? _(m->api->short_desc) :
                                    _("No information provided"),
                                 m->api->icon ? m->api->icon : "icon/module",
                                 NULL, NULL);

        enna_list2_item_button_add(item, "icon/podcast", "info",
                                   _info_button_cb , NULL);
        //~ enna_list2_item_toggle_add(item, "icon/photo", "just for test",
                                   //~ _button_cb , NULL);
        enna_list2_item_check_add(item, "icon/music", "enabled",
                                  m->enabled, _enable_button_cb, m);
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
