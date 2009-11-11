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
#include "infos.h"

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



static void _create_menu(void);
static void _delete_menu(void);
static void _show_subpanel(Enna_Config_Panel *p);
static void _hide_subpanel(Enna_Config_Panel *p);
static void _activity_hide (int dummy);



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
        it->is_directory = 1;

        enna_wall_file_append (mod->o_menu, it, _item_selected_cb, p);
        mod->items = eina_list_append (mod->items, it);
    }

    enna_wall_select_nth(mod->o_menu, 0, 0);
    edje_object_part_swallow (mod->o_edje, "menu.swallow", mod->o_menu);
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

    edje_object_part_swallow (mod->o_edje, "content.swallow", new);
    edje_object_signal_emit(mod->o_edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "content,show", "enna");

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
_activity_init (int dummy)
{
    //printf("**** ACTIVITY INIT ****\n");
}

static void
_activity_shutdown (int dummy)
{
    //printf("**** ACTIVITY SDOWN ****\n");
}

static void
_activity_show (int dummy)
{
    // create the enna_content if not created yet
    if (!mod->o_edje)
    {
        mod->o_edje = edje_object_add (enna->evas);
        edje_object_file_set (mod->o_edje, enna_config_theme_get (),
                              "activity/configuration");
        enna_content_append (ENNA_MODULE_NAME, mod->o_edje);
    }

    // create the menu if not done yet
    if (!mod->o_menu) _create_menu();

    // show the module
    enna_content_select(ENNA_MODULE_NAME);    
    edje_object_signal_emit (mod->o_edje, "menu,show", "enna");
}

static void
_activity_hide (int dummy)
{
    edje_object_signal_emit (mod->o_edje, "menu,hide", "enna");
    _hide_subpanel(mod->selected);
}

static void
_activity_event (enna_input event)
{
    // menu view
    if (mod->state == MENU_VIEW)
    {
        if (event == ENNA_INPUT_EXIT)
        {
            enna_content_hide();
            enna_mainmenu_show();
        }
        else
            enna_wall_input_feed(mod->o_menu, event);
    }
    // subpanel view
    else if (event == ENNA_INPUT_EXIT)
    {
        _hide_subpanel(mod->selected);
        edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
    }
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("Configuration"),
    NULL,
    "icon/config",
    "background/configuration",
    {
        _activity_init,
        NULL,
        _activity_shutdown,
        _activity_show,
        _activity_hide,
        _activity_event
    },
    NULL
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "configuration",
    N_("Configuration"),
    "icon/config",
    N_("This module create the enna configuration panel (DO NOT UNLOAD)"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
module_init (Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_Configuration));
    em->mod = mod;

    enna_activity_add (&class);

    info1 = enna_config_panel_register(_("Infos"), "icon/infos",
                                    info_panel_show, info_panel_hide, NULL);

}

void
module_shutdown (Enna_Module *em)
{
    enna_config_panel_unregister(info1);
    enna_activity_del(ENNA_MODULE_NAME);

    ENNA_OBJECT_DEL (mod->o_edje);
    _delete_menu ();
    free (mod);
}

