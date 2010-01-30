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

#include <string.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "xml_utils.h"
#include "url_utils.h"
#include "vfs.h"

#define ENNA_MODULE_NAME "shoutcast"

#define SHOUTCAST_URL      "http://www.shoutcast.com"
#define SHOUTCAST_GENRE    "shoutcast://"
#define SHOUTCAST_LIST     "http://www.shoutcast.com/sbin/newxml.phtml"
#define SHOUTCAST_STATION  "http://www.shoutcast.com/sbin/newxml.phtml?genre="
#define MAX_URL 1024

typedef struct Enna_Module_Music_s
{
    Evas *e;
    url_t handler;
} Enna_Module_Music;

static Enna_Module_Music *mod;

static Eina_List * browse_list(void)
{
    url_data_t chunk;
    xmlDocPtr doc;
    xmlNode *list, *n;
    Eina_List *files = NULL;

    chunk = url_get_data(mod->handler, SHOUTCAST_LIST);
    if (!chunk.buffer)
        return NULL;

    doc = get_xml_doc_from_memory (chunk.buffer);
    if (!doc)
        return NULL;

    list = get_node_xml_tree(xmlDocGetRootElement(doc), "genrelist");
    for (n = list->children; n; n = n->next)
    {
        Enna_Vfs_File *file;
        xmlChar *genre;
        char *uri;

        if (!xmlHasProp(n, (xmlChar *) "name"))
            continue;

        genre = xmlGetProp(n, (xmlChar *) "name");

        uri = malloc(strlen(SHOUTCAST_GENRE) + strlen((char *) genre) + 1);
        sprintf(uri, "%s%s", SHOUTCAST_GENRE, genre);

        file = enna_vfs_create_directory(uri, (const char *) genre, "icon/webradio",
                NULL);
        ENNA_FREE(uri);
        files = eina_list_append(files, file);
    }

    free(chunk.buffer);
    xmlFreeDoc(doc);

    return files;
}

static Eina_List * browse_by_genre(const char *path)
{
    url_data_t chunk;
    xmlDocPtr doc;
    xmlNode *list, *n;
    char url[MAX_URL];
    Eina_List *files = NULL;
    xmlChar *tunein = NULL;
    const char *genre = path + strlen(SHOUTCAST_GENRE);

    memset(url, '\0', MAX_URL);
    snprintf(url, MAX_URL, "%s%s", SHOUTCAST_STATION, genre);
    chunk = url_get_data(mod->handler, url);

    doc = xmlReadMemory(chunk.buffer, chunk.size, NULL, NULL, 0);
    if (!doc)
        return NULL;

    list = get_node_xml_tree(xmlDocGetRootElement(doc), "stationlist");
    for (n = list->children; n; n = n->next)
    {
        if (!n->name)
            continue;

        if (!xmlStrcmp(n->name, (xmlChar *) "tunein"))
        {
            if (!xmlHasProp(n, (xmlChar *) "base"))
                continue;

            tunein = xmlGetProp(n, (xmlChar *) "base");
            continue;
        }

        if (!xmlStrcmp(n->name, (xmlChar *) "station"))
        {
            Enna_Vfs_File *file;
            xmlChar *id, *name;
            char uri[MAX_URL];

            if (!xmlHasProp(n, (xmlChar *) "name") || !xmlHasProp(n,
                    (xmlChar *) "id") || !tunein)
                continue;

            name = xmlGetProp(n, (xmlChar *) "name");
            id = xmlGetProp(n, (xmlChar *) "id");
            memset(uri, '\0', MAX_URL);
            snprintf(uri, MAX_URL, "%s%s?id=%s", SHOUTCAST_URL, tunein, id);

            file = enna_vfs_create_file(uri, (const char *) name, "icon/music", NULL);
            files = eina_list_append(files, file);
        }
    }

    free(chunk.buffer);
    xmlFreeDoc(doc);

    return files;
}

static Eina_List * _class_browse_up(const char *path, void *cookie)
{
    if (!path)
        return browse_list();

    if (strstr(path, SHOUTCAST_GENRE))
        return browse_by_genre(path);

    return NULL;
}

static Eina_List * _class_browse_down(void *cookie)
{
    return browse_list();
}

static Enna_Vfs_File * _class_vfs_get(void *cookie)
{
    return enna_vfs_create_directory(NULL, NULL,
            eina_stringshare_add("icon/music"), NULL);
}

static Enna_Class_Vfs class_shoutcast =
{
    "shoutcast",
    10,
    N_("SHOUTcast streaming"),
    NULL,
    "icon/shoutcast",
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

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_shoutcast
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_shoutcast",
    N_("SHOUTcast browser"),
    "icon/shoutcast",
    N_("Listen to SHOUTcast stream"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Music));
    em->mod = mod;

    mod->handler = url_new();

    enna_vfs_append("shoutcast", ENNA_CAPS_MUSIC, &class_shoutcast);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    url_free (mod->handler);
}
