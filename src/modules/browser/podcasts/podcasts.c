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

/* Interface */

#include <string.h>
#include <unistd.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "url_utils.h"
#include "vfs.h"
#include "logs.h"
#include "xml_utils.h"
#include "utils.h"

#define ENNA_MODULE_NAME "podcast"

typedef enum browser_level {
    PODCAST_BROWSER_LEVEL_ROOT,
    PODCAST_BROWSER_LEVEL_CHANNELS,
} podcast_browser_level_t;

typedef struct _Enna_Module_Podcast {
    Evas *e;
    url_t *handler;
    podcast_browser_level_t level;
} Enna_Module_Podcast;

typedef struct podcast_stream_s {
    char *uri;
    char *name;
} podcast_stream_t;

typedef struct podcast_cfg_s {
    Eina_List *streams;
} podcast_cfg_t;

static podcast_cfg_t podcast_cfg;
static Enna_Module_Podcast *mod;

static podcast_stream_t *
podcast_stream_new (const char *name, const char *uri)
{
    podcast_stream_t *s;

    if (!name || !uri)
        return NULL;

    s       = calloc(1, sizeof(podcast_stream_t));
    s->name = strdup(name);
    s->uri  = strdup(uri);
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Adding new podcast: %s - %s", name, uri);

    return s;
}

static void
podcast_stream_free (podcast_stream_t *s)
{
    if (!s)
        return;

    ENNA_FREE(s->name);
    ENNA_FREE(s->uri);
    ENNA_FREE(s);
}

static Eina_List *
podcast_browse_channel (const char *uri)
{
    xmlDocPtr doc;
    url_data_t chunk;
    xmlNode *n, *channel;
    Eina_List *list = NULL;

    if (!uri)
        return NULL;

    /* perform web request */
    chunk = url_get_data(mod->handler, (char *) uri);

    /* parse it */
    doc = xmlReadMemory(chunk.buffer, chunk.size, NULL, NULL, 0);
    if (!doc)
        goto err_doc;

    channel = get_node_xml_tree(xmlDocGetRootElement(doc), "channel");
    if (!channel)
        goto err_parser;

    for (n = channel->children; n; n = n->next)
    {
        if (!n)
            continue;

        if (n->next && !xmlStrcmp(n->next->name, (const xmlChar *) "item"))
        {
            xmlChar *title;
            xmlChar *link;

            title = get_prop_value_from_xml_tree(n, "title");
            link  = get_attr_value_from_xml_tree(n, "enclosure", "url");

            if (title && link)
            {
                Enna_File *f;

                enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                         "New podcast item: %s at %s", title, link);
                f = enna_vfs_create_file((const char *) link,
                                         (const char *) title,
                                         "icon/music", NULL);
                list = eina_list_append(list, f);
            }

            if (title)
                xmlFree(title);
            if (link)
                xmlFree(link);
        }
    }


err_parser:
    if (doc)
        xmlFreeDoc(doc);

err_doc:
    ENNA_FREE(chunk.buffer);

    return list;
}

static Eina_List *
podcast_browse_feeds (const char *name)
{
    podcast_stream_t *s;
    Eina_List *l;

    if (!name)
        return NULL;

    mod->level = PODCAST_BROWSER_LEVEL_CHANNELS;
    EINA_LIST_FOREACH(podcast_cfg.streams, l, s)
        if (!strcmp(s->name, name))
            return podcast_browse_channel(s->uri);

    return NULL;
}

static Eina_List *
podcast_browse_streams (void)
{
    Eina_List *l, *list = NULL;
    podcast_stream_t *s;

    EINA_LIST_FOREACH(podcast_cfg.streams, l, s)
    {
        Enna_File *f;

	f = enna_vfs_create_directory(s->name, s->name, NULL, NULL);
	list = eina_list_append(list, f);
    }

    return list;
}

static Eina_List *
_class_browse_up (const char *path, void *cookie)
{
    mod->level = PODCAST_BROWSER_LEVEL_ROOT;
    return path ? podcast_browse_feeds(path) : podcast_browse_streams();
}

static Eina_List *
_class_browse_down (void *cookie)
{
    if (mod->level == PODCAST_BROWSER_LEVEL_CHANNELS)
    {
	mod->level = PODCAST_BROWSER_LEVEL_ROOT;
	return podcast_browse_streams();
    }

    return NULL;
}

static Enna_File * _class_vfs_get(void *cookie)
{
    return enna_vfs_create_directory(NULL, NULL, NULL, NULL);
}

static Enna_Vfs_Class class_podcast = {
    "podcast_podcast",
    1,
    N_("Listen to podcasts"),
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
    podcast_stream_t *s;

    EINA_LIST_FREE(podcast_cfg.streams, s)
        podcast_stream_free(s);
}

static void
cfg_podcast_section_load (const char *section)
{
    Eina_List *sl;

    cfg_podcast_free();

    if (!section)
        return;

    sl = enna_config_string_list_get(section, "stream");
    if (sl)
    {
        Eina_List *l;
        char *c;

        EINA_LIST_FOREACH(sl, l, c)
        {
            Eina_List *tuple;

            tuple = enna_util_tuple_get(c, ",");

            if (tuple && eina_list_count(tuple) == 2)
            {
                podcast_stream_t *s;
                const char *name, *uri;

                name = eina_list_nth(tuple, 0);
                uri  = eina_list_nth(tuple, 1);

                s = podcast_stream_new(name, uri);
                if (s)
                    podcast_cfg.streams =
                        eina_list_append(podcast_cfg.streams, s);
            }
        }
    }
}

static void
cfg_podcast_section_save (const char *section)
{
    podcast_stream_t *s;
    Eina_List *l, *sl = NULL;

    if (!section)
        return;

    EINA_LIST_FOREACH(podcast_cfg.streams, l, s)
    {
        char v[1024];

        snprintf(v, sizeof(v), "%s,%s", s->name, s->uri);
        sl = eina_list_append(sl, strdup(v));
    }

    enna_config_string_list_set(section, "stream", sl);
}

static void
cfg_podcast_section_set_default (void)
{
    cfg_podcast_free();

    podcast_cfg.streams = NULL;
}

static Enna_Config_Section_Parser cfg_podcast = {
    "podcast",
    cfg_podcast_section_load,
    cfg_podcast_section_save,
    cfg_podcast_section_set_default,
    cfg_podcast_free,
};

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_podcasts
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    enna_config_section_parser_register(&cfg_podcast);
//    cfg_podcast_section_set_default();
//    cfg_podcast_section_load(cfg_podcast.section);

    mod = calloc(1, sizeof(Enna_Module_Podcast));
    em->mod = mod;

    mod->handler = url_new();

    enna_vfs_append("podcast", ENNA_CAPS_MUSIC, &class_podcast);
}

static void
module_shutdown(Enna_Module *em)
{
    Enna_Module_Podcast *mod;

    mod = em->mod;
    url_free(mod->handler);
    ENNA_FREE(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_podcast",
    N_("Podcast browser"),
    "icon/podcast",
    N_("Podcast module"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
