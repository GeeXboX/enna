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

#include <string.h>
#include <unistd.h>

#include <Ecore_File.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "url_utils.h"
#include "vfs.h"
#include "logs.h"
#include "xml_utils.h"
#include "utils.h"
#include "volumes.h"
#include "xdg.h"

#define ENNA_MODULE_NAME "podcast"

#define PATH_PODCASTS    "podcasts"

typedef enum browser_level
{
    BROWSER_LEVEL_ROOT,
    BROWSER_LEVEL_CHANNELS,
    BROWSER_LEVEL_ITEMS
} browser_level_t;

typedef struct _Item
{
    char *title;
    char *link;
    char *description;
    char *author;
    char *category;
    char *pub_date;
    char *url;
}Item;

typedef struct _Channel
{
    char *url;
    char *title;
    char *link;
    char *description;
    char *language;
    char *copyright;
    char *last_data;
    char *generator;
    char *cover;
    Eina_List *items;


}Channel;

typedef struct _Enna_Module_Podcast
{
    Evas                 *e;
    Enna_Module          *em;
    Eina_List            *channels;
    Ecore_Event_Handler  *volume_add_handler;
    Ecore_Event_Handler  *volume_remove_handler;
    url_t                *handler;
    browser_level_t       level;
    int                   prev_id;
} Enna_Module_Podcast;

typedef struct podcast_cfg_s {
    Eina_List *list;
} podcast_cfg_t;

static podcast_cfg_t podcast_cfg;
static Enna_Module_Podcast *mod;



/*
 * Timer function : check for updates each 1 our
 *
 */


#define GET_PROP_VALUE(var, prop)					\
    tmp = get_prop_value_from_xml_tree (n , prop);			\
    if (tmp) var = strdup ((char *) tmp);				\
    if (tmp) enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,		\
	" --- " prop " : %s", tmp);					\

/*
 * Create channel members from xml Node
 */
static void xml_to_channel(xmlNode *node, Channel *channel)
{
    xmlChar *tmp;
    xmlNode *n, *cover, *items;
    Item *item;

    if (!node || !channel) return;

    n = get_node_xml_tree(node, "channel");

    GET_PROP_VALUE(channel->title, "title");
    GET_PROP_VALUE(channel->link, "link");
    GET_PROP_VALUE(channel->description, "description");
    GET_PROP_VALUE(channel->language, "language");
    GET_PROP_VALUE(channel->copyright, "copyright");
    GET_PROP_VALUE(channel->last_data, "lastBuildDate");
    GET_PROP_VALUE(channel->generator, "generator");

    cover = get_node_xml_tree (n, "image");
    GET_PROP_VALUE(channel->cover, "url");

    items = get_node_xml_tree(node, "item");
    for (n = items; n; n = n->next)
    {
	if (!n) break;
	item = calloc(sizeof(Item), 1);
	GET_PROP_VALUE(item->title, "title");
	GET_PROP_VALUE(item->link, "link");
	GET_PROP_VALUE(item->description, "description");
	GET_PROP_VALUE(item->author, "author");
	GET_PROP_VALUE(item->category, "category");
	GET_PROP_VALUE(item->pub_date, "pubDate");
	GET_PROP_VALUE(item->url, "guid");
	if (!item->url)
	{
	    free(item);
	    continue;
	}
	channel->items = eina_list_append(channel->items, item);
    }
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

    if (!ch || !ch->url) return;

    /* Compute MD5 Sum based on url */
    md5 = md5sum (ch->url);

    /* Create Channel directory if not existing*/
    snprintf (dst, sizeof (dst), "%s/%s/%s/",
	enna_cache_home_get(), PATH_PODCASTS, md5);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* Download and save xml file */
    snprintf (file, sizeof (file), "%s/channel.xml", dst);

    enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
	"Downloading podcast channel : %s", ch->url);
    url_save_to_disk (mod->handler, ch->url, file);

    doc = xmlReadFile(file, NULL, 0);
    if (!doc)
	goto error;

    xml_to_channel(xmlDocGetRootElement(doc), ch);
    snprintf (file, sizeof (file), "%s/%s", dst, ecore_file_file_get(ch->cover));
    url_save_to_disk (mod->handler, ch->cover, file);
    free(ch->cover);
    ch->cover = strdup(file);


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
    Eina_List *l;

    if (!podcast_cfg.list)
        return;

    for (l = podcast_cfg.list; l; l = l->next)
    {
        Channel *ch;

        if (!l->data)
          continue;

        ch = calloc(1, sizeof(Channel));
        ch->url = strdup(l->data);
        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 "Podcast stream found : %s", ch->url);
        _prepare_channel(ch);
        mod->channels = eina_list_append(mod->channels, ch);
    }
}

