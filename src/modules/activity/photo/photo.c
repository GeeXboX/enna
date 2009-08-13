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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "vfs.h"
#include "view_wall.h"
#include "image.h"
#include "browser.h"
#include "slideshow.h"
#include "view_list.h"
#include "content.h"
#include "mainmenu.h"
#include "event_key.h"
#include "logs.h"
#include "photo.h"
#include "exif.h"
#include "panel_infos.h"

#define ENNA_MODULE_NAME "photo"

static void _create_menu();
static void _create_gui();
static void _browser_root_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_selected_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_browse_down_cb (void *data, Evas_Object *obj, void *event_info);
static void _browse(void *data);

typedef struct _Enna_Module_Photo
{
    Evas *e;
    Evas_Object *o_edje;
    Evas_Object *o_menu;
    Evas_Object *o_browser;
    Evas_Object *o_infos;
    Evas_Object *o_slideshow;
    PHOTO_STATE state;
    Enna_Module *em;
    photo_exif_t *exif;
    int infos_displayed;
} Enna_Module_Photo;

static Enna_Module_Photo *mod;

/****************************************************************************/
/*                             Photo Helpers                                */
/****************************************************************************/

static void _delete_infos()
{
    ENNA_OBJECT_DEL(mod->o_infos);
}

static void _create_infos()
{  
    mod->o_infos = enna_panel_infos_add (evas_object_evas_get(mod->o_edje));
    edje_object_part_swallow (mod->o_edje,
                              "infos.panel.swallow", mod->o_infos);   
    edje_object_signal_emit (mod->o_edje, "infos,hide", "enna");
}

static void
panel_infos_display (int show)
{
    if (show)
    {
        edje_object_signal_emit (mod->o_edje, "infos,show", "enna");
        mod->infos_displayed = 1;
        mod->state = INFOS_VIEW;  
    }
    else
    {
        edje_object_signal_emit (mod->o_edje, "infos,hide", "enna");
        mod->infos_displayed = 0;
        mod->state = BROWSER_VIEW;  
    }
}

/* #############################################################
   #               slideshow helpers                           #
   ############################################################# */

