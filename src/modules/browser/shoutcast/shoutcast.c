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

#include "enna.h"
#include "xml_utils.h"
#include "url_utils.h"

#define ENNA_MODULE_NAME "shoutcast"

#define SHOUTCAST_URL      "http://www.shoutcast.com"
#define SHOUTCAST_GENRE    "shoutcast://"
#define SHOUTCAST_LIST     "http://www.shoutcast.com/sbin/newxml.phtml"
#define SHOUTCAST_STATION  "http://www.shoutcast.com/sbin/newxml.phtml?genre="
#define MAX_URL 1024

typedef struct Enna_Module_Music_s
{
    Evas *e;
    Enna_Module *em;
    CURL *curl;
} Enna_Module_Music;

static Enna_Module_Music *mod;

static Eina_List * browse_list(void)
{
    url_data_t chunk;
    xmlDocPtr doc;
    xmlNode *list, *n;
    Eina_List *files = NULL;

    chunk = url_get_data(mod->curl, SHOUTCAST_LIST);

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
        free(uri);
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
    chunk = url_get_data(mod->curl, url);

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
            evas_stringshare_add("icon/music"), NULL);
}

static Enna_Class_Vfs class_shoutcast =
{ "shoutcast", 1, "SHOUTcast Streaming", NULL, "icon/shoutcast",
{ NULL, NULL, _class_browse_up, _class_browse_down, _class_vfs_get,
}, NULL
};

/* Module interface */

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_BROWSER,
    "browser_shoutcast"
};

void module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    mod->curl = curl_easy_init();

    enna_vfs_append("shoutcast", ENNA_CAPS_MUSIC, &class_shoutcast);
}

void module_shutdown(Enna_Module *em)
{
    Enna_Module_Music *mod = em->mod;

    if (mod->curl)
        curl_easy_cleanup(mod->curl);
    curl_global_cleanup();
}
