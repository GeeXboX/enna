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
#include "activity.h"
#include "logs.h"
#include "module.h"
#include "content.h"
#include "mainmenu.h"
#include "view_list.h"
#include "vfs.h"
#include "url_utils.h"
#include "xml_utils.h"
#include "utils.h"
#include "image.h"

#define ENNA_MODULE_NAME                 "bookstore"

typedef struct _Enna_Module_BookStore {
  Evas *e;
  Evas_Object *edje;
} Enna_Module_BookStore;

static Enna_Module_BookStore *mod;

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
_class_init (int dummy)
{
    enna_content_append(ENNA_MODULE_NAME, mod->edje);
}

static void
_class_shutdown (int dummy)
{

}

static void
_class_show (int dummy)
{
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->edje, "module,show", "enna");
    edje_object_signal_emit(mod->edje, "content,show", "enna");
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit(mod->edje, "module,hide", "enna");
}

static void
_class_event (enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed BookStore : %d", event);
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("BookStore"),
    NULL,
    "icon/bookstore",
    "background/bookstore",
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
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);
    ENNA_FREE(mod);
}
