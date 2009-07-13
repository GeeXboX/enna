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
#include "wall.h"
#include "image.h"
#include "browser.h"
#include "slideshow.h"
#include "list.h"
#include "content.h"
#include "mainmenu.h"
#include "event_key.h"
#include "logs.h"
#include "photo.h"
#include "item_class.h"

#ifdef BUILD_LIBEXIF
#include <libexif/exif-data.h>
#endif

#define ENNA_MODULE_NAME "photo"

static void _create_menu();
static void _create_gui();
static void _browser_root_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_selected_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_browse_down_cb (void *data, Evas_Object *obj, void *event_info);
static void _browse(void *data);

static void _photo_info_fs();

typedef struct _Enna_Module_Photo
{
    Evas *e;
    Evas_Object *o_edje;
    Evas_Object *o_menu;
    Evas_Object *o_browser;
    Evas_Object *o_wall;
    Evas_Object *o_preview;
    Evas_Object *o_slideshow;
    Ecore_Timer *sel_timer;
    PHOTO_STATE state;
    Enna_Module *em;
#ifdef BUILD_LIBEXIF
    struct {
        Evas_Object *o_scroll;
        Evas_Object *o_exif;
        char *str;
    }exif;

#endif
    Elm_Genlist_Item_Class *item_class;
} Enna_Module_Photo;

static Enna_Module_Photo *mod;

/*****************************************************************************/
/*                              Photo Helpers                                */
/*****************************************************************************/

static void _photo_info_delete_cb(void *data,
    Evas_Object *obj,
    const char *emission,
    const char *source)
{
    Evas_Object *o_pict;

    edje_object_signal_callback_del(mod->o_preview, "done", "", _photo_info_delete_cb);
    o_pict = edje_object_part_swallow_get(mod->o_preview, "enna.swallow.content");

#ifdef BUILD_LIBEXIF
    ENNA_OBJECT_DEL(mod->exif.o_exif);
    ENNA_OBJECT_DEL(mod->exif.o_scroll);
    free(mod->exif.str);
    mod->exif.str = NULL;
#endif

    ENNA_OBJECT_DEL(o_pict);
    ENNA_OBJECT_DEL(mod->o_preview);
    mod->state = WALL_VIEW;
}

static void _photo_info_delete()
{
    edje_object_signal_callback_add(mod->o_preview, "done","", _photo_info_delete_cb, NULL);
    edje_object_signal_emit(mod->o_preview, "hide", "enna");
    edje_object_signal_emit(mod->o_preview, "hide,exif", "enna");
    edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
}

#ifdef BUILD_LIBEXIF
static void _exif_content_foreach_func(ExifEntry *entry, void *callback_data)
{
  char buf[2000];
  char buf_txtblk[2000];
  char *exif_str;
  size_t len;

  exif_entry_get_value(entry, buf, sizeof(buf));
  snprintf(buf_txtblk, sizeof(buf_txtblk), "<hilight>%s</hilight> : %s<br>",
         exif_tag_get_name(entry->tag),
         exif_entry_get_value(entry, buf, sizeof(buf)));

  len = strlen(buf_txtblk) + 1;
  if (mod->exif.str)
      len += strlen(mod->exif.str);

  exif_str = calloc(len, sizeof(char));
  if (mod->exif.str)
      strcpy(exif_str, mod->exif.str);
  strcat(exif_str, buf_txtblk);
  free(mod->exif.str);
  mod->exif.str = exif_str;

}

static void _exif_data_foreach_func(ExifContent *content, void *callback_data)
{
  exif_content_foreach_entry(content, _exif_content_foreach_func, callback_data);
}


/** Run EXIF parsing test on the given file. */

