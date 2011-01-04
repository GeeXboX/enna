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

#include <Ecore.h>
#include <Ecore_File.h>
#include <Elementary.h>
#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "view_wall.h"
#include "activity.h"
#include "logs.h"
#include "module.h"
#include "content.h"
#include "mainmenu.h"
#include "vfs.h"
#include "image.h"
#include "bookstore.h"
#include "bookstore_gocomics.h"

#define ENNA_MODULE_NAME                 "bookstore"

typedef enum _Bookstore_State
{
    BS_MENU_VIEW,
    BS_SERVICE_VIEW,
} Bookstore_State;

typedef struct _Enna_Module_Bookstore {
    Evas *e;
    Evas_Object *o_layout;
    Evas_Object *edje;
    Evas_Object *menu;
    Evas_Object *service_bg;
    Evas_Object *page;
    Eina_List *menu_items;
    Bookstore_State state;
    Bookstore_Service *current;
    Bookstore_Service *gocomics;
} Enna_Module_Bookstore;

static Enna_Module_Bookstore *mod;

/****************************************************************************/
/*                      Bookstore Service API                               */
/****************************************************************************/

void
bs_service_page_show (const char *file)
{
    Evas_Object *old;

    edje_object_signal_emit(mod->edje, "page,hide", "enna");

    if (!file)
    {
        edje_object_signal_emit(mod->edje, "page,hide", "enna");
        evas_object_del(mod->page);
        return;
    }

    old = mod->page;
    mod->page = enna_image_add(enna->evas);
    enna_image_fill_inside_set(mod->page, 1);
    enna_image_file_set(mod->page, file, NULL);

    elm_layout_content_set(mod->o_layout,
                        "service.book.page.swallow", mod->page);
    edje_object_signal_emit(mod->edje, "page,show", "enna");
    evas_object_del(old);
    evas_object_show(mod->page);
}

static void
bs_service_btn_prev_clicked_cb(void *data, Evas_Object *obj, void *ev)
{
    if (mod->current && mod->current->prev)
        (mod->current->prev)(data, obj, ev);
}

static void
bs_service_btn_next_clicked_cb(void *data, Evas_Object *obj, void *ev)
{
    if (mod->current && mod->current->next)
        (mod->current->next)(data, obj, ev);
}

static void
bs_service_ctrl_btn_add (const char *icon, const char *part,
                         void (*cb) (void *data, Evas_Object *obj, void *ev))
{
    Evas_Object *layout, *ic, *bt;

    layout = elm_layout_add(enna->layout);

    ic = elm_icon_add(layout);
    elm_icon_file_set(ic, enna_config_theme_get(), icon);
    elm_icon_scale_set(ic, 0, 0);
    evas_object_show(ic);

    bt = elm_button_add(layout);
    evas_object_smart_callback_add(bt, "clicked", cb, NULL);
    elm_button_icon_set(bt, ic);
    elm_object_style_set(bt, "mediaplayer");
    evas_object_size_hint_weight_set(bt, 0.0, 1.0);
    evas_object_size_hint_align_set(bt, 0.5, 0.5);
    evas_object_show(bt);

    elm_layout_content_set(mod->o_layout, part, bt);
}

static void
bs_service_set_bg (const char *bg)
{
    if (bg)
    {
        Evas_Object *old;

        old = mod->service_bg;
        mod->service_bg = elm_layout_add(mod->o_layout);
        elm_layout_file_set(mod->service_bg, enna_config_theme_get(), bg);
        elm_layout_content_set(mod->o_layout,
                            "service.bg.swallow", mod->service_bg);
        evas_object_show(mod->service_bg);
        evas_object_del(old);
    }
    else
    {
        evas_object_hide(mod->service_bg);
        elm_layout_file_set(mod->o_layout, "service.bg.swallow", NULL);
    }
}

static void
bs_service_show (Bookstore_Service *s)
{
    if (!s)
        return;

    if (s->show)
        (s->show)(mod->o_layout);

    bs_service_set_bg(s->bg);
    bs_service_ctrl_btn_add ("icon/mp_rewind",  "service.btn.prev.swallow",
                             bs_service_btn_prev_clicked_cb);
    bs_service_ctrl_btn_add ("icon/mp_forward", "service.btn.next.swallow",
                             bs_service_btn_next_clicked_cb);

    edje_object_signal_emit(mod->edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->edje, "service,show", "enna");

    mod->state = BS_SERVICE_VIEW;
    mod->current = s;
}

