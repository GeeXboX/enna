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
#include "event_key.h"
#include "vfs.h"
#include "enna_config.h"
#include "view_wall.h"
#include "content.h"
#include "mainmenu.h"
#include "infos.h"

#define ENNA_MODULE_NAME "configuration"

typedef enum _CONFIGURATION_STATE
{
    MENU_VIEW,
    CONTENT_VIEW
} CONFIGURATION_STATE;

typedef struct _Enna_Module_Configuration {
    Evas *e;
    Evas_Object *o_edje;
    Evas_Object *o_menu;
    Enna_Config_Panel *selected;
    Eina_List *items;
    CONFIGURATION_STATE state;
} Enna_Module_Configuration;

static Enna_Module_Configuration *mod;
static Enna_Config_Panel *info1 = NULL;

static void _delete_menu(void);

/****************************************************************************/
/*                            Callbacks                                     */
/****************************************************************************/

static void
_item_selected_cb(void *data)
{
    Evas_Object *new = NULL;
    Enna_Config_Panel *p = data;

    // run the create_cb from the Config_Panel
    if (p->create_cb) new = (p->create_cb)(p->data);
    if (!new) return;

    _delete_menu ();

    // Swalllow-in the new panel
    edje_object_part_swallow (mod->o_edje, "enna.swallow.content", new);
    mod->state = CONTENT_VIEW;
    mod->selected = p;
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

    mod->state = MENU_VIEW;

    mod->o_menu = enna_wall_add (enna->evas);
    edje_object_part_swallow (mod->o_edje, "enna.swallow.content", mod->o_menu);

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
    //printf("**** ACTIVITY SHOW ****\n");

    // create the content if not created yet
    if (!mod->o_edje)
    {
        mod->o_edje = edje_object_add (enna->evas);
        edje_object_file_set (mod->o_edje, enna_config_theme_get (),
                              "module/configuration");
        _create_menu ();
        enna_content_append (ENNA_MODULE_NAME, mod->o_edje);
    }

    enna_content_select(ENNA_MODULE_NAME);

    edje_object_signal_emit (mod->o_edje, "module,show", "enna");
    edje_object_signal_emit (mod->o_edje, "content,show", "enna");
}

static void
_activity_hide (int dummy)
{
    //printf("**** ACTIVITY HIDE ****\n");
    edje_object_signal_emit (mod->o_edje, "module,hide", "enna");
    edje_object_signal_emit (mod->o_edje, "content,hide", "enna");
    //TODO here we need to notify the hide to children panel, or they remain
    // active also when not showed.
}

static void
_activity_event (void *event_info)
{
    //printf("**** ACTIVITY EVENT ****\n");
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key (ev);

    switch (mod->state)
    {
    case CONTENT_VIEW:
        if (key == ENNA_KEY_CANCEL)
        {
            // run the destroy_cb from the Config_Panel
            Enna_Config_Panel *p = mod->selected;
            if (p && p->destroy_cb) (p->destroy_cb)(p->data);
            
            mod->selected = NULL;
            _create_menu();
        }
        break;
    default:
        if (key == ENNA_KEY_CANCEL)
        {
            //TODO here we need to notify the hide to children panel, or they remain
            // active also when not showed.
            enna_content_hide();
            enna_mainmenu_show();
        }
        else
            enna_wall_event_feed(mod->o_menu, event_info);
        break;
    }
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("Configuration"),
    NULL,
    "icon/infos",
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
    ENNA_MODULE_ACTIVITY,
    "configuration"
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

    ENNA_OBJECT_DEL (mod->o_edje);
    _delete_menu ();
    free (mod);
}

