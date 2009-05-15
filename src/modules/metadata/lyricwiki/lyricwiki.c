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

#define ENNA_MODULE_NAME        "metadata_lyricwiki"
#define ENNA_GRABBER_NAME       "lyricwiki"
#define ENNA_GRABBER_PRIORITY   5

#define MAX_URL_SIZE            1024

#define LYRICWIKI_HOSTNAME     "lyricwiki.org"
#define LYRICWIKI_QUERY_SEARCH "http://%s/api.php?artist=%s&song=%s&fmt=xml"

typedef struct _Enna_Metadata_LyricWiki
{
    Evas *evas;
    Enna_Module *em;
    url_t handler;
} Enna_Metadata_LyricWiki;

static Enna_Metadata_LyricWiki *mod;

/*****************************************************************************/
/*                         TheMovieDB.org Helpers                            */
/*****************************************************************************/

static void
lyricwiki_parse (Enna_Metadata *meta)
{
    char url[MAX_URL_SIZE];
    char *artist, *song, *lyrics = NULL;
    url_data_t data;

    xmlDocPtr doc = NULL;

    /* get HTTP compliant keywords */
    artist = url_escape_string (mod->handler, meta->music->artist);
    song = url_escape_string (mod->handler, meta->title);

    /* proceed with LyricWiki.org search request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, LYRICWIKI_QUERY_SEARCH,
              LYRICWIKI_HOSTNAME, artist, song);

    ENNA_FREE (artist);
    ENNA_FREE (song);

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

    /* fetch lyrics */
    xml_search_str (xmlDocGetRootElement (doc), "lyrics", &lyrics);

    /* check if lyrics have been found */
    if (!strcmp (lyrics, "Not found"))
        goto error;

    if (!meta->lyrics)
        meta->lyrics = strdup (lyrics);

 error:
    if (doc)
        xmlFreeDoc (doc);
    ENNA_FREE (lyrics);
}

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void
lyricwiki_grab (Enna_Metadata *meta, int caps)
{
    if (!meta)
        return;

    /* LyricWiki only has sense on video files */
    if (meta->type != ENNA_METADATA_AUDIO)
        return;

    /* we require artist and song name */
    if (!meta->title || !meta->music || !meta->music->artist)
        return;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Grabbing info from %s", meta->uri);

    lyricwiki_parse (meta);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    1,
    ENNA_GRABBER_CAP_COVER,
    lyricwiki_grab,
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

    mod = calloc(1, sizeof (Enna_Metadata_LyricWiki));

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
