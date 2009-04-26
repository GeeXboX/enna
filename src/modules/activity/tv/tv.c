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

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "logs.h"
#include "content.h"
#include "mainmenu.h"
#include "mediaplayer.h"

#define ENNA_MODULE_NAME "tv"

typedef struct _Enna_Module_Tv
{
    Evas *e;
    Evas_Object *o_background;
    Enna_Playlist *enna_playlist;
    Enna_Module *em;
} Enna_Module_Tv;

static Enna_Module_Tv *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _class_init(int dummy)
{
    mod->o_background = evas_object_rectangle_add(mod->em->evas);
    evas_object_color_set(mod->o_background, 0, 0, 0, 255);
    enna_content_append("tv", mod->o_background);
}

static void _class_show(int dummy)
{
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "starting playback");
    enna_mediaplayer_play(mod->enna_playlist);
    evas_object_show(mod->o_background);
}

static void _class_hide(int dummy)
{
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "stopping playback");
    enna_mediaplayer_stop();
    evas_object_hide(mod->o_background);
}

static void _class_event(void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key(ev);

    switch (key)
    {
        case ENNA_KEY_MENU:
            enna_content_hide();
            enna_mainmenu_show(enna->o_mainmenu);
            break;
        default:
            enna_mediaplayer_send_key(key);
            break;
    }
}

static Enna_Class_Activity class =
{
    "tv",
    1,
    "Television",
    NULL,
    "icon/tv",
    {
    _class_init,
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
    "activity_tv"
};

void module_init(Enna_Module *em)
{
    Enna_Config_Data *cfgdata;
    char *value = NULL;

    char *vdr_uri = NULL;

    if (!em)
        return;

    if (!enna_mediaplayer_supported_uri_type(ENNA_MP_URI_TYPE_VDR) ||
        !enna_mediaplayer_supported_uri_type(ENNA_MP_URI_TYPE_NETVDR))
        return;

    /* Load Config file values */
    cfgdata = enna_config_module_pair_get("tv");

    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "parameters:");

    if (cfgdata)
    {
        Eina_List *l;

        for (l = cfgdata->pair; l; l = l->next)
        {
            Config_Pair *pair = l->data;

            if (!strcmp("vdr_uri", pair->key))
            {
                enna_config_value_store(&value, "vdr_uri",
                                        ENNA_CONFIG_STRING, pair);

                if(value)
                {
                    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                             " * vdr_uri: %s", value);
                    vdr_uri = value;
                }
            }
        }
    }

    if (!value)
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 " * use all parameters by default");

    if (!vdr_uri)
    {
        vdr_uri = "vdr:/";
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 "   - no vdr_uri found, using 'vdr:/' instead");
    }

    mod = calloc(1, sizeof(Enna_Module_Tv));
    mod->em = em;
    em->mod = mod;

    mod->enna_playlist = enna_mediaplayer_playlist_create();
    enna_mediaplayer_uri_append(mod->enna_playlist, vdr_uri, "vdr");

    enna_activity_add(&class);
}

void module_shutdown(Enna_Module *em)
{
    if (!mod)
        return;

    ENNA_OBJECT_DEL(mod->o_background);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
}
