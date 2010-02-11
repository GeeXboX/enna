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
#include "browser.h"
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
static void _browser_root_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_selected_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_browse_down_cb (void *data, Evas_Object *obj, void *event_info);

typedef struct _Enna_Module_Photo
{
    Evas_Object *o_edje;
    Evas_Object *o_menu;
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
    mod->o_infos = photo_panel_infos_add (evas_object_evas_get(mod->o_edje));
    edje_object_part_swallow (mod->o_edje,
                              "infos.panel.swallow", mod->o_infos);
    edje_object_signal_emit (mod->o_edje, "infos,hide", "enna");
}

static void
panel_infos_display (int show)
{
    edje_object_signal_emit (mod->o_edje,
                             show ? "infos,show": "infos,hide", "enna");
    mod->infos_displayed = show ? 1 : 0;
    mod->state = show ? INFOS_VIEW : BROWSER_VIEW;
}

/* #############################################################
   #               slideshow helpers                           #
   ############################################################# */

static void _create_slideshow_gui(void)
{
    mod->state = SLIDESHOW_VIEW;

    ENNA_OBJECT_DEL (mod->o_slideshow);

    mod->o_slideshow = enna_photo_slideshow_add(enna->evas);
    edje_object_part_swallow(mod->o_edje,
                             "fullscreen.swallow", mod->o_slideshow);

    edje_object_signal_emit(mod->o_edje, "list,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "wall,hide", "enna");
}


static int
_slideshow_add_files(char *file_selected)
{
    Eina_List *files = NULL;
    Eina_List *l;
    int n = 0;
    int pos = 0;
    Enna_Vfs_File *file;

    files = enna_browser_files_get (mod->o_browser);
    EINA_LIST_FOREACH(files, l, file)
    {
        if (!strcmp(file_selected, file->uri))
            pos = n;
        enna_photo_slideshow_image_add(mod->o_slideshow, file->uri + 7, NULL);
        n++;
    }
    eina_list_free (files);
    return pos;
}

static void
_browser_root_cb (void *data, Evas_Object *obj, void *event_info)
{
    mod->state = MENU_VIEW;
    evas_object_smart_callback_del(mod->o_browser,
                                   "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser,
                                   "selected", _browser_selected_cb);
    evas_object_smart_callback_del(mod->o_browser,
                                   "browse_down", _browser_browse_down_cb);

    /* Delete objects */
    ENNA_OBJECT_DEL(mod->o_browser);

    mod->o_browser = NULL;
    _create_menu();
}

static void
_browser_browse_down_cb (void *data, Evas_Object *obj, void *event_info)
{
//    nothing to do here anymore
}

static void
_browser_selected_cb (void *data, Evas_Object *obj, void *event_info)
{
    Browser_Selected_File_Data *ev = event_info;
    int pos;

    if (!ev || !ev->file) return;

    if (!ev->file->is_directory && !ev->file->is_menu)
    {
        /* File is selected, display it in slideshow mode */
        _create_slideshow_gui();
        pos = _slideshow_add_files(ev->file->uri);
        enna_photo_slideshow_goto(mod->o_slideshow, pos);
    }
    free(ev);
}

static void
_browser_hilight_cb (void *data, Evas_Object *obj, void *event_info)
{
    Browser_Selected_File_Data *ev = event_info;

     if (!ev || !ev->file || !ev->file->uri)
        return;

     edje_object_part_text_set(mod->o_edje, "filename.text", ev->file->label);

     if (!ev->file->is_directory || !ev->file->is_menu)
        photo_panel_infos_set_cover(mod->o_infos, ev->file->uri + 7);

     photo_panel_infos_set_text(mod->o_infos, ev->file->uri + 7);
}


static void _browse(void *data)
{
    Enna_Class_Vfs *vfs = data;

    if(!vfs) return;

    mod->o_browser = enna_browser_add(enna->evas);

    enna_browser_view_add (mod->o_browser, ENNA_BROWSER_VIEW_WALL);

    evas_object_smart_callback_add (mod->o_browser,
                                    "root", _browser_root_cb, NULL);
    evas_object_smart_callback_add (mod->o_browser,
                                    "selected", _browser_selected_cb, NULL);
    evas_object_smart_callback_add (mod->o_browser,
                                    "browse_down",
                                    _browser_browse_down_cb, NULL);
    evas_object_smart_callback_add (mod->o_browser,
                                    "hilight", _browser_hilight_cb, NULL);

    mod->state = BROWSER_VIEW;

    evas_object_show(mod->o_browser);
    edje_object_part_swallow(mod->o_edje,
                             "browser.swallow", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);

    ENNA_OBJECT_DEL (mod->o_menu);
}

static void
_create_menu(void)
{
    Evas_Object *o;
    Eina_List *l, *categories;
    Enna_Class_Vfs *cat;

    /* Create List */
    o = enna_list_add(enna->evas);

    categories = enna_vfs_get(ENNA_CAPS_PHOTO);
    EINA_LIST_FOREACH(categories, l, cat)
    {
        Enna_Vfs_File *item;

        item = calloc(1, sizeof(Enna_Vfs_File));
        item->icon = (char*)eina_stringshare_add(cat->icon);
        item->label = (char*)eina_stringshare_add(gettext(cat->label));
        item->is_menu = 1;
        enna_list_file_append(o, item, _browse, cat);
    }

    enna_list_select_nth(o, 0);
    mod->o_menu = o;
    edje_object_part_swallow(mod->o_edje, "browser.swallow", o);

}

static void _create_gui(void)
{
    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    mod->o_edje = edje_object_add(enna->evas);
    edje_object_file_set(mod->o_edje,
                         enna_config_theme_get(), "activity/photo");

    _create_menu();
    _create_infos();
}

static void photo_event_menu (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_LEFT:
    case ENNA_INPUT_EXIT:
        enna_content_hide();
        enna_mainmenu_show();
        break;
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_OK:
        _browse (enna_list_selected_data_get(mod->o_menu));
        break;
    default:
        enna_list_input_feed(mod->o_menu, event);
    }
}

static void photo_event_browser (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_INFO:
        panel_infos_display(1);
        break;
    default:
        enna_browser_input_feed(mod->o_browser, event);
        break;
    }
}

static void photo_event_info (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_EXIT:
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
    switch (event)
    {
    case ENNA_INPUT_EXIT:
        ENNA_OBJECT_DEL (mod->o_slideshow);
        mod->state = BROWSER_VIEW;
        edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
        edje_object_signal_emit(mod->o_edje, "list,show", "enna");
        break;
    default:
        break;
    }
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void _class_init(int dummy)
{
    _create_gui();
    enna_content_append(ENNA_MODULE_NAME, mod->o_edje);
}

static void _class_show(int dummy)
{
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
    edje_object_signal_emit(mod->o_edje, "content,show", "enna");
}

static void _class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void _class_event(enna_input event)
{
    int i;

    static const struct {
        PHOTO_STATE state;
        void (*event_handler) (enna_input event);
    } evh [] = {
        { MENU_VIEW,         &photo_event_menu        },
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
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
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

    enna_activity_add(&class);
}

static void
module_shutdown(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);
    ENNA_OBJECT_DEL(mod->o_infos);
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
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
