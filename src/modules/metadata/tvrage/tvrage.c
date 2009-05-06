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
#include "metadata.c"
#include "xml_utils.h"
#include "url_utils.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME          "metadata_tvrage"
#define ENNA_GRABBER_NAME         "tvrage"
#define ENNA_GRABBER_PRIORITY     5

#define MAX_URL_SIZE              1024

#define TVRAGE_HOSTNAME           "services.tvrage.com"

/* See http://services.tvrage.com/index.php?page=public for the feeds */
#define TVRAGE_QUERY_SEARCH       "http://%s/feeds/search.php?show=%s"
#define TVRAGE_QUERY_INFO         "http://%s/feeds/full_show_info.php?sid=%s"

typedef struct _Enna_Metadata_Tvrage {
    Evas *evas;
    Enna_Module *em;
    url_t handler;
} Enna_Metadata_Tvrage;

static Enna_Metadata_Tvrage *mod;

/****************************************************************************/
/*                           TvRage.com Helpers                            */
/****************************************************************************/

static void
tvrage_parse (Enna_Metadata *meta)
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

    /* proceed with TVDB search request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, TVRAGE_QUERY_SEARCH,
              TVRAGE_HOSTNAME, escaped_keywords);

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

    /* check for a known DB entry */
    n = get_node_xml_tree (xmlDocGetRootElement (doc), "show");
    if (!n)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "Unable to find the item \"%s\"", escaped_keywords);
        goto error;
    }

    /* get TVDB serie ID */
    tmp = get_prop_value_from_xml_tree (n, "showid");
    if (!tmp)
        goto error;

    xmlFreeDoc (doc);
    doc = NULL;

    /* proceed with TVDB info request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, TVRAGE_QUERY_INFO,
              TVRAGE_HOSTNAME, tmp);
    xmlFree (tmp);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Info Request: %s", url);

    data = url_get_data (mod->handler, url);
    if (data.status != 0)
        goto error;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Info Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = xmlReadMemory (data.buffer, data.size, NULL, NULL, 0);
    free (data.buffer);
    if (!doc)
        goto error;

    /* fetch tv show french title (to be extended to language param) */
    if (!meta->alternative_title)
    {
        tmp = get_prop_value_from_xml_tree_by_attr (xmlDocGetRootElement (doc),
                                            "aka","country","FR");
        if (tmp)
        {
            meta->alternative_title = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

   /* fetch tv show country */
    if (!meta->country)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "origin_country");
        if (tmp)
        {
            meta->country = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch tv show studio */
    if (!meta->studio)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "network");
        if (tmp)
        {
            meta->studio = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch tv show runtime (in minutes) */
     if (!meta->runtime)
     {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "runtime");
        if (tmp)
        {
            meta->runtime = atoi ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch tv show categories */
    if (!meta->categories)
    {
        xmlNode *cat;
        int i;

        cat = get_node_xml_tree (xmlDocGetRootElement (doc), "genre");
        for (i = 0; i < 4; i++)
        {
            if (!cat)
                break;

            tmp = get_prop_value_from_xml_tree (cat, "genre");
            if (tmp)
            {
                enna_metadata_add_category (meta, (char *) tmp);
                xmlFree (tmp);
            }
            cat = cat->next;
        }
    }

    enna_metadata_dump(meta);

 error:
    if (doc)
        xmlFreeDoc (doc);
    ENNA_FREE (escaped_keywords);
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
tvrage_grab (Enna_Metadata *meta, int caps)
{
    if (!meta || !meta->keywords)
        return;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Grabbing info from %s", meta->uri);

    tvrage_parse (meta);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    1,
    ENNA_GRABBER_CAP_COVER,
    tvrage_grab,
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

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

    mod = calloc(1, sizeof (Enna_Metadata_Tvrage));

    mod->em = em;
    mod->evas = em->evas;

    mod->handler = url_new ();
    enna_metadata_add_grabber (&grabber);
}

void
module_shutdown (Enna_Module *em)
{
    url_free (mod->handler);
    free (mod);
}
