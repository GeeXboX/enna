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
    /* Movie Studios */
    { "flags/studio/fox",          "FoX"               },
    { "flags/studio/artisan",      "Artisan"           },
    { "flags/studio/carolco",      "Carolco"           },
    { "flags/studio/castlerock",   "CastleRock"        },
    { "flags/studio/columbia",     "Columbia"          },
    { "flags/studio/dimension",    "Dimension"         },
    { "flags/studio/disney",       "Disney"            },
    { "flags/studio/dreamworks",   "Dreamworks"        },
    { "flags/studio/filmfour",     "FilmFour"          },
    { "flags/studio/gaumont",      "Gaumont"           },
    { "flags/studio/hollywood",    "Hollywood"         },
    { "flags/studio/lionsgate",    "LionsGate"         },
    { "flags/studio/lucasfilm",    "LucasFilm"         },
    { "flags/studio/mgm",          "MGM"               },
    { "flags/studio/miramax",      "MiraMax"           },
    { "flags/studio/newline",      "Newline"           },
    { "flags/studio/paramount",    "Paramount"         },
    { "flags/studio/revolution",   "Revolution"        },
    { "flags/studio/screengems",   "ScreenGems"        },
    { "flags/studio/sony",         "Sony"              },
    { "flags/studio/canal",        "Studio Canal"      },
    { "flags/studio/touchstone",   "TouchStone"        },
    { "flags/studio/tristar",      "Tristar"           },
    { "flags/studio/united",       "United"            },
    { "flags/studio/universal",    "Universal"         },
    { "flags/studio/warner",       "WarnerBros"        },

    /* TV Networks */
    { "flags/network/abc",         "ABC"               },
    { "flags/network/bbc",         "BBC One"           },
    { "flags/network/cartoon",     "Cartoon Network"   },
    { "flags/network/cbs",         "CBS"               },
    { "flags/network/channel4",    "Channel 4"         },
    { "flags/network/citytv",      "CityTV"            },
    { "flags/network/comedy",      "Comedy Central"    },
    { "flags/network/fox",         "Fox"               },
    { "flags/network/hbo",         "HBO"               },
    { "flags/network/lifetime",    "Lifetime"          },
    { "flags/network/living",      "LIVING"            },
    { "flags/network/nbc",         "NBC"               },
    { "flags/network/showtime",    "Showtime"          },

    { NULL,                        0                   }
};

static const struct {
    const char *name;
    int min_height;
} flag_media_mapping[] = {
    { "flags/media/divx",     480 },
    { "flags/media/dvd",      576 },
    { "flags/media/hdtv",     720 },
    { "flags/media/bluray",  1080 },
    { NULL,                     0 }
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
            if (!strcasecmp (studio, flag_studio_mapping[i].fullname))
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
    char *h_str, *flag = NULL;

    if (!m)
        goto media_unknown;

    /* try to guess video flag, based on video resolution */
    h_str = enna_metadata_meta_get(m, "height", 1);
    if (h_str)
    {
        int i, h;

        h = atoi (h_str);
        for (i = 0; flag_media_mapping[i].name; i++)
            if (h <= flag_media_mapping[i].min_height)
            {
                flag = strdup(flag_media_mapping[i].name);
                break;
            }

        if (!flag)
            flag = strdup("flags/media/divx");
        ENNA_FREE(h_str);
    }

media_unknown:
    if (!flag)
        flag = strdup("flags/media/default");

    flag_set(sd, &sd->o_media, flag, "flags.media.swallow");
    ENNA_FREE(flag);
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
