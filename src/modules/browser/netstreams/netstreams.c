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
#include <unistd.h>

#include <Ecore_File.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "url_utils.h"
#include "vfs.h"
#include "logs.h"
#include "utils.h"

#define ENNA_MODULE_NAME "netstreams"

#define EXT_M3U_HEADER "#EXTM3U"
#define EXT_M3U_INF "#EXTINF:"

#define MAX_LINE 1024

typedef struct netstream_s
{
    char *uri;
    char *label;
    char *icon;
} netstream_t;

typedef struct netstreams_priv_s
{
    const char *uri;
    const char *prev_uri;
    Eina_List *config_netstreams;
} netstreams_priv_t;

typedef struct _Enna_Module_Netstreams
{
    Evas *e;
    Enna_Module *em;
    url_t handler;
    netstreams_priv_t *music;
    netstreams_priv_t *video;
} Enna_Module_Netstreams;

typedef struct netstreams_cfg_s {
    Eina_List *music;
    Eina_List *video;
} netstreams_cfg_t;

static netstreams_cfg_t netstreams_cfg;
static Enna_Module_Netstreams *mod;

static Eina_List * browse_streams_list(netstreams_priv_t *data)
{
    Eina_List *list = NULL;
    Eina_List *l;

    for (l = data->config_netstreams; l; l = l->next)
    {
        Enna_Vfs_File *file;
        netstream_t *stream;

        stream = l->data;
        file = enna_vfs_create_directory(stream->uri, stream->label,
                stream->icon, NULL);
        list = eina_list_append(list, file);
    }

    data->prev_uri = NULL;
    data->uri = NULL;

    return list;
}

static char * read_line_from_stream(FILE *stream)
{
    char line[MAX_LINE];
    int i = 0;
    char *l;

    memset(line, '\0', MAX_LINE);
    l = fgets(line, MAX_LINE, stream);

    if (!strcmp(line, ""))
        return NULL;

    while (line[i] != '\n' && i < MAX_LINE)
        i++;
    line[i] = '\0';

    return strdup(line);
}

static Eina_List * parse_extm3u(FILE *f)
{
    Eina_List *list = NULL;

    while (!feof(f))
    {
        char *l1 = read_line_from_stream(f);
        if (!l1)
            break; /* End Of Stream */

        if (!strncmp(l1, EXT_M3U_INF, strlen(EXT_M3U_INF)))
        {
            char *title = strstr(l1, "-");
            if (!title)
                continue; /* invalid EXT_M3U line */

            title += 2;

            while (1)
            {
                char *l2;
                Enna_Vfs_File *file;

                l2 = read_line_from_stream(f);
                if (!l2)
                    break;

                if (*l2 == '#')
                {
                    /* skip this line */
                    free(l2);
                    continue;
                }

                file = enna_vfs_create_file(l2, title, "icon/video", NULL);
                list = eina_list_append(list, file);
                free(l2);
                break;
            }
        }
        free(l1);
    }

    return list;
}

static Eina_List * parse_netstream(const char *path, netstreams_priv_t *data)
{
    FILE *f;
    char tmp[] = "/tmp/enna-netstreams-XXXXXX";
    char *file, *header;
    Eina_List *streams = NULL;
    int n, dl = 1;

    if (strstr(path, "file://"))
        dl = 0;

    /* download playlist */
    if (dl)
    {
        url_data_t chunk;

        file = mktemp(tmp);
        chunk = url_get_data(mod->handler, (char *) path);
        if (chunk.status != 0)
          return NULL;

        f = fopen(file, "w");
        if (!f)
            return NULL;

        n = fwrite(chunk.buffer, chunk.size, 1, f);
        free(chunk.buffer);
        fclose(f);
    }
    else
        file = (char *) path + 7;

    /* parse playlist */
    f = fopen(file, "r");
    if (!f)
        return NULL;

    /* Network Extended M3U Playlist Stream Online Listings */
    header = read_line_from_stream(f);
    if (header && !strncmp(header, EXT_M3U_HEADER, strlen(EXT_M3U_HEADER)))
        streams = parse_extm3u(f);

    free(header);
    fclose(f);
    if (dl)
        unlink(file);

    data->uri = eina_stringshare_add(path);
    data->prev_uri = data->uri;

    return streams;
}

static Eina_List * browse_up(const char *path, netstreams_priv_t *data,
        char *icon)
{
    if (!path)
        return browse_streams_list(data);

    /* path is given, download playlist and parse it */
    return parse_netstream(path, data);
}

static Eina_List * browse_up_music(const char *path, void *cookie)
{
    return browse_up(path, mod->music, "icon/music");
}

static Eina_List * browse_up_video(const char *path, void *cookie)
{
    return browse_up(path, mod->video, "icon/video");
}

