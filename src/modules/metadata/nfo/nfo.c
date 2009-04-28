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

#define _GNU_SOURCE
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "enna.h"
#include "module.h"
#include "metadata.h"
#include "xml_utils.h"
#include "url_utils.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME        "metadata_nfo"
#define ENNA_GRABBER_NAME       "nfo"
#define ENNA_GRABBER_PRIORITY   2

typedef struct _Enna_Metadata_Nfo
{
    Evas *evas;
    Enna_Module *em;
} Enna_Metadata_Nfo;

static Enna_Metadata_Nfo *mod;

/****************************************************************************/
/*                          NFO Reader Helpers                              */
/****************************************************************************/

static void
nfo_parse_stream_video (Enna_Metadata *meta, xmlNode *video)
{
    xmlChar *tmp;

    /* video codec */
    if (!meta->video->codec)
    {
        tmp = get_prop_value_from_xml_tree (video, "codec");
        if (tmp)
        {
            meta->video->codec = strdup ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Video Codec: %s", meta->video->codec);
            xmlFree (tmp);
        }
    }

    /* video width */
    if (!meta->video->width)
    {
        tmp = get_prop_value_from_xml_tree (video, "width");
        if (tmp)
        {
            meta->video->width = atoi ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Video Width: %d", meta->video->width);
            xmlFree (tmp);
        }
    }

    /* video height */
    if (!meta->video->height)
    {
        tmp = get_prop_value_from_xml_tree (video, "height");
        if (tmp)
        {
            meta->video->height = atoi ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Video Height: %d", meta->video->height);
            xmlFree (tmp);
        }
    }
}

static void
nfo_parse_stream_audio (Enna_Metadata *meta, xmlNode *audio)
{
    xmlChar *tmp;

    /* audio codec */
    if (!meta->music->codec)
    {
        tmp = get_prop_value_from_xml_tree (audio, "codec");
        if (tmp)
        {
            meta->music->codec = strdup ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Audio Codec: %s", meta->music->codec);
            xmlFree (tmp);
        }
    }

    /* audio channels */
    if (!meta->music->channels)
    {
        tmp = get_prop_value_from_xml_tree (audio, "channels");
        if (tmp)
        {
            meta->music->channels = atoi ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Audio Channels: %d", meta->music->channels);
            xmlFree (tmp);
        }
    }
}

static void
nfo_parse (Enna_Metadata *meta, const char *filename)
{
    xmlDocPtr doc = NULL;
    xmlNode *movie, *fileinfo, *streamdetails;
    xmlChar *tmp;

    struct stat st;
    char *buf;
    ssize_t n;
    int fd;
    int tvshow = 0;

    /* read NFO file */
    stat (filename, &st);
    buf = calloc (1, st.st_size + 1);
    fd = open (filename, O_RDONLY);
    n = read (fd, buf, st.st_size);
    close (fd);

    /* parse its XML content */
    doc = get_xml_doc_from_memory (buf);
    free (buf);
    if (!doc)
        goto error;

    movie = get_node_xml_tree (xmlDocGetRootElement (doc), "movie");
    if (!movie)
    {
        tvshow = 1;
        movie = get_node_xml_tree (xmlDocGetRootElement (doc),
                                   "episodedetails");
        if (!movie)
            goto error;
    }

    fileinfo = get_node_xml_tree (movie, "fileinfo");
    if (!fileinfo)
        goto error;

    streamdetails = get_node_xml_tree (fileinfo, "streamdetails");
    if (streamdetails)
    {
        xmlNode *audio, *video;

        video = get_node_xml_tree (streamdetails, "video");
        if (video)
            nfo_parse_stream_video (meta, video);

        audio = get_node_xml_tree (streamdetails, "audio");
        if (audio)
            nfo_parse_stream_audio (meta, audio);
    }

    /* movie title */
    if (!meta->title)
    {
        tmp = get_prop_value_from_xml_tree (movie, "title");
        if (tmp)
        {
            xmlChar *season = NULL, *episode = NULL;
            if (tvshow)
            {
                season = get_prop_value_from_xml_tree (movie, "season");
                episode = get_prop_value_from_xml_tree (movie, "episode");
            }

            if (season && episode)
            {
                char title[1024];

                memset (title, '\0', sizeof (title));
                snprintf (title, sizeof (title), "S%.2dE%.2d - %s",
                          atoi ((char *) season),
                          atoi ((char *) episode), tmp);
                meta->title = strdup (title);

                if (season)
                    xmlFree (season);
                if (episode)
                    xmlFree (episode);
            }
            else
                meta->title = strdup ((char *) tmp);

            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Title: %s", meta->title);
            xmlFree (tmp);
        }
    }

    /* plot */
    if (!meta->overview)
    {
        tmp = get_prop_value_from_xml_tree (movie, "plot");
        if (tmp)
        {
            meta->overview = strdup ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Plot: %s", meta->overview);
            xmlFree (tmp);
        }
    }

    /* production year */
    if (!meta->year)
    {
        tmp = get_prop_value_from_xml_tree (movie, "year");
        if (tmp)
        {
            meta->year = atoi ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Year: %d", meta->year);
            xmlFree (tmp);
        }
    }

    /* genre */
    if (!meta->categories)
    {
        tmp = get_prop_value_from_xml_tree (movie, "genre");
        if (tmp)
        {
            meta->categories = strdup ((char *) tmp);
            enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                      "Genre: %s", meta->categories);
            xmlFree (tmp);
        }
    }

 error:
    if (doc)
        xmlFreeDoc (doc);
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
nfo_grab (Enna_Metadata *meta, int caps)
{
    char *s, *e, *base;
    char nfo[1024];
    struct stat st;

    if (!meta)
        return;

    s = strchr (meta->uri, '/');
    if (!s)
        return;

    e = strrchr (s + 2, '.');
    if (!e)
        return;

    base = strndup (s + 2, strlen (s + 2) - strlen (e));
    memset (nfo, '\0', sizeof (nfo));
    snprintf (nfo, sizeof (nfo), "%s.nfo", base);
    free (base);

    stat (nfo, &st);
    if (!S_ISREG (st.st_mode))
        return;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Grabbing info from %s", nfo);

    nfo_parse (meta, nfo);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    1,
    ENNA_GRABBER_CAP_COVER,
    nfo_grab,
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

    mod = calloc (1, sizeof (Enna_Metadata_Nfo));

    mod->em = em;
    mod->evas = em->evas;

    enna_metadata_add_grabber (&grabber);
}

void
module_shutdown (Enna_Module *em)
{
    free (mod);
}
