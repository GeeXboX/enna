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

#include <Ecore.h>

#include "browser2.h"
#include "enna.h"
#include "activity.h"
#include "buffer.h"
#include "logs.h"
#include "utils.h"
#include "vfs.h"

typedef enum _Enna_Browser_Type
{
    BROWSER_ROOT,
    BROWSER_ACTIVITY,
    BROWSER_MODULE,
}Enna_Browser_Type;

struct _Enna_Browser
{

    Ecore_Idler *queue_idler;
    void (*add)(Enna_Vfs_File *file, void *data);
    void (*del)(Enna_Vfs_File *file, void *data);
    void *add_data;
    void *del_data;
    const char *uri;
    Enna_Browser_Type type;
    Ecore_Event_Handler *ev_handler;
    Eina_List *tokens;

};

Eina_List *enna_browser2_browse_activity(Enna_Browser* browser, const char* act_name);
Eina_List *enna_browser2_browse_module(Enna_Browser* browser);
static Eina_List *_enna_activities = NULL;

int _add_idler(void *data)
{
    Enna_Browser* b = data;
    DBG("uri : %s\n", b->uri);
    switch (b->type)
    {
    case BROWSER_ROOT:
        enna_browser2_browse_root(b);
        break;
    case BROWSER_ACTIVITY:
	enna_browser2_browse_activity(b, "music");
	break;
    case BROWSER_MODULE:
	enna_browser2_browse_module(b);
	break;
    default:
        break;

    }
    b->queue_idler = NULL;
    return EINA_FALSE;

}

static int
_activities_changed_cb(void *data, int type, void *event)
{
    Enna_Browser *browser = data;
    Enna_Vfs_File *file = event;

    if (!file || !browser)
        return ECORE_CALLBACK_RENEW;

    if (browser->add)
        browser->add(file, browser->add_data);


    return ECORE_CALLBACK_RENEW;
}

Enna_Browser *
enna_browser2_add( void (*add)(Enna_Vfs_File *file, void *data), void *add_data,
                   void (*del)(Enna_Vfs_File *file, void *data), void *del_data,
                   const char *uri)
{
    Enna_Browser *b;

    b = malloc(sizeof(Enna_Browser));
    b->add = add;
    b->del = del;
    b->add_data = add_data;
    b->del_data = del_data;
    b->queue_idler = NULL;
    b->uri = eina_stringshare_add(uri);
    b->tokens = NULL;
    b->tokens = enna_util_tuple_get(uri, "/");

    if (!b->tokens || eina_list_count(b->tokens) == 0)
    {
        b->type = BROWSER_ROOT;
        b->ev_handler = ecore_event_handler_add(ENNA_EVENT_BROWSER_CHANGED,
                                                  _activities_changed_cb, b);
        printf("0 token, %s\n", eina_list_nth(b->tokens, 0));
    }
    else if (eina_list_count(b->tokens) == 1)
    {
        b->type = BROWSER_ACTIVITY;
    }
    else if (eina_list_count(b->tokens) >= 2)
    {
        b->type = BROWSER_MODULE;
    }
    else
        return NULL;

    b->queue_idler = ecore_idler_add(_add_idler, b);
    return b;
}

void
enna_browser2_del(Enna_Browser *b)
{
    if (!b)
        return;
    if (b->queue_idler)
    ecore_idler_del(b->queue_idler);
      b->queue_idler = NULL;
    eina_stringshare_del(b->uri);
//     if (b->ev_handler)
// 	ecore_event_handler_del(b->ev_handler);
    free(b);
}

Enna_Vfs_File *
enna_browser2_get_file(const char *uri)
{
    return NULL;
}

Eina_List *enna_browser2_browse(Enna_Browser *browser, const char *uri)
{
    return NULL;
}

void
enna_browser2_browse_root(Enna_Browser *browser)
{
    Eina_List *l;
    Enna_Class_Activity *act;
    buffer_t *buf;
    Enna_Vfs_File *f;
    EINA_LIST_FOREACH(enna_activities_get(), l, act)
    {
        f = calloc(1, sizeof(Enna_Vfs_File));

        buf = buffer_new();
        buffer_appendf(buf, "/%s", act->name);
	f->name = act->name;
        f->uri = eina_stringshare_add(buf->buf);
        buffer_free(buf);
        f->label = eina_stringshare_add(act->label);
        f->icon = eina_stringshare_add(act->icon);
        f->icon_file = eina_stringshare_add(act->bg);
        f->is_menu = 1;
        if (browser->add)
            browser->add(f, browser->add_data);

    }
}

Eina_List *
enna_browser2_browse_activity(Enna_Browser *browser, const char *act_name)
{
    
    if (!strcmp(act_name, "music"))
    {
	Enna_Class_Vfs *vfs;
	Eina_List *l;
	Enna_Vfs_File *f;
	buffer_t *buf;
	EINA_LIST_FOREACH(enna_vfs_get(ENNA_CAPS_MUSIC), l, vfs)
	{
	    f = calloc(1, sizeof(Enna_Vfs_File));

	    buf = buffer_new();
	    buffer_appendf(buf, "/music/%s", vfs->name);
	    f->name = vfs->name;
	    f->uri = eina_stringshare_add(buf->buf);
	    buffer_free(buf);
	    f->label = eina_stringshare_add(vfs->label);
	    f->icon = eina_stringshare_add(vfs->icon);
	    f->is_menu = 1;
	    if (browser->add)
		browser->add(f, browser->add_data);
	}
    }
    
    return NULL;

}

Eina_List *
enna_browser2_browse_module(Enna_Browser *browser)
{
    Enna_Class2_Vfs *vfs = NULL, *tmp = NULL;
    Eina_List *l;
    const char *name=  (const char*)eina_list_nth(browser->tokens, 1) ;
    
    EINA_LIST_FOREACH(enna_vfs_get(ENNA_CAPS_MUSIC), l, tmp)
    {

	if (!strcmp(tmp->name, name))
	{
	    vfs = tmp;
	    break;
	}
    }
    if (!vfs)
	return NULL;
    
    Eina_List *children;
    Enna_Vfs_File *child;
    children = vfs->func.get_children(eina_list_nth_list(browser->tokens, 2));
    EINA_LIST_FOREACH(children , l, child)
    {
	 if (browser->add)
		browser->add(child, browser->add_data);
    }
    
}

const char *
enna_browser2_uri_get(Enna_Browser *b)
{
    if(!b)
	return NULL;
    return b->uri;
}