static void _create_slideshow_gui()
{
    Evas_Object *o;

    mod->state = SLIDESHOW_VIEW;

    ENNA_OBJECT_DEL (mod->o_slideshow);

    o = enna_slideshow_add(mod->em->evas);
    edje_object_part_swallow(enna->o_edje, "enna.swallow.fullscreen", o);
    evas_object_show(o);
    mod->o_slideshow = o;

    edje_object_signal_emit(mod->o_edje, "list,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "wall,hide", "enna");
}


void _slideshow_add_files()
{
    Eina_List *files = NULL;
    files = enna_browser_files_get (mod->o_browser);
    enna_slideshow_append_list (mod->o_slideshow, files);
    eina_list_free(files);
}


static void
_browser_root_cb (void *data, Evas_Object *obj, void *event_info)
{
    mod->state = MENU_VIEW;
    evas_object_smart_callback_del(mod->o_browser, "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser, "selected", _browser_selected_cb);
    evas_object_smart_callback_del(mod->o_browser, "browse_down", _browser_browse_down_cb);

    /* Delete objects */
    ENNA_OBJECT_DEL(mod->o_browser);
    edje_object_signal_emit(mod->o_edje, "browser,hide", "enna");

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

    if (!ev || !ev->file) return;

    if (!ev->file->is_directory)
    {
        /* File is selected, display it in slideshow mode */
        _create_slideshow_gui();
        _slideshow_add_files();
        enna_slideshow_set(mod->o_slideshow, ev->file->uri + 7);
    }
    free(ev);
}

static void
_browser_hilight_cb (void *data, Evas_Object *obj, void *event_info)
{
    Browser_Selected_File_Data *ev = event_info;

     if (!ev || !ev->file)
        return;
        
     if (!ev->file->is_directory)
        enna_panel_infos_set_cover(mod->o_infos, ev->file->uri + 7);
     
     enna_panel_infos_set_text(mod->o_infos, ev->file->uri + 7);
}


static void _browse(void *data)
{
    Enna_Class_Vfs *vfs = data;

    if(!vfs) return;

    mod->o_browser = enna_browser_add(mod->em->evas);

    enna_browser_view_add (mod->o_browser, ENNA_BROWSER_VIEW_WALL);

    evas_object_smart_callback_add (mod->o_browser, "root", _browser_root_cb, NULL);
    evas_object_smart_callback_add (mod->o_browser, "selected", _browser_selected_cb, NULL);
    evas_object_smart_callback_add (mod->o_browser, "browse_down", _browser_browse_down_cb, NULL);
    evas_object_smart_callback_add (mod->o_browser, "hilight", _browser_hilight_cb, NULL);

    mod->state = BROWSER_VIEW;

    evas_object_show(mod->o_browser);
    edje_object_part_swallow(mod->o_edje, "enna.swallow.browser", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);

    edje_object_signal_emit(mod->o_edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "browser,show", "enna");
//    edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
    edje_object_signal_emit(mod->o_edje, "slideshow,hide", "enna");

    ENNA_OBJECT_DEL (mod->o_menu);
}

static void
_create_menu()
{
    Evas_Object *o;
    Eina_List *l, *categories;
    Enna_Class_Vfs *cat;

    /* Create List */
    o = enna_list_add(mod->em->evas);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");

    categories = enna_vfs_get(ENNA_CAPS_PHOTO);
    EINA_LIST_FOREACH(categories, l, cat)
    {
        Enna_Vfs_File *item;

        item = calloc(1, sizeof(Enna_Vfs_File));
        item->icon = (char*)eina_stringshare_add(cat->icon);
        item->label = (char*)eina_stringshare_add(gettext(cat->label));
        enna_list_file_append(o, item, _browse, cat);
    }

    enna_list_select_nth(o, 0);
    mod->o_menu = o;
    edje_object_part_swallow(mod->o_edje, "enna.swallow.menu", o);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
}

static void _create_gui(void)
{
    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    mod->o_edje = edje_object_add(mod->em->evas);
    edje_object_file_set(mod->o_edje, enna_config_theme_get(), "module/photo");

    _create_menu();
    _create_infos();
}

static void photo_event_menu (void *event_info, enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_LEFT:
    case ENNA_KEY_CANCEL:
        enna_content_hide ();
        enna_mainmenu_show (enna->o_mainmenu);
        break;
    case ENNA_KEY_RIGHT:
    case ENNA_KEY_OK:
    case ENNA_KEY_SPACE:
        _browse (enna_list_selected_data_get(mod->o_menu));
        break;
    default:
        enna_list_event_feed(mod->o_menu, event_info);
    }
}

static void photo_event_browser (void *event_info, enna_key_t key)
{
    switch (key)
    {
	case ENNA_KEY_I:
	    panel_infos_display(1);
	    break;
    default:
        enna_browser_event_feed(mod->o_browser, event_info);
	}
}

static void photo_event_info (void *event_info, enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_CANCEL:
    case ENNA_KEY_I:
        panel_infos_display(0);
        break;
    case ENNA_KEY_OK:
        _create_slideshow_gui();
        _slideshow_add_files();
        break;
    default:
        break;
    }
}

static void photo_event_slideshow (void *event_info, enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_CANCEL:
        ENNA_OBJECT_DEL (mod->o_slideshow);
        mod->state = BROWSER_VIEW;
        edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
        edje_object_signal_emit(mod->o_edje, "list,show", "enna");
        break;
    case ENNA_KEY_RIGHT:
        enna_slideshow_next(mod->o_slideshow);
        break;
    case ENNA_KEY_LEFT:
        enna_slideshow_prev(mod->o_slideshow);
        break;
    case ENNA_KEY_OK:
    case ENNA_KEY_SPACE:
        enna_slideshow_play(mod->o_slideshow);
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
    enna_content_append("photo", mod->o_edje);
}

static void _class_show(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
}

static void _class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void _class_event(void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key(ev);
    int i;

    static const struct {
        PHOTO_STATE state;
        void (*event_handler) (void *event_info, enna_key_t key);
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
            evh[i].event_handler (event_info, key);
            break;
        }
}

static Enna_Class_Activity class =
{
    "photo",
    1,
    N_("Photo"),
    NULL,
    "icon/photo",
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

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_ACTIVITY,
    "activity_photo"
};

void module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Photo));
    mod->em = em;
    em->mod = mod;
    mod->exif = calloc (1, sizeof (photo_exif_t));

    enna_activity_add(&class);
}

void module_shutdown(Enna_Module *em)
{
    _delete_infos();
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
    ENNA_OBJECT_DEL(mod->o_browser);
    free(mod);
}
