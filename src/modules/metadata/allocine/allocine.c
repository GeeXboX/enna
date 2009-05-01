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
#include <ctype.h>

#include "enna.h"
#include "module.h"
#include "metadata.h"
#include "xml_utils.h"
#include "url_utils.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME         "metadata_allocine"
#define ENNA_GRABBER_NAME        "allocine"
#define ENNA_GRABBER_PRIORITY    4

#define MAX_URL_SIZE             1024
#define MAX_XPATH_QUERY_SIZE     1024

#define ALLOCINE_HOSTNAME        "www.geexbox.org/php"

#define ALLOCINE_QUERY           "http://%s/searchMovieAllocine.php?title=%s"

#define XPATH_QUERY_TITLE	 "//movie/title[@lower='%s']/ancestor::movie"
#define XPATH_QUERY_ALTER_TITLE  "//movie/alternative_title[@lower='%s']/ancestor::movie"

typedef struct _Enna_Metadata_Allocine {
    Evas *evas;
    Enna_Module *em;
    url_t handler;
} Enna_Metadata_Allocine;

static Enna_Metadata_Allocine *mod;

/****************************************************************************/
/*                           Allocine Helpers                               */
/****************************************************************************/

static xmlNodePtr
search_allocine_movie (xmlDocPtr doc, char *xpath_body, Enna_Metadata *meta)
{
    char xpath[MAX_XPATH_QUERY_SIZE];

    xmlChar *xpath_query;
    xmlXPathObjectPtr results_xpath_request = NULL;
    xmlNodeSetPtr nodeset_results_xpath = NULL;
    xmlNodePtr allocine_movie_result = NULL;

    /* conversion fo the keywords in lowercase */
    char *keywords;
    char *tmp;

    if (!meta || !meta->keywords)
        return NULL;

    keywords = strdup(meta->keywords);

    tmp = keywords;
    while (*tmp)
    {
        *tmp = tolower (*tmp);
        tmp++;
    }

    /* setting the xpath query */
    memset (xpath, '\0', MAX_XPATH_QUERY_SIZE);
    snprintf (xpath, MAX_XPATH_QUERY_SIZE, xpath_body, keywords);
    free (keywords);
    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Multiple results with allocine grabber, searching with the XPath\
              request: \"%s\"",
	      xpath);

    xpath_query = xmlCharStrndup (xpath, MAX_XPATH_QUERY_SIZE);

    if (!xpath_query)
        goto error_xpath_query;

    results_xpath_request = get_xnodes_from_xml_tree (doc, xpath_query);

    if (!results_xpath_request)
        goto error_results_xpath;

    nodeset_results_xpath = results_xpath_request->nodesetval;

    if (!nodeset_results_xpath || !nodeset_results_xpath->nodeNr)
        goto error_results_xpath;

    /* get Allocine movie node */
    if (!nodeset_results_xpath->nodeTab[0])
        goto error_results_xpath;
    allocine_movie_result = xmlCopyNode (nodeset_results_xpath->nodeTab[0], 1);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "One result has been found");

 error_results_xpath:
    if (results_xpath_request)
        xmlXPathFreeObject (results_xpath_request);

 error_xpath_query:
    if (xpath_query)
        xmlFree (xpath_query);

    return allocine_movie_result;
}

static void
allocine_parse (Enna_Metadata *meta)
{
    char url[MAX_URL_SIZE];
    char *escaped_keywords;
    url_data_t data;

    xmlDocPtr doc = NULL;
    xmlChar *tmp = NULL;

    xmlNodePtr allocine_movie = NULL;

    if (!meta || !meta->keywords)
        return;

    /* Allocine only has sense on video files */
    if (meta->type != ENNA_METADATA_VIDEO)
        return;

    /* get HTTP compliant keywords */
    escaped_keywords = url_escape_string (mod->handler, meta->keywords);

    /* proceed with Allocine search request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, ALLOCINE_QUERY,
              ALLOCINE_HOSTNAME, escaped_keywords);

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

    /* if there is no result goto error */
    if (!xmlStrcmp ((unsigned char *) tmp, (unsigned char *) "0"))
        goto error;

    xmlFree (tmp);

    /* we try to search the right node using the title */
    allocine_movie = search_allocine_movie (doc, XPATH_QUERY_TITLE, meta);

    if (!allocine_movie)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "Unable to find the item using title");
        /* we try to search the right node using the alternative title */
        allocine_movie = search_allocine_movie (doc,XPATH_QUERY_ALTER_TITLE,meta);
        if (!allocine_movie)
        {
            enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                      "Unable to find the item using alternative title");
	    goto error;
        }
    }

    /* fetch movie alternative title */
    if (!meta->alternative_title)
    {
        tmp = get_prop_value_from_xml_tree (allocine_movie,
                                            "alternative_title");
        if (tmp)
        {
            meta->alternative_title = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

     /* fetch movie overview description */
    if (!meta->overview)
    {
        tmp = get_prop_value_from_xml_tree (allocine_movie,
                                            "short_overview");
        if (tmp)
        {
            meta->overview = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch movie runtime (in minutes) */
    if (!meta->runtime)
    {
        tmp = get_prop_value_from_xml_tree (allocine_movie,
                                            "runtime");
        if (tmp)
        {
            meta->runtime = atoi ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch movie year of production */
    if (!meta->year)
    {
        tmp = get_prop_value_from_xml_tree (allocine_movie,
                                            "release");
        if (tmp)
        {
            int r, y, m, d;
            r = sscanf ((char *) tmp, "%d-%d-%d", &y, &m, &d);
            xmlFree (tmp);
            if (r == 3)
                meta->year = y;
        }
    }

    /* fetch movie categories */
    if (!meta->categories)
    {
        xmlNode *cat;
        int i;

        cat = get_node_xml_tree (allocine_movie, "category");
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

    error:
        if (allocine_movie)
            xmlFreeNode (allocine_movie);
        if (doc)
            xmlFreeDoc (doc);
        ENNA_FREE (escaped_keywords);
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
allocine_grab (Enna_Metadata *meta, int caps)
{
    if (!meta || !meta->keywords)
        return;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Grabbing info from %s", meta->uri);

    allocine_parse (meta);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    1,
    ENNA_GRABBER_CAP_COVER,
    allocine_grab,
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

    mod = calloc(1, sizeof (Enna_Metadata_Allocine));

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
