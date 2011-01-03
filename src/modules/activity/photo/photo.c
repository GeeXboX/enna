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

#include <string.h>

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "vfs.h"
#include "view_wall.h"
#include "image.h"
#include "browser_obj.h"
#include "view_list.h"
#include "content.h"
#include "mainmenu.h"
#include "logs.h"
#include "photo.h"
#include "photo_infos.h"
#include "photo_slideshow_view.h"
#include "module.h"

#define ENNA_MODULE_NAME "photo"

static void _create_menu(void);

typedef struct _Enna_Module_Photo
{
    Evas_Object *o_layout;
    Evas_Object *o_browser;
    Evas_Object *o_infos;
    Evas_Object *o_slideshow;
    PHOTO_STATE state;
    Enna_Module *em;
    int infos_displayed;
} Enna_Module_Photo;

static Enna_Module_Photo *mod;

/****************************************************************************/
/*                             Photo Helpers                                */
/****************************************************************************/

static void _create_infos(void)
{
	Evas_Object *o_edje;
    mod->o_infos = photo_panel_infos_add (evas_object_evas_get(mod->o_layout));
    edje_object_part_swallow (mod->o_layout,
                              "infos.panel.swallow", mod->o_infos);
    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_signal_emit (o_edje, "infos,hide", "enna");
}

static void
panel_infos_display (int show)
{
	Evas_Object *o_edje;
	o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_signal_emit (o_edje,
                             show ? "infos,show": "infos,hide", "enna");
    mod->infos_displayed = show ? 1 : 0;
    mod->state = show ? INFOS_VIEW : BROWSER_VIEW;
}



/* #############################################################
   #               slideshow helpers                           #
   ############################################################# */
static void 
_slideshow_delete_cb(void *data, Evas_Object *obj, void *event_info)
{	
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    ENNA_OBJECT_DEL (mod->o_slideshow);
    mod->state = BROWSER_VIEW;
    edje_object_signal_emit(o_edje, "wall,show", "enna");
    edje_object_signal_emit(o_edje, "list,show", "enna");
}


static void _create_slideshow_gui(void)
{
	Evas_Object *o_edje;
    mod->state = SLIDESHOW_VIEW;

    ENNA_OBJECT_DEL (mod->o_slideshow);

    mod->o_slideshow = enna_photo_slideshow_add(mod->o_layout);
    elm_layout_content_set(enna->layout,
                             "enna.fullscreen.swallow", mod->o_slideshow);
    evas_object_smart_callback_add(mod->o_slideshow, "delete,requested", _slideshow_delete_cb, NULL);
    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_signal_emit(o_edje, "list,hide", "enna");
    edje_object_signal_emit(o_edje, "wall,hide", "enna");
}


static int
_slideshow_add_files(const char *file_selected)
{
    Eina_List *files = NULL;
    Eina_List *l;
    int n = 0;
    int pos = 0;
    Enna_Vfs_File *file;

    if (!file_selected)
      return -1;

    files = enna_browser_obj_files_get (mod->o_browser);
    EINA_LIST_FOREACH(files, l, file)
    {
        if (!file->mrl)
            continue;
        if (!strcmp(file_selected, file->mrl))
            pos = n;

        enna_photo_slideshow_image_add(mod->o_slideshow, file->mrl + 7, NULL);
        n++;
    }

    return pos;
}

static void
_browser_cb_root (void *data, Evas_Object *obj, void *event_info)
{
    enna_content_hide();
}

static void
_browser_cb_selected (void *data, Evas_Object *obj, void *event_info)
{
    Enna_Vfs_File *file = event_info;
    int pos;

    if (!file) return;

    if (file->type != ENNA_FILE_DIRECTORY && file->type != ENNA_FILE_MENU)
    {
        /* File is selected, display it in slideshow mode */
        _create_slideshow_gui();
        pos = _slideshow_add_files(file->mrl);
        enna_photo_slideshow_goto(mod->o_slideshow, pos);
    }
}

static void
_browser_cb_hilight (void *data, Evas_Object *obj, void *event_info)
{
    Enna_Vfs_File *file = event_info;
    Evas_Object *o_edje;

    if (!file || !file->mrl)
        return;

    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_part_text_set(o_edje, "filename.text", file->label);

    if (file->type != ENNA_FILE_DIRECTORY || file->type != ENNA_FILE_MENU)
        photo_panel_infos_set_cover(mod->o_infos, file->mrl + 7);

    photo_panel_infos_set_text(mod->o_infos, file->mrl + 7);
}

