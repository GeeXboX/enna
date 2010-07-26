/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2010 The Enna Project
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _GNU_SOURCE

#include <Eina.h>
#include <Edje.h>

#include "enna.h"
#include "vfs.h"
#include "enna_config.h"
#include "view_wall.h"
#include "content.h"
#include "mainmenu.h"
#include "module.h"
#include "configuration_sysinfo.h"
#include "configuration_credits.h"

#define ENNA_MODULE_NAME "configuration"

typedef enum _CONFIGURATION_STATE
{
    MENU_VIEW,
    CONTENT_VIEW
} CONFIGURATION_STATE;

typedef struct _Enna_Module_Configuration {
    Evas_Object *o_edje;
    Evas_Object *o_menu;
    Enna_Config_Panel *selected;
    Eina_List *items;
    CONFIGURATION_STATE state;
} Enna_Module_Configuration;

static Enna_Module_Configuration *mod;
static Enna_Config_Panel *info1 = NULL;
static Enna_Config_Panel *credits = NULL;



static void _create_menu(void);
static void _delete_menu(void);
static void _show_subpanel(Enna_Config_Panel *p);
static void _hide_subpanel(Enna_Config_Panel *p);
static void _activity_hide();



/****************************************************************************/
/*                            Callbacks                                     */
/****************************************************************************/

static void
_item_selected_cb(void *data)
{
    Enna_Config_Panel *p = data;

    _show_subpanel(p);
}


/****************************************************************************/
/*                        Private Module Functions                          */
/****************************************************************************/

static void
_create_menu (void)
{
    Enna_Vfs_File *it;
    Eina_List *panels, *l;
    Enna_Config_Panel *p;

    mod->o_menu = enna_wall_add (enna->evas);

    // populate menu from config_panel
    panels = enna_config_panel_list_get();
    EINA_LIST_FOREACH(panels, l, p)
    {
        it = calloc (1, sizeof(Enna_Vfs_File));
        it->icon = (char*)eina_stringshare_add (p->icon);
        it->label = (char*)eina_stringshare_add (p->label);
        it->is_menu = 1;

        enna_wall_file_append (mod->o_menu, it, _item_selected_cb, p);
        mod->items = eina_list_append (mod->items, it);
    }

    enna_wall_select_nth(mod->o_menu, 0, 0);
    elm_layout_content_set(mod->o_edje, "menu.swallow", mod->o_menu);
    mod->state = MENU_VIEW;
}

static void
_delete_menu(void)
{
    Enna_Vfs_File *it;

    if (!mod->o_menu) return;

    EINA_LIST_FREE(mod->items, it);
        enna_vfs_remove(it);

    ENNA_OBJECT_DEL(mod->o_menu);
}

static void
_show_subpanel(Enna_Config_Panel *p)
{
    Evas_Object *new = NULL;

    if (!p) return;

    // run the create_cb from the Config_Panel
    if (p->create_cb) new = (p->create_cb)(p->data);
    if (!new) return;

    elm_layout_content_set(mod->o_edje, "content.swallow", new);
    edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "menu,hide", "enna");
    edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "content,show", "enna");

    mod->state = CONTENT_VIEW;
    mod->selected = p;
}

static void
_hide_subpanel(Enna_Config_Panel *p)
{
    if (!p) return;
    // run the destroy_cb from the Config_Panel
    if (p && p->destroy_cb) (p->destroy_cb)(p->data);

    mod->selected = NULL;
    mod->state = MENU_VIEW;
}

/****************************************************************************/
/*                        Activity Class API                                */
/****************************************************************************/
static void
_activity_show()
{
    // create the enna_content if not created yet
    if (!mod->o_edje)
    {
        mod->o_edje = elm_layout_add(enna->layout);
        elm_layout_file_set(mod->o_edje, enna_config_theme_get (),
                              "activity/configuration");
        enna_content_append (ENNA_MODULE_NAME, mod->o_edje);
    }

    // create the menu if not done yet
    if (!mod->o_menu) _create_menu();

    // show the module
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "menu,show", "enna");
    edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "module,show", "enna");
}

static void
_activity_hide()
{
    edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "menu,hide", "enna");
    edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "module,hide", "enna");
    _hide_subpanel(mod->selected);
}

static void
_activity_event (enna_input event)
{
    // menu view
    if (mod->state == MENU_VIEW)
    {
        if (event == ENNA_INPUT_BACK)
        {
            enna_content_hide();
        }
        else
            enna_wall_input_feed(mod->o_menu, event);
    }
    // subpanel view
    else if (event == ENNA_INPUT_BACK)
    {
        _hide_subpanel(mod->selected);
        edje_object_signal_emit(elm_layout_edje_get(mod->o_edje), "menu,show", "enna");
    }
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("Configuration"),
    NULL,
    "icon/config",
    "background/configuration",
    ENNA_CAPS_NONE,
    {
        NULL,
        NULL,
        NULL,
        _activity_show,
        _activity_hide,
        _activity_event
    }
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_configuration
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_Configuration));
    em->mod = mod;

    enna_activity_register(&class);

    info1 = enna_config_panel_register(_("System information"), "icon/infos",
                                    info_panel_show, info_panel_hide, NULL);

    credits = enna_config_panel_register(_("Credits"), "icon/enna",
                                         credits_panel_show,
                                         credits_panel_hide, NULL);
}

static void
module_shutdown(Enna_Module *em)
{
    enna_config_panel_unregister(info1);
    enna_config_panel_unregister(credits);
    enna_activity_unregister(&class);

    ENNA_OBJECT_DEL (mod->o_edje);
    _delete_menu ();
    free (mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_configuration",
    N_("Configuration"),
    "icon/config",
    N_("This module creates the Enna configuration panel"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

