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
#include "bookstore.h"
#include "bookstore_gocomics.h"
#include "bookstore_onemanga.h"

#define ENNA_MODULE_NAME                 "bookstore"

typedef enum _BookStore_State
{
    BS_MENU_VIEW,
    BS_SERVICE_VIEW,
} BookStore_State;

typedef struct _Enna_Module_BookStore {
    Evas *e;
    Evas_Object *edje;
    Evas_Object *menu;
    Evas_Object *service_bg;
    Eina_List *menu_items;
    BookStore_State state;
    BookStore_Service *current;
    BookStore_Service *gocomics;
    BookStore_Service *onemanga;
} Enna_Module_BookStore;

static Enna_Module_BookStore *mod;

/****************************************************************************/
/*                      BookStore Service API                               */
/****************************************************************************/

static void
bs_service_show (BookStore_Service *s)
{
    Evas_Object *obj = NULL;
    Evas_Object *old_img;

    if (!s)
        return;

    if (s->show)
        obj = (s->show)(mod->edje);
    if (!obj)
        return;

    old_img = mod->service_bg;
    mod->service_bg = edje_object_add(evas_object_evas_get(mod->edje));
    edje_object_file_set(mod->service_bg, enna_config_theme_get(), s->bg);
    edje_object_part_swallow(mod->edje, "service.bg.swallow", mod->service_bg);
    evas_object_show(mod->service_bg);
    evas_object_del(old_img);

    edje_object_part_swallow(mod->edje, "content.swallow", obj);
    edje_object_signal_emit(mod->edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->edje, "service,show", "enna");

    mod->state = BS_SERVICE_VIEW;
    mod->current = s;
}

static void
bs_service_hide (BookStore_Service *s)
{
    if (!s)
        return;

    if (s && s->hide)
        (s->hide)(mod->edje);

    evas_object_hide(mod->service_bg);
    edje_object_part_swallow(mod->edje, "service.bg.swallow", NULL);
    mod->current = NULL;
    mod->state = BS_MENU_VIEW;

    edje_object_signal_emit(mod->edje, "service,hide", "enna");
    edje_object_signal_emit(mod->edje, "menu,show", "enna");
}

/****************************************************************************/
/*                         BookStore Menu API                               */
/****************************************************************************/

static void
bs_menu_item_cb_selected (void *data)
{
    BookStore_Service *s = data;
    bs_service_show(s);
}

static void
bs_menu_add (BookStore_Service *s)
{
    Enna_Vfs_File *f;

    if (!s)
        return;

    f          = calloc (1, sizeof(Enna_Vfs_File));
    f->icon    = (char *) eina_stringshare_add(s->icon);
    f->label   = (char *) eina_stringshare_add(s->label);
    f->is_menu = 1;

    enna_wall_file_append(mod->menu, f, bs_menu_item_cb_selected, s);
    mod->menu_items = eina_list_append (mod->menu_items, f);
}

static void
bs_menu_create (void)
{
    mod->menu = enna_wall_add(enna->evas);

    bs_menu_add(mod->gocomics);
    bs_menu_add(mod->onemanga);

    enna_wall_select_nth(mod->menu, 0, 0);
    edje_object_part_swallow(mod->edje, "menu.swallow", mod->menu);
    mod->state = BS_MENU_VIEW;
}

static void
bs_menu_delete (void)
{
    Enna_Vfs_File *f;

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
_class_show (int dummy)
{
    /* create the activity content once for all */
    if (!mod->edje)
    {
        mod->edje = edje_object_add(enna->evas);
        edje_object_file_set(mod->edje, enna_config_theme_get(),
                             "activity/bookstore");
        enna_content_append(ENNA_MODULE_NAME, mod->edje);
    }

    /* create the menu, once for all */
    if (!mod->menu)
        bs_menu_create();

    /* show module */
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->edje, "menu,show", "enna");
    edje_object_signal_emit(mod->edje, "module,show", "enna");
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit(mod->edje, "menu,hide", "enna");
    edje_object_signal_emit(mod->edje, "module,hide", "enna");
    bs_service_hide(mod->current);
}

static void
_class_event (enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed BookStore : %d", event);

    switch (mod->state)
    {
    /* Menu View */
    case BS_MENU_VIEW:
    {
        if (event == ENNA_INPUT_EXIT)
        {
            enna_content_hide();
            enna_mainmenu_show();
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
            b = (mod->current->event)(mod->edje, event);

        if ((b == ENNA_EVENT_CONTINUE) && (event == ENNA_INPUT_EXIT))
        {
            bs_service_hide(mod->current);
        }
        break;
    }
    default:
        break;
    }
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("BookStore"),
    NULL,
    "icon/bookstore",
    "background/bookstore",
    {
        NULL,
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
#define MOD_PREFIX enna_mod_activity_bookstore
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_bookstore",
    N_("BookStore"),
    "icon/bookstore",
    N_("Read your books"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    mod = calloc (1, sizeof(Enna_Module_BookStore));
    em->mod = mod;

    enna_activity_add(&class);

    mod->gocomics = &bs_gocomics;
    mod->onemanga = &bs_onemanga;
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);

    ENNA_OBJECT_DEL(mod->edje);
    ENNA_OBJECT_DEL(mod->service_bg);
    bs_menu_delete();
    ENNA_FREE(mod);
}
