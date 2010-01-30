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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <Ecore_File.h>

#include "enna.h"
#include "url_utils.h"
#include "logs.h"

#define ENNA_MODULE_NAME        "enna"

static size_t
url_buffer_get(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    url_data_t *mem = (url_data_t *) data;

    mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);
    if (mem->buffer)
    {
        memcpy(&(mem->buffer[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->buffer[mem->size] = 0;
    }

    return realsize;
}

url_t
url_new(void)
{
    CURL *curl;

    curl = curl_easy_init();
    return (url_t) curl;
}

void
url_free(url_t url)
{
    if (url)
        curl_easy_cleanup((CURL *) url);
}

void
url_global_init(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void
url_global_uninit(void)
{
    curl_global_cleanup();
}

url_data_t
url_get_data(url_t handler, char *url)
{
    url_data_t chunk;
    CURL *curl = (CURL *) handler;

    chunk.buffer = NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0; /* no data at this point */
    chunk.status = CURLE_FAILED_INIT;

    if (!curl || !url)
      return chunk;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, url_buffer_get);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

    chunk.status = curl_easy_perform(curl);

    return chunk;
}

char *
url_escape_string(url_t handler, const char *buf)
{
    CURL *curl = (CURL *) handler;

    if (!curl || !buf)
        return NULL;

    return curl_easy_escape(curl, buf, strlen (buf));
}

void
url_save_to_disk(url_t handler, char *src, char *dst)
{
    url_data_t data;
    int n, fd;
    CURL *curl = (CURL *) handler;

    if (!curl || !src || !dst)
        return;

    /* no need to download again an already existing file */
    if (ecore_file_exists(dst))
        return;

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Saving %s to %s", src, dst);

    data = url_get_data(curl, src);
    if (data.status != CURLE_OK)
    {
        enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                 "Unable to download requested cover file");
        return;
    }

    fd = open(dst, O_WRONLY | O_CREAT, 0666);
    if (fd < 0)
    {
        enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                 "Unable to open stream to save cover file");

        free(data.buffer);
        return;
    }

    n = write(fd, data.buffer, data.size);
    close(fd);
    free(data.buffer);
}
