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

#include <Ecore_File.h>

#include "enna.h"
#include "module.h"
#include "metadata.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME "metadata_localfiles"
#define ENNA_GRABBER_NAME "localfiles"

typedef struct _Enna_Module_Localfiles
{
    Evas *evas;
    Enna_Module *em;
} Enna_Module_Localfiles;

static Enna_Module_Localfiles *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void
cover_get_from_saved_file (Enna_Metadata *meta)
{
    char cover[1024];

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Trying to get cover from previously saved cover file");

    if (!meta->keywords || !meta->md5)
        return;

    memset (cover, '\0', sizeof (cover));
    snprintf (cover, sizeof (cover), "%s/.enna/covers/%s.png",
              enna_util_user_home_get (), meta->md5);

    if (!ecore_file_exists (cover))
        return;

    meta->cover = strdup (cover);
}

static void
cover_get_from_picture_file (Enna_Metadata *meta)
{
    const char *known_filenames[] =
    { "cover", "front" };

    const char *known_extensions[] =
    { "jpg", "JPG", "jpeg", "JPEG", "png", "PNG", "tbn", "TBN" };

    char *s, *dir = NULL;
    const char *filename = NULL;
    char cover[1024];
    int i, j;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Trying to get cover from picture files");

    if (!meta || !meta->uri)
        return;

    filename = ecore_file_file_get (meta->uri);
    if (!filename)
        goto out;

    dir = ecore_file_dir_get (meta->uri);
    s = strchr (dir, '/');
    if (!s)
        goto out;

    if (ecore_file_can_read (s + 1))
        goto out;

    for (i = 0; i < ARRAY_NB_ELEMENTS (known_extensions); i++)
    {
        char *f, *x;
        x = strrchr (filename, '.');
        if (!x)
            continue;

        f = strndup (filename, strlen (filename) - strlen (x));
        memset (cover, '\0', sizeof (cover));
        snprintf (cover, sizeof (cover), "%s/%s.%s", s + 1, f,
                  known_extensions[i]);
        free (f);

        if (ecore_file_exists (cover))
        {
            meta->cover = strdup (cover);
            goto out;
        }

        for (j = 0; j < ARRAY_NB_ELEMENTS (known_filenames); j++)
        {
            memset (cover, '\0', sizeof (cover));
            snprintf (cover, sizeof (cover), "%s/%s.%s", s + 1,
                      known_filenames[j], known_extensions[i]);

            if (!ecore_file_exists (cover))
                continue;

            meta->cover = strdup (cover);
            goto out;
        }
    }

 out:
    ENNA_FREE (dir);
}

static void
localfiles_grab (Enna_Metadata *meta, int caps)
{
    if (!meta)
        return;

    /* do not grab if already known */
    if (meta->cover)
        return;

    cover_get_from_saved_file (meta);
    if (meta->cover)
        goto cover_found;

    /* check for known cover artwork filenames */
    cover_get_from_picture_file (meta);
    if (meta->cover)
        goto cover_found;

 cover_found:
    if (meta->cover)
        enna_log (ENNA_MSG_INFO, ENNA_MODULE_NAME,
                  "Using cover from: %s", meta->cover);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY_MAX,
    0,
    ENNA_GRABBER_CAP_COVER,
    localfiles_grab,
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

void module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Localfiles));

    mod->em = em;
    mod->evas = em->evas;

    enna_metadata_add_grabber (&grabber);
}

void module_shutdown(Enna_Module *em)
{
    //enna_metadata_remove_grabber (ENNA_GRABBER_NAME);
    free(mod);
}
