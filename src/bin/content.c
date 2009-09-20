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

#include "activity.h"
#include "enna_config.h"
#include "content.h"

Evas_Object *_content = NULL;

typedef struct _Enna_Content_Element Enna_Content_Element;

struct _Enna_Content_Element
{
    const char *name;
    Evas_Object *content;
    unsigned char selected : 1;
};

static Eina_List *_enna_contents = NULL;

/* local subsystem functions */

/* externally accessible functions */
Evas_Object *
enna_content_add(Evas *evas)
{
    Evas_Object *o;

    o = edje_object_add(evas);
    edje_object_file_set(o, enna_config_theme_get(), "content");
    _content = o;
    return o;
}

int enna_content_append(const char *name, Evas_Object *content)
{
    Eina_List *l;
    Enna_Content_Element *elem;

    if (!name || !content)
        return -1;
    for (l = _enna_contents; l; l = l ->next)
    {
        Enna_Content_Element *e;
        e = l->data;
        if (!e)
            continue;
        if (!strcmp(e->name, name))
            return -1;
    }
    elem = calloc(1, sizeof(Enna_Content_Element));
    elem->name = eina_stringshare_add(name);
    elem->content = content;
    elem->selected = 0;
    _enna_contents = eina_list_append(_enna_contents, elem);
    return 0;
}

int enna_content_select(const char *name)
{
    Eina_List *l;
    Enna_Content_Element *new = NULL;
    Enna_Content_Element *prev = NULL;

    if (!name)
        return -1;
    for (l = _enna_contents; l; l = l->next)
    {
        Enna_Content_Element *e;
        e = l->data;

        if (!e)
            continue;

        if (!strcmp(name, e->name))
        {
            new = e;
            e->selected = 1;
        }
        else if (e->selected)
        {
            prev = e;
            e->selected = 0;
        }
    }

    if (prev)
    {
        edje_object_part_unswallow(_content, prev->content);
        enna_activity_hide(prev->name);
    }
    if (new)
    {
        edje_object_part_swallow(_content, "enna.swallow.content", new->content);
    }

    return 0;
}

static void enna_content_update(int show)
{
    Eina_List *l;

    for (l = _enna_contents; l; l = l->next)
    {
        Enna_Content_Element *e;
        e = l->data;

        if (!e)
            continue;
        if (e->selected)
        {
            if (show)
               enna_activity_show(e->name);
            else
               enna_activity_hide(e->name);
            return;
        }
    }
}

void enna_content_hide(void)
{
   enna_content_update(0);
}

void enna_content_show(void)
{
   enna_content_update(1);
}
