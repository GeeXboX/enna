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

#include "enna.h"
#include "vfs.h"
#include "module.h"
#include "buffer.h"

#define ENNA_MODULE_NAME "jamendo"

static Ecore_Timer *timer = NULL;
static int count = 0;

static int
_timeout_cb(void *data)
{
    Enna_File *f3;
    Enna_Browser *browser = data;
    char uri[32];
    char name[32];
    
    snprintf(name, sizeof(name), "item%d", count);
    snprintf(uri, sizeof(uri), "/music/jamendo/item%d", count);
    printf("%s %s\n", uri, name);
    f3 = enna_browser_create_menu(name, uri, "toto", "icon/artist");
    enna_browser_file_add(browser, f3);
    
    count++;
    return 1;
}

static void *
_add(Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    timer = ecore_timer_add(1.0, _timeout_cb, browser);
    
    return NULL;
}

static void
_get_children(void *priv, Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    Enna_File *f1;
    Enna_File *f2;
    Enna_Buffer *buf;
    Enna_Buffer *uri;
    Eina_List *l;
    char *p;
    
    buf = enna_buffer_new();
    uri = enna_buffer_new();
    
    EINA_LIST_FOREACH(tokens, l, p)
    enna_buffer_appendf(buf, "/%s", p);
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "artist");
    
    f1 = enna_browser_create_menu("artist", uri->buf, "Artists", "icon/artist");
    enna_buffer_free(uri);
    uri = enna_buffer_new();
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "artist");
    
    f2 = enna_browser_create_menu("album", uri->buf, "Albums", "icon/album");
    enna_buffer_free(uri);
    enna_buffer_free(buf);
    
    enna_browser_file_add(browser, f1);
    enna_browser_file_add(browser, f2);
}

static void
_del(void *priv)
{
    ecore_timer_del(timer);
    count = 0;
    timer = NULL;
}

static Enna_Vfs_Class class = {
    "jamendo",
                        1,
                        N_("Browse Jamendo music library"),
                        NULL,
                        "icon/library",
                        {
                            _add,
                            _get_children,
                            _del
                        },
                        NULL
};


/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_jamendo
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;
    
    enna_vfs_register(&class, ENNA_CAPS_MUSIC);
}

static void
module_shutdown(Enna_Module *em)
{
    
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_jamendo",
    N_("Jamendo Music Library"),
    "icon/hd",
    N_("Browse files of jamendo music library"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

