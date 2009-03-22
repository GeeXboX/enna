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
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Ecore_File.h>

#include "enna.h"
#include "url_utils.h"
#include "logs.h"

#define ENNA_MODULE_NAME        "enna"

static size_t url_buffer_get(void *ptr, size_t size, size_t nmemb, void *data)
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

url_data_t url_get_data(CURL *curl, char *url)
{
    url_data_t chunk;

    chunk.buffer = NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0; /* no data at this point */
    chunk.status = CURLE_FAILED_INIT;
    
    if (!curl || !url)
      return chunk;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, url_buffer_get);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

    chunk.status = curl_easy_perform(curl);

    return chunk;
}

void url_escape_string(char *outbuf, const char *inbuf)
{
    unsigned char c, c1, c2;
    int i, len;

    len = strlen(inbuf);

    for (i = 0; i < len; i++)
    {
        c = inbuf[i];
        if ((c == '%') && i < len - 2)
        { /* need 2 more characters */
            c1 = toupper(inbuf[i + 1]);
            c2 = toupper(inbuf[i + 2]);
            /* need uppercase chars */
        }
        else
        {
            c1 = 129;
            c2 = 129;
            /* not escaped chars */
        }

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c
                <= '9') || (c >= 0x7f))
            *outbuf++ = c;
        else if (c =='%' && ((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1
                <= 'F')) && ((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2
                <= 'F')))
        {
            /* check if part of an escape sequence */
            *outbuf++ = c; /* already escaped */
        }
        else
        {
            /* all others will be escaped */
            c1 = ((c & 0xf0) >> 4);
            c2 = (c & 0x0f);
            if (c1 < 10)
                c1 += '0';
            else
                c1 += 'A' - 10;
            if (c2 < 10)
                c2 += '0';
            else
                c2 += 'A' - 10;
            *outbuf++ = '%';
            *outbuf++ = c1;
            *outbuf++ = c2;
        }
    }
    *outbuf++='\0';
}

void
url_save_to_disk (CURL *curl, char *src, char *dst)
{
    url_data_t data;
    int n, fd;
    
    if (!curl || !src || !dst)
        return;

    /* no need to download again an already existing file */
    if (ecore_file_exists (dst))
        return;
    
    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Saving %s to %s", src, dst);

    data = url_get_data (curl, src);
    if (data.status != CURLE_OK)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "Unable to download requested cover file");
        return;
    }

    fd = open (dst, O_WRONLY | O_CREAT, 0666);
    if (fd < 0)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "Unable to open stream to save cover file");

        free (data.buffer);
        return;
    }

    n = write (fd, data.buffer, data.size);
    free (data.buffer);
}
