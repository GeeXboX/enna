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
    Evas_Object *o_preview;
    Evas_Object *o_slideshow;
    Ecore_Timer *sel_timer;
    PHOTO_STATE state;
    Enna_Module *em;
    photo_exif_t *exif;
} Enna_Module_Photo;

static Enna_Module_Photo *mod;

/****************************************************************************/
/*                             Photo Helpers                                */
/****************************************************************************/

static void _photo_info_delete_cb(void *data,
    Evas_Object *obj,
    const char *emission,
    const char *source)
{
    Evas_Object *o_pict;

    edje_object_signal_callback_del(mod->o_preview, "done", "", _photo_info_delete_cb);
    o_pict = edje_object_part_swallow_get(mod->o_preview, "enna.swallow.content");

    photo_exif_free (mod->exif);
    ENNA_OBJECT_DEL(o_pict);
    ENNA_OBJECT_DEL(mod->o_preview);
    mod->state = WALL_VIEW;
}

static void _photo_info_delete()
{
    edje_object_signal_callback_add(mod->o_preview, "done","", _photo_info_delete_cb, NULL);
    edje_object_signal_emit(mod->o_preview, "hide", "enna");
    photo_exif_hide (mod->o_preview);
    edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
}

static void
_flip_picture (int way)
{
    Evas_Object *obj;

    int w = way ? ELM_IMAGE_FLIP_HORIZONTAL : ELM_IMAGE_FLIP_VERTICAL;

    obj = edje_object_part_swallow_get(mod->o_preview, "enna.swallow.content");
    if (obj)
        elm_image_orient_set (obj, w);
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
    /* FIXME files must be retrieved from browser */
    //files = enna_wall_get_filenames (mod->o_wall);
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
    Enna_Vfs_File *f;
    Eina_List *l;
    Browser_Selected_File_Data *ev = event_info;
    int count = 0;

    if (!ev || !ev->file) return;

    if (!ev->file->is_directory)
    {
        /* File is selected, display it in slideshow mode */

    }
    free(ev);
}

static void _browse(void *data)
{
    Enna_Class_Vfs *vfs = data;

    if(!vfs) return;

    mod->o_browser = enna_browser_add(mod->em->evas);

    enna_browser_view_add (mod->o_browser, ENNA_BROWSER_VIEW_WALL);

    evas_object_smart_callback_add(mod->o_browser, "root", _browser_root_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "selected", _browser_selected_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "browse_down", _browser_browse_down_cb, NULL);

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
    //switch (key)
    //{
	//case ENNA_KEY_RIGHT:
	//case ENNA_KEY_LEFT:
        //mod->state = WALL_VIEW;
        //edje_object_signal_emit(mod->o_edje, "browser,hide", "enna");
        //break;
    //default:
        enna_browser_event_feed(mod->o_browser, event_info);
	//}
}

static void photo_event_wall (void *event_info, enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_CANCEL:
        edje_object_signal_emit(mod->o_edje, "browser,show", "enna");
        mod->state = BROWSER_VIEW;
        break;
    case ENNA_KEY_OK:
    case ENNA_KEY_SPACE:
        break;
    case ENNA_KEY_RIGHT:
    case ENNA_KEY_LEFT:
    case ENNA_KEY_UP:
    case ENNA_KEY_DOWN:
        break;
    default:
        break;
    }
}

static void photo_event_preview (void *event_info, enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_CANCEL:
        _photo_info_delete();
        break;
    case ENNA_KEY_UP:
        photo_exif_show (mod->o_preview);
        break;
    case ENNA_KEY_DOWN:
        photo_exif_hide (mod->o_preview);
        break;
    case ENNA_KEY_RIGHT:
    case ENNA_KEY_LEFT:
        //FIXME: should be made possible to switch to prev/next pic right here
        _photo_info_delete();
        break;
    case ENNA_KEY_V:
        _flip_picture (0);
        break;
    case ENNA_KEY_H:
        _flip_picture (1);
        break;
    case ENNA_KEY_OK:
        _create_slideshow_gui();
        _slideshow_add_files();
        enna_slideshow_play(mod->o_slideshow);
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
        _photo_info_delete();
        ENNA_OBJECT_DEL (mod->o_slideshow);
        mod->state = WALL_VIEW;
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
        { WALL_VIEW,         &photo_event_wall        },
        { WALL_PREVIEW,      &photo_event_preview     },
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
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
    ENNA_OBJECT_DEL(mod->o_browser);
    free(mod);
}