static void
bs_service_hide (Bookstore_Service *s)
{
    if (!s)
        return;

    if (s && s->hide)
        (s->hide)(mod->o_layout);

    mod->current = NULL;
    mod->state = BS_MENU_VIEW;

    bs_service_set_bg(NULL);
    edje_object_signal_emit(mod->edje, "service,hide", "enna");
    edje_object_signal_emit(mod->edje, "module,show", "enna");
    edje_object_signal_emit(mod->edje, "menu,show", "enna");
}

/****************************************************************************/
/*                         Bookstore Menu API                               */
/****************************************************************************/

static void
bs_menu_item_cb_selected (void *data)
{
    Bookstore_Service *s = data;
    bs_service_show(s);
}

static void
bs_menu_add (Bookstore_Service *s)
{
    Enna_File *f;

    if (!s)
        return;

    f          = calloc (1, sizeof(Enna_File));
    f->icon    = (char *) eina_stringshare_add(s->icon);
    f->label   = (char *) eina_stringshare_add(s->label);
    f->is_menu = 1;

    enna_wall_file_append(mod->menu, f, bs_menu_item_cb_selected, s);
    mod->menu_items = eina_list_append (mod->menu_items, f);
}

static void
bs_menu_create (void)
{
    mod->menu = enna_wall_add(mod->o_layout);

    bs_menu_add(mod->gocomics);

    enna_wall_select_nth(mod->menu, 0, 0);
    elm_layout_content_set(mod->o_layout, "menu.swallow", mod->menu);
    mod->state = BS_MENU_VIEW;
}

static void
bs_menu_delete (void)
{
    Enna_File *f;

    if (!mod->menu)
        return;

    EINA_LIST_FREE(mod->menu_items, f);
        enna_vfs_remove(f);

    ENNA_OBJECT_DEL(mod->menu);
}

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static void
_class_show(void)
{
    /* create the activity content once for all */
    if (!mod->o_layout)
    {
        mod->o_layout = elm_layout_add(enna->layout);
        elm_layout_file_set(mod->o_layout, enna_config_theme_get(),
                             "activity/bookstore");
        mod->edje = elm_layout_edje_get(mod->o_layout);
        enna_content_append(ENNA_MODULE_NAME, mod->o_layout);
    }

    /* create the menu, once for all */
    if (!mod->menu)
        bs_menu_create();

    /* show module */
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->edje, "module,show", "enna");
    edje_object_signal_emit(mod->edje, "menu,show", "enna");
}

static void
_class_hide(void)
{
    edje_object_signal_emit(mod->edje, "service,hide", "enna");
    edje_object_signal_emit(mod->edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->edje, "module,hide", "enna");
    bs_service_hide(mod->current);
}

static void
_class_event (enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed Bookstore : %d", event);

    switch (mod->state)
    {
    /* Menu View */
    case BS_MENU_VIEW:
    {
        if (event == ENNA_INPUT_BACK)
        {
            enna_content_hide();
        }
        else
            enna_wall_input_feed(mod->menu, event);
        break;
    }
    /* Service View */
    case BS_SERVICE_VIEW:
    {
        Eina_Bool b = ENNA_EVENT_BLOCK;
        if (mod->current && mod->current->event)
            b = (mod->current->event)(mod->o_layout, event);

        if ((b == ENNA_EVENT_CONTINUE) && (event == ENNA_INPUT_BACK))
            bs_service_hide(mod->current);
        break;
    }
    default:
        break;
    }
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    9,
    N_("Bookstore"),
    NULL,
    "icon/bookstore",
    "background/bookstore",
    ENNA_CAPS_NONE,
    {
        NULL,
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
#define MOD_PREFIX enna_mod_activity_bookstore
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    mod = calloc (1, sizeof(Enna_Module_Bookstore));
    em->mod = mod;

    enna_activity_register(&class);

    mod->gocomics = &bs_gocomics;
}

static void
module_shutdown(Enna_Module *em)
{
    enna_activity_unregister(&class);

    ENNA_OBJECT_DEL(mod->o_layout);
    ENNA_OBJECT_DEL(mod->page);
    ENNA_OBJECT_DEL(mod->service_bg);
    bs_menu_delete();
    ENNA_FREE(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_bookstore",
    N_("Bookstore"),
    "icon/bookstore",
    N_("Read your books"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