static Eina_List *_browse_root()
{
    Eina_List *list = NULL;
    Eina_List *l;
    Channel *ch;
    char str[64];
    Enna_Vfs_File *f;
    int i = 0;

    mod->level = BROWSER_LEVEL_ROOT;

    EINA_LIST_FOREACH(mod->channels, l, ch)
    {
	snprintf(str, sizeof(str), "0/%i", i);
	f = enna_vfs_create_directory(str, ch->title, ch->cover, NULL);
	list = eina_list_append(list, f);
	i++;
    }
    return list;
}

static Eina_List *_browse_channels(int id)
{
    Eina_List *files = NULL;
    Eina_List *l;
    Item *it;
    Enna_Vfs_File *f;
    Channel *ch;
    char title[4096];

    ch = eina_list_nth(mod->channels, id);

    EINA_LIST_FOREACH(ch->items, l, it)
    {
	snprintf(title, sizeof(title), "%s - %s", it->pub_date, it->title);
    	f = enna_vfs_create_file(it->url, title, "icon/music", NULL);
    	files = eina_list_append(files, f);
    }
    return files;
}

static Eina_List *_class_browse_up(const char *path, void *cookie)
{
    int id = 0;
    int rc = 0;

    mod->level = BROWSER_LEVEL_ROOT;

    if (!path)
	return _browse_root();

    rc = sscanf(path, "%i/%i", (int *)&mod->level, &id);

    if (rc != 2)
        return NULL;

    switch(mod->level)
    {
    case BROWSER_LEVEL_CHANNELS:
    case BROWSER_LEVEL_ROOT:
	mod->prev_id = id;
	mod->level = BROWSER_LEVEL_CHANNELS;
	return _browse_channels(id);
    default:
	break;
    }
    return NULL;
}

static Eina_List * _class_browse_down(void *cookie)
{
    switch(mod->level)
    {
    case BROWSER_LEVEL_CHANNELS:
	mod->level = BROWSER_LEVEL_ROOT;
	return _browse_root();
    default:
	break;
    }
    return NULL;
}


static Enna_Vfs_File * _class_vfs_get(void *cookie)
{
    char str[64];
    snprintf(str, sizeof(str), "%i/%i", mod->level, mod->prev_id);
    return enna_vfs_create_directory(str, NULL, NULL, NULL);
}


static Enna_Class_Vfs class_podcast = {
    "podcast_podcast",
    1,
    N_("Listen to Podcasts"),
    NULL,
    "icon/podcast",
    {
	NULL,
	NULL,
	_class_browse_up,
	_class_browse_down,
	_class_vfs_get,
    },
    NULL
};

static void
cfg_podcast_free (void)
{
    char *c;

    EINA_LIST_FREE(podcast_cfg.list, c)
      ENNA_FREE(c);
}

static void
cfg_podcast_section_load (const char *section)
{
    cfg_podcast_free();
    podcast_cfg.list = enna_config_string_list_get(section, "stream");
}

static void
cfg_podcast_section_set_default (void)
{
    cfg_podcast_free();

    podcast_cfg.list = NULL;
}

static Enna_Config_Section_Parser cfg_podcast = {
    "podcast",
    cfg_podcast_section_load,
    NULL,
    cfg_podcast_section_set_default,
    cfg_podcast_free,
};

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_podcasts
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_podcast",
    N_("Podcast browser"),
    "icon/podcast",
    N_("Podcast module"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    char dst[1024];

    if (!em)
        return;

    enna_config_section_parser_register(&cfg_podcast);
    cfg_podcast_section_set_default();
    cfg_podcast_section_load(cfg_podcast.section);

    mod = calloc(1, sizeof(Enna_Module_Podcast));
    mod->em = em;
    em->mod = mod;

    mod->handler = url_new();

    enna_vfs_append ("podcast", ENNA_CAPS_MUSIC, &class_podcast);

    /* try to create podcasts directory storage */
    memset (dst, '\0', sizeof (dst));
    snprintf (dst, sizeof (dst), "%s/%s",
              enna_cache_home_get(), PATH_PODCASTS);
    if (!ecore_file_is_dir (dst))
        ecore_file_mkdir (dst);

    /* read configuration file */
    _read_configuration();
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Module_Podcast *mod;
    Channel *ch;

    mod = em->mod;
    mod->level = BROWSER_LEVEL_ROOT;
    /* Clean up channels list */
    while(mod->channels)
    {
	ch = mod->channels->data;
	mod->channels = eina_list_remove_list(mod->channels, mod->channels);
	free(ch->url);
	free(ch);
    }
    eina_list_free(mod->channels);

    url_free (mod->handler);

    /* Clean up module */
    free(mod);
}
