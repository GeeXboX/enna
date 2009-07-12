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
#include "photo.h"
#include "item_class.h"

static char *
func_label_get (const void *data, Evas_Object *obj, const char *part)
{
    const Photo_Item_Class_Data *item = data;

    return (item && item->label) ? strdup (item->label) : NULL;
}

static Evas_Object *
func_icon_get (const void *data, Evas_Object *obj, const char *part)
{
    const Photo_Item_Class_Data *item = data;
    Evas_Object *ic;

    if (!item)
        return NULL;

    if (strcmp (part, "elm.swallow.icon"))
        return NULL;

    ic = elm_icon_add (obj);
    elm_icon_file_set (ic, enna_config_theme_get (), item->icon);
    evas_object_size_hint_min_set (ic, 64, 64);
    evas_object_show (ic);

    return ic;
}

static Evas_Bool
func_state_get (const void *data, Evas_Object *obj, const char *part)
{
    return 0;
}

static void
func_del (const void *data, Evas_Object *obj)
{
    /* nothing to do */
}

Elm_Genlist_Item_Class *
photo_item_class_new (void)
{
    Elm_Genlist_Item_Class *c;

    c = calloc (1, sizeof (Elm_Genlist_Item_Class));

    c->item_style     = "default";
    c->func.label_get = func_label_get;
    c->func.icon_get  = func_icon_get;
    c->func.state_get = func_state_get;
    c->func.del       = func_del;

    return c;
}
