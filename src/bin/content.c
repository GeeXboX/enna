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

#include "activity.h"
#include "enna_config.h"
#include "content.h"

typedef struct _Enna_Content_Element Enna_Content_Element;

struct _Enna_Content_Element
{
    const char *name;
    Evas_Object *content;
    unsigned char selected : 1;
};

static Eina_List *_enna_contents = NULL;


/* externally accessible functions */

int
enna_content_append(const char *name, Evas_Object *content)
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

int
enna_content_select(const char *name)
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
        enna_activity_hide(prev->name);
        elm_layout_content_unset(enna->layout, "enna.content.swallow");
    }
    if (new)
    {
        elm_layout_content_set(enna->layout, "enna.content.swallow", new->content);
    }

    return 0;
}

/* local subsystem functions */

static void
enna_content_update(int show)
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
            {
                enna_mainmenu_hide(enna->o_menu);
                enna_activity_show(e->name);
            }
            else
            {
                enna_activity_hide(e->name);
                enna_mainmenu_show(enna->o_menu);
            }
            return;
        }
    }
}

void
enna_content_hide(void)
{
   enna_content_update(0);
}

void
enna_content_show(void)
{
   enna_content_update(1);
}