static void _exif_parse_metadata(const char *filename)
{
  ExifData *d;
  Evas_Coord mw, mh;

  if (!filename) return;

  if (mod->exif.str)
      free(mod->exif.str);

  mod->exif.str = NULL;

  ENNA_OBJECT_DEL (mod->exif.o_exif);

  mod->exif.o_exif = edje_object_add(mod->em->evas);
  edje_object_file_set(mod->exif.o_exif, enna_config_theme_get(), "exif/data");
  mod->exif.o_scroll = elm_scroller_add(mod->o_edje);
  edje_object_part_swallow(mod->o_preview, "enna.swallow.exif", mod->exif.o_scroll);
  d = exif_data_new_from_file(filename);
  exif_data_foreach_content(d, _exif_data_foreach_func, NULL);
  exif_data_unref(d);

  if (!mod->exif.str)
      mod->exif.str=strdup(_("No exif information found."));

  edje_object_part_text_set(mod->exif.o_exif, "enna.text.exif", mod->exif.str);
  edje_object_size_min_calc(mod->exif.o_exif, &mw, &mh);
  evas_object_resize(mod->exif.o_exif, mw, mh);
  evas_object_size_hint_min_set(mod->exif.o_exif, mw, mh);
  elm_scroller_content_set(mod->exif.o_scroll, mod->exif.o_exif);
}
#endif


static void
_flip_picture (int way)
{
    Evas_Object *obj;

    int w = way ? ELM_IMAGE_FLIP_HORIZONTAL : ELM_IMAGE_FLIP_VERTICAL;

    obj = edje_object_part_swallow_get(mod->o_preview, "enna.swallow.content");
    if (obj)
        elm_image_orient_set (obj, w);
}

static int
_show_sel_image(void *data)
{
    const char *filename = data;
    Evas_Object *o_pict;
    Evas_Coord x1,y1,w1,h1, x2,y2,w2,h2;
    Evas_Coord xi,yi,wi,hi, xf,yf,wf,hf;
    Edje_Message_Int_Set *msg;
    Evas_Object *o_edje;

    /* Prepare edje message */
    msg = calloc(1,sizeof(Edje_Message_Int_Set) - sizeof(int) + (4 * sizeof(int)));
    msg->count = 4;

    enna_wall_selected_geometry_get(mod->o_wall, &x2, &y2, &w2, &h2);

    o_pict = elm_image_add(mod->o_edje);
    elm_image_file_set(o_pict, filename, NULL);
    elm_image_no_scale_set(o_pict, 1);
    elm_image_smooth_set(o_pict, 1);
    elm_image_scale_set(o_pict, 1, 1);

    o_edje = edje_object_add(mod->em->evas);
    edje_object_file_set(o_edje, enna_config_theme_get(), "enna/picture/info");
    edje_object_part_swallow(o_edje, "enna.swallow.content", o_pict);

    /* Set Final state in fullscreen */
    edje_object_part_swallow(mod->o_edje, "enna.swallow.slideshow", o_edje);
    evas_object_geometry_get(mod->o_edje, &x1, &y1, &w1, &h1);
    hf = h1;
    wf = hf * (float)w2 / (float)h2;
    xf = w1 / 2 - wf / 2;
    yf = h1 / 2 - hf / 2;

    msg->val[0] = xf;
    msg->val[1] = yf;
    msg->val[2] = wf;
    msg->val[3] = hf;

    edje_object_message_send(o_edje, EDJE_MESSAGE_INT_SET, 2, msg);

    /* Set custom state : size and position of actual thumbnail */
    xi = x2 - x1;
    yi = y2 - y1;
    wi = w2;
    hi = h2;
    msg->val[0] = xi;
    msg->val[1] = yi;
    msg->val[2] = wi;
    msg->val[3] = hi;
    edje_object_message_send(o_edje, EDJE_MESSAGE_INT_SET, 1, msg);
    free(msg);

    mod->o_preview = o_edje;
#ifdef BUILD_LIBEXIF
    _exif_parse_metadata(filename);
#endif
    edje_object_signal_emit(mod->o_preview, "show", "enna");
    mod->sel_timer = NULL;
    return 0;
}