static Eina_List * browse_down(netstreams_priv_t *data)
{
    if (!data->prev_uri)
        return browse_streams_list(data);

    return NULL;
}

static Eina_List * browse_down_music(void *cookie)
{
    return browse_down(mod->music);
}

static Eina_List * browse_down_video(void *cookie)
{
    return browse_down(mod->video);
}

static Enna_Vfs_File * vfs_get_music(void *cookie)
{
    return enna_vfs_create_directory(mod->music->uri,
            ecore_file_file_get(mod->music->uri),
            eina_stringshare_add("icon/music"), NULL);
}

static Enna_Vfs_File * vfs_get_video(void *cookie)
{
    return enna_vfs_create_directory(mod->video->uri,
            ecore_file_file_get(mod->video->uri),
            eina_stringshare_add("icon/video"), NULL);
}

static void class_init(const char *name, netstreams_priv_t **priv,
                       ENNA_VFS_CAPS caps, Enna_Class_Vfs *class)
{
    netstreams_priv_t *data;
    Eina_List *stream_list;
    Eina_List *l;

    data = calloc(1, sizeof(netstreams_priv_t));
    *priv = data;

    enna_vfs_append(name, caps, class);

    data->prev_uri = NULL;
    data->config_netstreams = NULL;

    switch (caps)
    {
    case ENNA_CAPS_MUSIC:
        stream_list = netstreams_cfg.music;
        break;
    case ENNA_CAPS_VIDEO:
        stream_list = netstreams_cfg.video;
        break;
    default:
        return;
    }

    if (!stream_list)
        return;

    for (l = stream_list; l; l = l->next)
    {
        Eina_List *tuple;

        tuple = enna_util_tuple_get(l->data, ",");
        if (tuple && eina_list_count(tuple) == 3)
        {
            netstream_t *stream;

            stream        = calloc(1, sizeof(netstream_t));
            stream->uri   = eina_list_nth(tuple, 0);
            stream->label = eina_list_nth(tuple, 1);
            stream->icon  = eina_list_nth(tuple, 2);

            enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                     "Adding new netstream '%s' (using icon %s), URI is '%s'",
                     stream->label, stream->icon, stream->uri);

            data->config_netstreams =
                eina_list_append(data->config_netstreams, stream);
        }
    }
}

static Enna_Class_Vfs class_music = {
    "netstreams_music",
    1,
    N_("Browse network streams"),
    NULL,
    "icon/music",
    {
        NULL,
        NULL,
        browse_up_music,
        browse_down_music,
        vfs_get_music,
    },
    NULL
};

static Enna_Class_Vfs class_video = {
    "netstreams_video",
    1,
    N_("Browse network streams"),
    NULL,
    "icon/video",
    {
        NULL,
        NULL,
        browse_up_video,
        browse_down_video,
        vfs_get_video,
    },
    NULL
};

static void
cfg_netstreams_free (void)
{
    char *c;

    EINA_LIST_FREE(netstreams_cfg.music, c)
        ENNA_FREE(c);

    EINA_LIST_FREE(netstreams_cfg.video, c)
        ENNA_FREE(c);
}

static void
cfg_netstreams_section_load (const char *section)
{
    cfg_netstreams_free();

    netstreams_cfg.music =
        enna_config_string_list_get(section, "stream_music");
    netstreams_cfg.video =
        enna_config_string_list_get(section, "stream_video");
}

static void
cfg_netstreams_section_save (const char *section)
{
    enna_config_string_list_set(section, "stream_music", netstreams_cfg.music);
    enna_config_string_list_set(section, "stream_video", netstreams_cfg.video);
}

static void
cfg_netstreams_section_set_default (void)
{
    cfg_netstreams_free();

    netstreams_cfg.music = NULL;
    netstreams_cfg.video = NULL;
}

static Enna_Config_Section_Parser cfg_netstreams = {
    "netstreams",
    cfg_netstreams_section_load,
    cfg_netstreams_section_save,
    cfg_netstreams_section_set_default,
    cfg_netstreams_free,
};

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_netstreams
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    enna_config_section_parser_register(&cfg_netstreams);
    cfg_netstreams_section_set_default();
    cfg_netstreams_section_load(cfg_netstreams.section);

    mod = calloc(1, sizeof(Enna_Module_Netstreams));
    mod->em = em;
    em->mod = mod;

    mod->handler = url_new();

    class_init("netstreams_music", &mod->music, ENNA_CAPS_MUSIC, &class_music);
    class_init("netstreams_video", &mod->video, ENNA_CAPS_VIDEO, &class_video);
}

static void
module_shutdown(Enna_Module *em)
{
    Enna_Module_Netstreams *mod;

    mod = em->mod;;
    free(mod->music);
    free(mod->video);

    url_free(mod->handler);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_netstreams",
    N_("Netstreams"),
    "icon/module",
    N_("Browse and play network streams"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
