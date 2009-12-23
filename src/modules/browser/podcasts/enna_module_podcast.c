/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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

/* Interface */

#include "enna.h"
#include "url_utils.h"
#include "xml_utils.h"

#define ENNA_MODULE_NAME "podcast"

#define PATH_PODCASTS    "podcasts"


typedef struct _Channel
{
    char *url;
    unsigned char active : 1;
    char *title;
    char *link;
    char *description;
    char *cover_url;
    char *author;
    char *category;


}Channel;


typedef struct _Enna_Module_Podcast
{
    Evas                *e;
    Enna_Module         *em;
    Eina_List           *channels;
    Ecore_Event_Handler *volume_add_handler;
    Ecore_Event_Handler *volume_remove_handler;
    CURL                *curl;
} Enna_Module_Podcast;

static Enna_Module_Podcast *mod;


static Eina_List *_class_browse_up(const char *path, void *cookie)
{
    Enna_Vfs_File *f;
    Eina_List *l = NULL;
    f = enna_vfs_create_file("podcast://", _("Play"), "icon/video", NULL);
    l = eina_list_append(l, f);
    f = enna_vfs_create_file("podcastnav://", _("Play (With Menus)"), "icon/video", NULL);
    l = eina_list_append(l, f);
    return l;
}


/*
 * Timer function : check for updates each 1 our
 *
 */


/*
 * Create channel members from xml Node
 */
static void xml_to_channel(xmlNode *node, Channel *channel)
{
    xmlChar *tmp;
    xmlNode *n, *cover;


    if (!node || !channel) return;

    n = get_node_xml_tree(node, "channel");

    tmp = get_prop_value_from_xml_tree (n , "title");
    if (tmp) channel->title = strdup ((char *) tmp);
    if (tmp) enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	" --- Title : %s", tmp);

    tmp = get_prop_value_from_xml_tree (n , "link");
    if (tmp) channel->link = strdup ((char *) tmp);
    if (tmp) enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	" --- Link : %s", tmp);

    tmp = get_prop_value_from_xml_tree (n , "description");
    if (tmp) channel->description = strdup ((char *) tmp);
    if (tmp) enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	" --- Description : %s", tmp);

    tmp = get_prop_value_from_xml_tree (n, "author");
    if (tmp) channel->author = strdup ((char *) tmp);
    if (tmp) enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	" --- itunes:author : %s", tmp);

    tmp = get_prop_value_from_xml_tree (n, "category");
    if (tmp) channel->category = strdup ((char *) tmp);
    if (tmp) enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	" --- itunes:category : %s", tmp);

    cover = get_node_xml_tree (n, "image");
    tmp = get_prop_value_from_xml_tree (cover,"url");
    if (tmp) channel->cover_url = strdup ((char *) tmp);
    if (tmp) enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	" --- Image : url %s", tmp);

}


/*
 * Prepare channel :
 * Create podcast directory
 * Download podcast file
 */

static void _prepare_channel(Channel *ch)
{
    char  dst[1024];
    char  file[1024];
    char *md5;
    xmlDocPtr doc = NULL;

    if (!ch || !ch->url || !ch->active) return;

    /* Compute MD5 Sum based on url */
    md5 = md5sum (ch->url);

    /* Create Channel directory if not existing*/
    snprintf (dst, sizeof (dst), "%s/.enna/%s/%s/",
	enna_util_user_home_get(), PATH_PODCASTS, md5);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* Download and save xml file */
    snprintf (file, sizeof (file), "%s/channel.xml", dst);

    enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	"Downloading podcast channel : %s", ch->url);
    url_save_to_disk (mod->curl, ch->url, file);

    doc = xmlReadFile(file, NULL, 0);
    if (!doc)
	goto error;


    xml_to_channel(xmlDocGetRootElement(doc), ch);
    snprintf (file, sizeof (file), "%s/cover.png", dst);
    url_save_to_disk (mod->curl, ch->cover_url, file);


error:
    if (doc)
        xmlFreeDoc (doc);
    /* clean up */
    free (md5);

}


/*
 * Read Configuration
 * Add streams found in configuration file in streams list
 */

static void _read_configuration()
{
    Enna_Config_Data *cfg;
    Eina_List *l;
    Config_Pair *pair;

    cfg = enna_config_module_pair_get("podcast");
    if (!cfg)
        return;

    EINA_LIST_FOREACH(cfg->pair, l, pair)
    {
	if (!strcmp(pair->key, "stream"))
        {
            Eina_List *dir_data;
            enna_config_value_store(&dir_data, "stream",
		ENNA_CONFIG_STRING_LIST, pair);

            if (dir_data)
            {
                if (eina_list_count(dir_data) != 2)
                    continue;
                else
                {
                    Channel *ch;

                    ch = calloc(1, sizeof(Channel));
                    ch->url = strdup(eina_list_nth(dir_data, 0));
		    ch->active = atoi(eina_list_nth(dir_data, 1));
		    if (ch->active)
			enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
			    "Podcast stream found : %s", ch->url);
		    _prepare_channel(ch);
		    mod->channels = eina_list_append(mod->channels, ch);
                }
            }
        }
    }
}

static Eina_List * _class_browse_down(void *cookie)
{
    return NULL;
}

static Enna_Vfs_File * _class_vfs_get(void *cookie)
{

    return NULL;
}


static int _add_volumes_cb(void *data, int type, void *event)
{
    Enna_Volume *v =  event;
    if (!strcmp(v->type, "podcast://"))
    {

    }
    return 1;
}

static int _remove_volumes_cb(void *data, int type, void *event)
{
    Enna_Volume *v = event;

    if (!strcmp(v->type, "podcast://"))
    {

    }
    return 1;
}


static Enna_Class_Vfs class_podcast = {
    "podcast_podcast",
    1,
    N_("Watch PODCAST Video"),
    NULL,
    "icon/dev/podcast",
    {
	NULL,
	NULL,
	_class_browse_up,
	_class_browse_down,
	_class_vfs_get,
    },
    NULL
};

/* Module interface */

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "browser_podcast"
};

void module_init(Enna_Module *em)
{
    char dst[1024];

    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Podcast));
    mod->em = em;
    em->mod = mod;

    curl_global_init (CURL_GLOBAL_DEFAULT);
    mod->curl = curl_easy_init ();

    enna_vfs_append ("podcast", ENNA_CAPS_VIDEO, &class_podcast);

    mod->volume_add_handler =
        ecore_event_handler_add (ENNA_EVENT_VOLUME_ADDED,
	    _add_volumes_cb, mod);
    mod->volume_remove_handler =
        ecore_event_handler_add (ENNA_EVENT_VOLUME_REMOVED,
	    _remove_volumes_cb, mod);



    /* try to create podcasts directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/.enna/%s",
              enna_util_user_home_get (), PATH_PODCASTS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* read configuration file */
    _read_configuration();
}

void module_shutdown(Enna_Module *em)
{
    Enna_Module_Podcast *mod;
    Channel *ch;

    mod = em->mod;

    /* Clean up channels list */
    while(mod->channels)
    {
	ch = mod->channels->data;
	mod->channels = eina_list_remove_list(mod->channels, mod->channels);
	free(ch->url);
	free(ch);
    }
    eina_list_free(mod->channels);

    /* Cleanup CURL */
    if (mod->curl)
        curl_easy_cleanup (mod->curl);
    curl_global_cleanup ();

    /* Clean up module */
    free(mod);
}
