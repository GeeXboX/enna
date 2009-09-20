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
    Enna_Module *em;
    Evas_Object *o_menu;
    Evas_Object *o_content;
    Eina_List *items;
    CONFIGURATION_STATE state;
} Enna_Module_Configuration;

static Enna_Module_Configuration *mod;

static void _delete_menu(void);

/****************************************************************************/
/*                            Callbacks                                     */
/****************************************************************************/

static void
_infos_selected_cb(void *data)
{
    _delete_menu ();
    ENNA_OBJECT_DEL (mod->o_content);
    mod->o_content = enna_infos_add (mod->em->evas);
    edje_object_part_swallow (mod->o_edje, "enna.swallow.content", mod->o_content);
    mod->state = CONTENT_VIEW;
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
_create_menu (void)
{
    Enna_Vfs_File *it;

    mod->state = MENU_VIEW;

    mod->o_menu = enna_wall_add (mod->em->evas);
    edje_object_part_swallow (mod->o_edje, "enna.swallow.content", mod->o_menu);

    it = calloc (1, sizeof(Enna_Vfs_File));
    it->icon = (char*)eina_stringshare_add ("icon/infos");
    it->label = (char*)eina_stringshare_add (_("Infos"));
    it->is_directory = 1;

    enna_wall_file_append (mod->o_menu, it, _infos_selected_cb, NULL);
    mod->items = eina_list_append (mod->items, it);

    enna_wall_select_nth(mod->o_menu, 0, 0);

}

static void
_delete_menu(void)
{
    Eina_List *l;

    if (!mod->o_menu)
	return;

    for (l = mod->items; l; l = l->next)
    {
        Enna_Vfs_File *it = l->data;
        mod->items = eina_list_remove(mod->items, it);
    }
    ENNA_OBJECT_DEL(mod->o_menu);
}

static void
_class_init (int dummy)
{
    mod->o_edje = edje_object_add (mod->em->evas);
    edje_object_file_set (mod->o_edje,
	enna_config_theme_get (), "module/configuration");
    _create_menu ();
    enna_content_append (ENNA_MODULE_NAME, mod->o_edje);
}

static void
_class_shutdown (int dummy)
{
    ENNA_OBJECT_DEL (mod->o_edje);
}

static void
_class_show (int dummy)
{
    edje_object_signal_emit (mod->o_edje, "module,show", "enna");
    edje_object_signal_emit (mod->o_edje, "content,show", "enna");
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit (mod->o_edje, "module,hide", "enna");
    edje_object_signal_emit (mod->o_edje, "content,hide", "enna");
}

static void
_class_event (void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key (ev);



    switch (mod->state)
    {
    case CONTENT_VIEW:
	if (key == ENNA_KEY_CANCEL)
	{
	    ENNA_OBJECT_DEL(mod->o_content);
	    _create_menu();
	}
	break;
    default:
	if (key == ENNA_KEY_CANCEL)
	{

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
        _class_init,
        NULL,
        _class_shutdown,
        _class_show,
        _class_hide,
        _class_event
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
    mod->em = em;
    em->mod = mod;

    enna_activity_add (&class);
}

void
module_shutdown (Enna_Module *em)
{
    evas_object_del (mod->o_edje);
    _delete_menu ();
    ENNA_OBJECT_DEL (mod->o_content);
    free (mod);
}