static void _photo_info_fs()
{
    const char *filename;
    Evas_Object *o_pict;

    filename = enna_wall_selected_filename_get(mod->o_wall);
    if (!filename) return;

    o_pict = edje_object_part_swallow_get(mod->o_preview, "enna.swallow.content");
    if (o_pict) //user clicked too fast, preview already there or in progress
        return;

    mod->state = WALL_PREVIEW;
    edje_object_signal_emit(mod->o_edje, "wall,hide", "enna");
    mod->sel_timer = ecore_timer_add(0.01, _show_sel_image, filename);
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

    //edje_object_signal_emit(mod->o_edje, "slideshow,show", "enna");
    edje_object_signal_emit(mod->o_edje, "list,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "wall,hide", "enna");
}

void _slideshow_add_files()
{
    Eina_List *files;

    files = enna_wall_get_filenames (mod->o_wall);
    enna_slideshow_append_list (mod->o_slideshow, files);
    eina_list_free(files);
}

static void
_picture_selected_cb (void *data, Evas_Object *obj, void *event_info)
{
    _photo_info_fs();
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
    evas_object_smart_callback_del(mod->o_wall, "selected", _picture_selected_cb);
    ENNA_OBJECT_DEL(mod->o_wall);
    edje_object_signal_emit(mod->o_edje, "wall,hide", "enna");
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

    if (ev->file->is_directory)
    {
        ENNA_OBJECT_DEL(mod->o_wall);
        mod->o_wall = enna_wall_add(mod->em->evas);
        evas_object_smart_callback_add(mod->o_wall, "selected", _picture_selected_cb, NULL);

        evas_object_show(mod->o_wall);

        EINA_LIST_FOREACH(ev->files, l, f)
        {
            if (!f->is_directory)
            {
                enna_wall_picture_append(mod->o_wall, f->uri);
            }
            else
            {
                count++;
            }
        }
        edje_object_part_swallow(mod->o_edje, "enna.swallow.wall", mod->o_wall);
        edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
        enna_wall_select_nth(mod->o_wall, 0, 0);

        if (!count)
        {
            edje_object_signal_emit(mod->o_edje, "browser,hide", "enna");
            mod->state = WALL_VIEW;
	}

    }

    free(ev);
}

static void _browse(void *data)
{
    Enna_Class_Vfs *vfs = data;

    if(!vfs) return;

    mod->o_browser = enna_browser_add(mod->em->evas);
    enna_browser_show_file_set(mod->o_browser, 0);
    evas_object_smart_callback_add(mod->o_browser, "root", _browser_root_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "selected", _browser_selected_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "browse_down", _browser_browse_down_cb, NULL);

    mod->state = BROWSER_VIEW;

    evas_object_show(mod->o_browser);
    edje_object_part_swallow(mod->o_edje, "enna.swallow.browser", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);

    edje_object_signal_emit(mod->o_edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "browser,show", "enna");
    edje_object_signal_emit(mod->o_edje, "wall,show", "enna");
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
        Photo_Item_Class_Data *item;

        item = calloc(1, sizeof(Photo_Item_Class_Data));
        item->icon = eina_stringshare_add(cat->icon);
        item->label = eina_stringshare_add(gettext(cat->label));
        enna_list_append(o, mod->item_class, item, item->label, _browse, cat);
    }

    enna_list_selected_set(o, 0);
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
        enna_list_event_key_down(mod->o_menu, event_info);
    }
}

static void photo_event_browser (void *event_info, enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_RIGHT:
    case ENNA_KEY_LEFT:
        mod->state = WALL_VIEW;
        edje_object_signal_emit(mod->o_edje, "browser,hide", "enna");
        break;
    default:
        enna_browser_event_feed(mod->o_browser, event_info);
    }
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
        _photo_info_fs();
        break;
    case ENNA_KEY_RIGHT:
    case ENNA_KEY_LEFT:
    case ENNA_KEY_UP:
    case ENNA_KEY_DOWN:
        enna_wall_event_feed(mod->o_wall, event_info);
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
#ifdef BUILD_LIBEXIF
    case ENNA_KEY_UP:
        edje_object_signal_emit(mod->o_preview, "show,exif", "enna");
        break;
    case ENNA_KEY_DOWN:
        edje_object_signal_emit(mod->o_preview, "hide,exif", "enna");
        break;
#endif
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
        enna_slideshow_set (mod->o_slideshow,
                            enna_wall_selected_filename_get (mod->o_wall));
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

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

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

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

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

    mod->item_class = photo_item_class_new ();
    enna_activity_add(&class);
}

void module_shutdown(Enna_Module *em)
{
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_wall);
    ENNA_OBJECT_DEL(mod->o_menu);
    ENNA_OBJECT_DEL(mod->o_browser);
    free(mod);
}
