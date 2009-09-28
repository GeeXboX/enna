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

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "content.h"
#include "mainmenu.h"
#include "module.h"
#include "buffer.h"
#include "event_key.h"

#ifdef BUILD_LIBSVDRP
#include "utils.h"
#endif

#ifdef BUILD_LIBXRANDR
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#endif

#include <player.h>

#ifdef BUILD_BROWSER_VALHALLA
#include <valhalla.h>
#endif

#define BUF_LEN 1024
#define BUF_DEFAULT "Unknown"
#define LSB_FILE "/etc/lsb-release"
#define DEBIAN_VERSION_FILE "/etc/debian_version"
#define DISTRIB_ID "DISTRIB_ID="
#define E_DISTRIB_ID "Distributor ID:"
#define E_RELEASE "Release:"
#define DISTRIB_ID_LEN strlen (DISTRIB_ID)
#define DISTRIB_RELEASE "DISTRIB_RELEASE="
#define DISTRIB_RELEASE_LEN strlen (DISTRIB_RELEASE)

#define ENNA_MODULE_NAME "infos"

#define STR_CPU "processor"
#define STR_MODEL "model name"
#define STR_MHZ "cpu MHz"

#define STR_MEM_TOTAL "MemTotal:"
#define STR_MEM_ACTIVE "Active:"

typedef struct _Enna_Module_Infos {
    Evas *e;
    Evas_Object *edje;
    Enna_Module *em;
    Evas_Object o_list;
} Enna_Module_Infos;

static Enna_Module_Infos *mod;


/****************************************************************************/
/*                        Event Callbacks                                   */
/****************************************************************************/

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
_crate_menu()
{
    Enna_Vfs_File *item;
    
    ENNA_OBJECT_DEL(mod->o_list);
    mod->o_list = enna_list_add(evas_object_evas_get(mod->o_edje));
    
    edje_object_part_swallo(sd->o_edje, "enna.swallow.list", mod->o_list);
            

    item = calloc(1, sizeof(Enna_Vfs_File));
    item->icon = (char*)eina_stringshare_add("icon/infos");
    item->label = (char*)eina_stringshare_add(_("Infos"));
    
    enna_list_file_append(o, item, NULL, NULL);
    
}

static void
create_gui (void)
{
    mod->edje = edje_object_add (mod->em->evas);
    edje_object_file_set (mod->edje,
                          enna_config_theme_get (), "module/infos");
    
    _create_menu();
                          
}

static void
_class_init (int dummy)
{
    create_gui ();
    enna_content_append (ENNA_MODULE_NAME, mod->edje);
}

static void
_class_shutdown (int dummy)
{
    ENNA_OBJECT_DEL (mod->edje);
}

static void
_class_show (int dummy)
{
    edje_object_signal_emit (mod->edje, "content,show", "enna");
    edje_object_signal_emit (mod->edje, "content,show", "enna");
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit (mod->edje, "module,hide", "enna");
    edje_object_signal_emit (mod->edje, "content,hide", "enna");
}

static void
_class_event (void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key (ev);

    if (key != ENNA_KEY_CANCEL)
        return;

    enna_content_hide ();
    enna_mainmenu_show (enna->o_mainmenu);
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("Configuration"),
    NULL,
    "icon/infos",
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

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "activity_configuration"
};

void
module_init (Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_Infos));
    mod->em = em;
    em->mod = mod;

    enna_activity_add (&class);
}

void
module_shutdown (Enna_Module *em)
{
    evas_object_del (mod->edje);
    free (mod);
}