static void
_create_menu(void)
{
    mod->state = BROWSER_VIEW;

    /* Create List */
    ENNA_OBJECT_DEL(mod->o_browser);

    mod->o_browser = enna_browser_obj_add(mod->o_layout);
    enna_browser_obj_view_type_set(mod->o_browser, ENNA_BROWSER_VIEW_WALL);

    evas_object_smart_callback_add(mod->o_browser, "root",
                                   _browser_cb_root, NULL);
    evas_object_smart_callback_add(mod->o_browser, "selected",
                                   _browser_cb_selected, NULL);
    evas_object_smart_callback_add(mod->o_browser,
                                   "hilight", _browser_cb_hilight, NULL);

    elm_layout_content_set(mod->o_layout, "browser.swallow", mod->o_browser);
    enna_browser_obj_root_set(mod->o_browser, "/photo");
}

static void _create_gui(void)
{
    /* Set default state */
    mod->state = BROWSER_VIEW;

    /* Create main edje object */
    mod->o_layout = elm_layout_add(enna->layout);
    elm_layout_file_set(mod->o_layout,
                         enna_config_theme_get(), "activity/photo");

    _create_menu();
    _create_infos();
}

/****************************************************************************/
/*                             Event Handlers                               */
/****************************************************************************/

static void photo_event_browser (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_INFO:
        panel_infos_display(1);
        break;
    default:
        enna_browser_obj_input_feed(mod->o_browser, event);
        break;
    }
}

static void photo_event_info (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_BACK:
    case ENNA_INPUT_INFO:
        panel_infos_display(0);
        break;
    case ENNA_INPUT_OK:
        _create_slideshow_gui();
        _slideshow_add_files(NULL);
        break;
    default:
        break;
    }
}

static void photo_event_slideshow (enna_input event)
{
	Evas_Object *o_edje;

	o_edje = elm_layout_edje_get(mod->o_layout);
    switch (event)
    {
    case ENNA_INPUT_BACK:
        ENNA_OBJECT_DEL (mod->o_slideshow);
        mod->state = BROWSER_VIEW;
        edje_object_signal_emit(o_edje, "wall,show", "enna");
        edje_object_signal_emit(o_edje, "list,show", "enna");
        break;
    default:
        break;
    }
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void _class_init(void)
{
    _create_gui();
    enna_content_append(ENNA_MODULE_NAME, mod->o_layout);
}

static void _class_show(void)
{
	Evas_Object *o_edje;

	o_edje = elm_layout_edje_get(mod->o_layout);
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(o_edje, "module,show", "enna");
    edje_object_signal_emit(o_edje, "content,show", "enna");
}

static void _class_hide(void)
{
	Evas_Object *o_edje;

	o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_signal_emit(o_edje, "module,hide", "enna");
}

static void _class_event(enna_input event)
{
    int i;

    static const struct {
        PHOTO_STATE state;
        void (*event_handler) (enna_input event);
    } evh [] = {
        { BROWSER_VIEW,      &photo_event_browser     },
        { INFOS_VIEW,        &photo_event_info        },
        { SLIDESHOW_VIEW,    &photo_event_slideshow   },
        { 0,                 NULL                     }
    };

    for (i = 0; evh[i].event_handler; i++)
        if (mod->state == evh[i].state)
        {
            evh[i].event_handler(event);
            break;
        }
}

static Enna_Class_Activity class =
{
    ENNA_MODULE_NAME,
    1,
    N_("Photo"),
    NULL,
    "icon/photo",
    "background/photo",
    ENNA_CAPS_PHOTO,
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    }
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_photo
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Photo));
    mod->em = em;
    em->mod = mod;

    enna_activity_register(&class);
}

static void
module_shutdown(Enna_Module *em)
{
    enna_activity_unregister(&class);
    ENNA_OBJECT_DEL(mod->o_layout);

    evas_object_smart_callback_del(mod->o_browser,
                                   "root", _browser_cb_root);
    evas_object_smart_callback_del(mod->o_browser,
                                   "selected", _browser_cb_selected);
    evas_object_smart_callback_del(mod->o_browser,
                                   "hilight", _browser_cb_hilight);

    ENNA_OBJECT_DEL(mod->o_infos);
    ENNA_OBJECT_DEL(mod->o_browser);
    free(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_photo",
    N_("Photo"),
    "icon/photo",
    N_("Browse your photos and create slideshows"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
