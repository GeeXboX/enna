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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"
#include "logs.h"
#include "image.h"
#include "buffer.h"
#include "utils.h"

#define SMART_NAME "enna_panel_infos"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_img;
    Evas_Object *o_rating;
    Evas_Object *o_cover;
};


/* externally accessible functions */
Evas_Object *
enna_panel_infos_add(Evas * evas)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));

    sd->o_edje = edje_object_add(evas);
    edje_object_file_set(sd->o_edje,
                         enna_config_theme_get(),
                         "activity/video/panel_infos");
    evas_object_show(sd->o_edje);
    evas_object_data_set(sd->o_edje, "sd", sd);
    return sd->o_edje;
}

/****************************************************************************/
/*                          Information Panel                               */
/****************************************************************************/

void
enna_panel_infos_set_text (Evas_Object *obj, Enna_Metadata *m)
{
    buffer_t *buf;
    char *alternative_title, *title, *categories, *year;
    char *runtime, *length, *director, *actors, *overview;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!m)
    {
        edje_object_part_text_set (sd->o_edje, "infos.panel.textblock",
	    _("No such information ..."));
        return;
    }

    buf = buffer_new ();

    buffer_append (buf, "<h4><hl><sd><b>");
    alternative_title = enna_metadata_meta_get (m, "alternative_title", 1);
    title = enna_metadata_meta_get (m, "title", 1);
    buffer_append (buf, alternative_title ? alternative_title : title);
    buffer_append (buf, "</b></sd></hl></h4><br>");

    categories = enna_metadata_meta_get (m, "category", 5);
    if (categories)
        buffer_appendf (buf, "<h2>%s</h2><br>", categories);

    year = enna_metadata_meta_get (m, "year", 1);
    if (year)
        buffer_append (buf, year);

    runtime = enna_metadata_meta_get (m, "runtime", 1);
    length = enna_metadata_meta_get (m, "length", 1);
    if (runtime || length)
    {
        int hh = 0, mm = 0;

        if (year)
            buffer_append (buf, " - ");

        if (runtime)
        {
            hh = (int) (atoi (runtime) / 60);
            mm = (int) (atoi (runtime) - 60 * hh);
        }
        else if (length)
        {
            hh = (int) (atoi (length) / 3600 / 1000);
            mm = (int) ((atoi (length) / 60 / 1000) - (60 * hh));
        }

        if (hh)
            buffer_appendf (buf, ngettext("%.2d hour ", "%.2d hours ", hh), hh);
        if (hh && mm)
            buffer_append (buf, " ");
        if (mm)
            buffer_appendf (buf, ngettext("%.2d minute", "%.2d minutes", mm), mm);
    }
    buffer_append (buf, "<br><br>");

    director = enna_metadata_meta_get (m, "director", 1);
    if (director)
	buffer_appendf (buf, _("<ul>Director:</ul> %s<br>"), director);

    actors = enna_metadata_meta_get (m, "actor", 5);
    if (actors)
        buffer_appendf (buf, _("<ul>Cast:</ul> %s<br>"), actors);

    if (director || actors)
	buffer_append (buf, "<br>");

    overview = enna_metadata_meta_get (m, "synopsis", 1);
    if (overview)
        buffer_appendf (buf, "%s", overview);

#if 0
    buffer_append (buf, "<br><br>");
    buffer_appendf (buf, _("<hl>Video: </hl> %s, %dx%d, %.2f fps<br>"),
                    m->video->codec, m->video->width,
                    m->video->height, m->video->framerate);
    buffer_appendf (buf, _("<hl>Audio: </hl> %s, %d ch., %i kbps, %d Hz<br>"),
                    m->music->codec, m->music->channels,
                    m->music->bitrate / 1000, m->music->samplerate);
    buffer_appendf (buf, _("<hl>Size: </hl> %.2f MB<br>"),
                    m->size / 1024.0 / 1024.0);
#endif
    edje_object_part_text_set (sd->o_edje, "infos.panel.textblock", buf->buf);

    buffer_free (buf);
    ENNA_FREE (alternative_title);
    ENNA_FREE (title);
    ENNA_FREE (categories);
    ENNA_FREE (year);
    ENNA_FREE (runtime);
    ENNA_FREE (length);
    ENNA_FREE (director);
    ENNA_FREE (actors);
    ENNA_FREE (overview);
}

void
enna_panel_infos_set_cover(Evas_Object *obj, Enna_Metadata *m)
{
    Evas_Object *cover;
    char *file = NULL;
    int from_vfs = 1;
    char *cv;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (!m)
    {
        file = strdup ("backdrop/default");
        from_vfs = 0;
    }

    cv = enna_metadata_meta_get (m, "cover", 1);
    if (!file && cv)
    {
        char dst[1024] = { 0 };

        snprintf (dst, sizeof (dst), "%s/.enna/covers/%s",
                  enna_util_user_home_get (), cv);
        file = strdup (dst);
    }

    if (!file)
    {
        file = strdup ("backdrop/default");
        from_vfs = 0;
    }

    if (from_vfs)
    {
        cover = enna_image_add (evas_object_evas_get(sd->o_edje));
        enna_image_fill_inside_set (cover, 0);
        enna_image_file_set (cover, file, NULL);
    }
    else
    {
        cover = edje_object_add (evas_object_evas_get(sd->o_edje));
        edje_object_file_set (cover, enna_config_theme_get(), file);
    }


    ENNA_OBJECT_DEL (sd->o_cover);
    sd->o_cover = cover;
    edje_object_part_swallow (sd->o_edje,
                              "infos.panel.cover.swallow", sd->o_cover);
    edje_object_signal_emit (sd->o_edje, strcmp(file, "backdrop/default") ?  "cover,show": "cover,hide", "enna");
    ENNA_FREE (cv);
    ENNA_FREE (file);
}

void
enna_panel_infos_set_rating(Evas_Object *obj, Enna_Metadata *m)
{
    Evas_Object *rating = NULL;
    char *rt;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    rt = enna_metadata_meta_get (m, "rating", 1);
    if (rt)
    {
        char rate[16];
        int r;

        r = MAX (atoi (rt), 0);
        r = MIN (atoi (rt), 5);
        memset (rate, '\0', sizeof (rate));
        snprintf (rate, sizeof (rate), "rating/%d", r);
        rating = edje_object_add (evas_object_evas_get(sd->o_edje));
        edje_object_file_set (rating, enna_config_theme_get(), rate);
    }

    ENNA_OBJECT_DEL (sd->o_rating);
    sd->o_rating = rating;
    edje_object_part_swallow (sd->o_edje,
                              "infos.panel.rating.swallow", sd->o_rating);
    ENNA_FREE (rt);
}


