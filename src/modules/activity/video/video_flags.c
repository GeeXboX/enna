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

#include "enna.h"
#include "enna_config.h"
#include "metadata.h"

#define EDJE_GROUP "activity/video/flags"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *o_edje;
    Evas_Object *o_video;
    Evas_Object *o_audio;
    Evas_Object *o_studio;
    Evas_Object *o_media;
};

static const struct {
    const char *name;
    int min_height;
} flag_video_mapping[] = {
    { "flags/video/480p",     480 },
    { "flags/video/540p",     540 },
    { "flags/video/576p",     576 },
    { "flags/video/720p",     720 },
    { "flags/video/1080p",   1080 },
    { NULL,                     0 }
};

static const struct {
    const char *name;
    int channels;
} flag_audio_mapping[] = {
    { "flags/audio/mono",     1 },
    { "flags/audio/dd20",     2 },
    { "flags/audio/dd51",     5 },
    { "flags/audio/dd71",     7 },
    { NULL,                   0 }
};

static const struct {
    const char *name;
    const char *fullname;
} flag_studio_mapping[] = {
    { NULL,                   0 }
};

static void
video_flags_del(void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    ENNA_OBJECT_DEL(sd->o_video);
    ENNA_OBJECT_DEL(sd->o_audio);
    ENNA_OBJECT_DEL(sd->o_studio);
    ENNA_OBJECT_DEL(sd->o_media);
    ENNA_FREE(sd);
}

static void
flag_set (Smart_Data *sd, Evas_Object **obj,
          const char *icon, const char *edje)
{
    if (!sd || !obj || !icon || !edje)
        return;

    ENNA_OBJECT_DEL(*obj);
    *obj = edje_object_add(enna->evas);
    edje_object_file_set(*obj, enna_config_theme_get(), icon);
    evas_object_show(*obj);
    edje_object_part_swallow(sd->o_edje, edje, *obj);
}

static void
video_flag_set (Smart_Data *sd, Enna_Metadata *m)
{
    char *h_str, *flag = NULL;

    if (!m)
        goto video_unknown;

    /* try to guess video flag, based on video resolution */
    h_str = enna_metadata_meta_get(m, "height", 1);
    if (h_str)
    {
        int i, h;

        h = atoi (h_str);
        for (i = 0; flag_video_mapping[i].name; i++)
            if (h <= flag_video_mapping[i].min_height)
            {
                flag = strdup(flag_video_mapping[i].name);
                break;
            }

        if (!flag)
            flag = strdup("flags/video/sd");
        ENNA_FREE(h_str);
    }

video_unknown:
    if (!flag)
        flag = strdup("flags/video/default");

    flag_set(sd, &sd->o_video, flag, "flags.video.swallow");
    ENNA_FREE(flag);
}

static void
audio_flag_set (Smart_Data *sd, Enna_Metadata *m)
{
    char *c_str, *flag = NULL;

    if (!m)
        goto audio_unknown;

    /* try to guess audio flag (naive method atm) */
    c_str = enna_metadata_meta_get(m, "audio_channels", 1);
    if (c_str)
    {
        int i, c;

        c = atoi (c_str);
        for (i = 0; flag_audio_mapping[i].name; i++)
            if (c <= flag_audio_mapping[i].channels)
            {
                flag = strdup(flag_audio_mapping[i].name);
                break;
            }

        ENNA_FREE(c_str);
    }

audio_unknown:
    if (!flag)
        flag = strdup("flags/audio/default");

    flag_set(sd, &sd->o_audio, flag, "flags.audio.swallow");
    ENNA_FREE(flag);
}



static void
studio_flag_set (Smart_Data *sd, Enna_Metadata *m)
{
    char *studio, *flag = NULL;

    if (!m)
        goto studio_unknown;

    /* try to guess studio flag */
    studio = enna_metadata_meta_get(m, "studio", 1);
    if (studio)
    {
        int i;

        for (i = 0; flag_studio_mapping[i].name; i++)
            if (!strcmp (studio, flag_studio_mapping[i].fullname))
            {
                flag = strdup(flag_studio_mapping[i].name);
                break;
            }

        ENNA_FREE(studio);
    }

studio_unknown:
    if (!flag)
        flag = strdup("flags/studio/default");

    flag_set(sd, &sd->o_studio, flag, "flags.studio.swallow");
    ENNA_FREE(flag);
}

static void
media_flag_set (Smart_Data *sd, Enna_Metadata *m)
{
    /* detect media type: no idea how to that atm, alwasy use default one */
    flag_set(sd, &sd->o_media,
             "flags/media/default", "flags.media.swallow");
}

/****************************************************************************/
/*                               Public API                                 */
/****************************************************************************/

Evas_Object *
enna_video_flags_add(Evas * evas)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));

    sd->o_edje = edje_object_add(evas);
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), EDJE_GROUP);
    evas_object_show(sd->o_edje);
    evas_object_data_set(sd->o_edje, "sd", sd);
    evas_object_event_callback_add(sd->o_edje, EVAS_CALLBACK_DEL,
                                   video_flags_del, sd);
    return sd->o_edje;
}

void
enna_video_flags_update(Evas_Object *obj, Enna_Metadata *m)
{
    Smart_Data *sd;

    sd = evas_object_data_get(obj, "sd");
    video_flag_set(sd, m);
    audio_flag_set(sd, m);
    studio_flag_set(sd, m);
    media_flag_set(sd, m);
}
