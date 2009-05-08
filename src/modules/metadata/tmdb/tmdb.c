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
#include "module.h"
#include "metadata.h"
#include "xml_utils.h"
#include "url_utils.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME        "metadata_tmdb"
#define ENNA_GRABBER_NAME       "tmdb"
#define ENNA_GRABBER_PRIORITY   5

#define PATH_BACKDROPS          "backdrops"
#define PATH_COVERS             "covers"

#define MAX_URL_SIZE            1024

#define TMDB_HOSTNAME           "api.themoviedb.org"

#define TMDB_API_KEY            "5401cd030990fba60e1c23d2832de62e"

#define TMDB_QUERY_SEARCH "http://%s/2.0/Movie.search?title=%s&api_key=%s"
#define TMDB_QUERY_INFO   "http://%s/2.0/Movie.getInfo?id=%s&api_key=%s"

typedef struct _Enna_Metadata_Tmdb
{
    Evas *evas;
    Enna_Module *em;
    url_t handler;
} Enna_Metadata_Tmdb;

static Enna_Metadata_Tmdb *mod;

/*****************************************************************************/
/*                         TheMovieDB.org Helpers                            */
/*****************************************************************************/

static void
tmdb_parse (Enna_Metadata *meta)
{
    char url[MAX_URL_SIZE];
    char *escaped_keywords;
    url_data_t data;

    xmlDocPtr doc = NULL;
    xmlChar *tmp;
    xmlNode *n;

    if (!meta || !meta->keywords)
        return;

    /* TMDB only has sense on video files */
    if (meta->type != ENNA_METADATA_VIDEO)
        return;

    /* get HTTP compliant keywords */
    escaped_keywords = url_escape_string (mod->handler, meta->keywords);

    /* proceed with TMDB search request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, TMDB_QUERY_SEARCH,
              TMDB_HOSTNAME, escaped_keywords, TMDB_API_KEY);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Search Request: %s", url);

    data = url_get_data (mod->handler, url);
    if (data.status != 0)
        goto error;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Search Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = get_xml_doc_from_memory (data.buffer);
    free (data.buffer);
    if (!doc)
        goto error;

    /* check for total number of results */
    tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                        "totalResults");
    if (!tmp)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "Unable to find the item \"%s\"", escaped_keywords);
        goto error;
    }

    /* check that requested item is known on TMDB */
    if (!xmlStrcmp ((unsigned char *) tmp, (unsigned char *) "0"))
        goto error;
    xmlFree (tmp);

    /* get TMDB Movie ID */
    tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc), "id");
    if (!tmp)
        goto error;

    xmlFreeDoc (doc);
    doc = NULL;

    /* proceed with TMDB search request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE,
              TMDB_QUERY_INFO, TMDB_HOSTNAME, tmp, TMDB_API_KEY);
    xmlFree (tmp);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Info Request: %s", url);

    data = url_get_data (mod->handler, url);
    if (data.status != 0)
        goto error;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Info Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = xmlReadMemory (data.buffer, data.size, NULL, NULL, 0);
    free (data.buffer);
    if (!doc)
        goto error;

    n = xmlDocGetRootElement (doc);

    /* fetch movie overview description */
    xml_search_str (n, "short_overview", &meta->overview);

    /* fetch movie runtime (in minutes) */
    xml_search_int (n, "runtime", &meta->runtime);

    /* fetch movie year of production */
    xml_search_year (n, "release", &meta->year);

    /* fetch movie rating */
    xml_search_int (n, "rating", &meta->rating);
    meta->rating /= 2;

    /* fetch movie budget */
    xml_search_int (n, "budget", &meta->budget);

    /* fetch movie country */
    if (!meta->country)
    {
        xmlNode *country;

        country = get_node_xml_tree (n, "country");
        if (country)
        {
            xmlChar *tmp;

            tmp = get_prop_value_from_xml_tree (country, "short_name");
            if (tmp)
            {
                meta->country = strdup ((char *) tmp);
                xmlFree (tmp);
            }
        }
    }

    /* fetch movie categories */
    if (!meta->categories)
    {
        xmlNode *cat;
        int i;

        cat = get_node_xml_tree (n, "category");
        for (i = 0; i < 4; i++)
        {
            if (!cat)
                break;

            tmp = get_prop_value_from_xml_tree (cat, "name");
            if (tmp)
            {
                enna_metadata_add_category (meta, (char *) tmp);
                xmlFree (tmp);
            }
            cat = cat->next;
        }
    }

    /* fetch movie poster/cover */
    if (!meta->cover)
    {
        tmp = get_prop_value_from_xml_tree_by_attr (n,
                                                    "poster", "size", "cover");

        if (tmp)
        {
            char cover[1024];

            snprintf (cover, sizeof (cover), "%s/.enna/%s/%s.png",
                      enna_util_user_home_get(), PATH_COVERS, meta->md5);
            url_save_to_disk (mod->handler, (char *) tmp, cover);
            xmlFree (tmp);

            meta->cover = strdup (cover);
        }
    }

    /* fetch movie backdrop */
    if (!meta->backdrop)
    {
        tmp = get_prop_value_from_xml_tree_by_attr (n,
                                                    "backdrop", "size", "mid");

        if (tmp)
        {
            char back[1024];

            snprintf (back, sizeof (back), "%s/.enna/%s/%s.png",
                      enna_util_user_home_get(), PATH_BACKDROPS, meta->md5);
            url_save_to_disk (mod->handler, (char *) tmp, back);
            xmlFree (tmp);

            meta->backdrop = strdup (back);
        }
    }

    /* fetch movie people */
    if (!meta->director || !meta->actors)
    {
        xmlNode *cat;

        cat = get_node_xml_tree (n, "people");
        while (cat)
        {
            xmlChar *ch;

            ch = get_attr_value_from_node (cat, "job");
            if (!ch)
                continue;

            if (!strcmp ((char *) ch, "director"))
                xml_search_str (cat, "name", &meta->director);
            else if (!strcmp ((char *) ch, "actor"))
            {
                char *actor = NULL, *role = NULL;

                xml_search_str (cat, "name", &actor);
                xml_search_str (cat, "role", &role);

                if (actor)
                {
                    char str[128];

                    memset (str, '\0', sizeof (str));
                    if (role)
                        snprintf (str, sizeof (str), "%s (%s)", actor, role);
                    else
                        snprintf (str, sizeof (str), "%s", actor);
                    enna_metadata_add_actors (meta, str);
                }

                ENNA_FREE (actor);
                ENNA_FREE (role);
            }
            xmlFree (ch);
            cat = cat->next;
        }
    }

 error:
    if (doc)
        xmlFreeDoc (doc);
    ENNA_FREE (escaped_keywords);
}

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void
tmdb_grab (Enna_Metadata *meta, int caps)
{
    if (!meta || !meta->keywords)
        return;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Grabbing info from %s", meta->uri);

    tmdb_parse (meta);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    1,
    ENNA_GRABBER_CAP_COVER,
    tmdb_grab,
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_METADATA,
    ENNA_MODULE_NAME
};

void
module_init (Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof (Enna_Metadata_Tmdb));

    mod->em = em;
    mod->evas = em->evas;

    mod->handler = url_new ();
    enna_metadata_add_grabber (&grabber);
}

void
module_shutdown (Enna_Module *em)
{
    //enna_metadata_remove_grabber (ENNA_GRABBER_NAME);
    url_free (mod->handler);
    free (mod);
}
