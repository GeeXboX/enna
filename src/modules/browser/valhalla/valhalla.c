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

#include <valhalla.h>

#include "enna.h"
#include "module.h"
#include "vfs.h"
#include "logs.h"
#include "utils.h"
#include "metadata.h"

#define ENNA_MODULE_NAME "valhalla"

#define PATH_BUFFER 4096

typedef enum browser_level
{
    BROWSER_LEVEL_ROOT,
    BROWSER_LEVEL_AUTHOR,
    BROWSER_LEVEL_AUTHOR_LIST,
    BROWSER_LEVEL_AUTHOR_LIST_ALBUM,
    BROWSER_LEVEL_ALBUM,
    BROWSER_LEVEL_ALBUM_LIST,
    BROWSER_LEVEL_GENRE,
    BROWSER_LEVEL_GENRE_LIST,
    BROWSER_LEVEL_GENRE_LIST_ALBUM,
    BROWSER_LEVEL_UNCLASSIFIED,
} browser_level_t;

typedef struct vh_data_s {
    Eina_List       **list;
    browser_level_t   level;
    const char       *icon;
} vh_data_t;

typedef struct _Enna_Module_libvalhalla
{
    Enna_Module     *em;
    valhalla_t      *valhalla;
    browser_level_t  level;
    int64_t          prev_id_m1, prev_id_d1;
    int64_t          prev_id_m2, prev_id_d2;
    Enna_Vfs_File   *vfs;
} Enna_Module_libvalhalla;

static Enna_Module_libvalhalla *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _vfs_add_dir(Eina_List **list, int level,
                         valhalla_db_metares_t *res, const char *icon)
{
    Enna_Vfs_File *file;
    char str[128];

    snprintf(str, sizeof(str),
             "%i/%"PRIi64":%"PRIi64, level, res->meta_id, res->data_id);
    file = enna_vfs_create_directory(str, res->data_value, icon, NULL);
    *list = eina_list_append(*list, file);
}

static void _vfs_add_file(Eina_List **list,
                          valhalla_db_fileres_t *file,
                          valhalla_db_filemeta_t *metadata,
                          const char *icon)
{
    Enna_Vfs_File *f;
    char buf[PATH_BUFFER];
    char name[256];
    char *it;
    const char *track = NULL, *title = NULL;
    valhalla_db_filemeta_t *md_it;

    snprintf(buf, sizeof(buf), "file://%s", file->path);
    it = strrchr(buf, '/');

    for (md_it = metadata; md_it; md_it = md_it->next)
    {
        if (!strcmp(md_it->meta_name, "track")
            && md_it->group == VALHALLA_META_GRP_ORGANIZATIONAL)
            track = md_it->data_value;
        else if (!strcmp(md_it->meta_name, "title")
                 && md_it->group == VALHALLA_META_GRP_TITLES)
            title = md_it->data_value;
    }

    if (track)
    {
        char *it2;
        it2 = strchr(track, '/');
        if (it2)
          *it2 = '\0';
        snprintf(name, sizeof(name), "%2s - %s", track, title ? title : it + 1);
    }
    else
        snprintf(name, sizeof(name), "%s", title ? title : it + 1);

    f = enna_vfs_create_file(buf, name, icon, NULL);
    *list = eina_list_append(*list, f);
}

static int _result_dir_cb(void *data, valhalla_db_metares_t *res)
{
    vh_data_t *vh = data;

    if (!data || !res)
        return 0;

    _vfs_add_dir(vh->list, vh->level, res, vh->icon);
    return 0;
}

static int _result_file_cb(void *data, valhalla_db_fileres_t *res)
{
    Eina_List **list = data;
    valhalla_db_filemeta_t *metadata = NULL;
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_STR(EQUAL, "track", NULL);
    valhalla_db_restrict_t r2 = VALHALLA_DB_RESTRICT_STR(EQUAL, "title", NULL);

    if (!res)
        return 0;

    VALHALLA_DB_RESTRICT_LINK(r2, r1);

    /* retrieve the track and the title */
    valhalla_db_file_get (mod->valhalla, res->id, NULL, &r1, &metadata);
    _vfs_add_file(list, res, metadata, "icon/file/music");
    VALHALLA_DB_FILEMETA_FREE(metadata);
    return 0;
}

static int _sort_cb(const void *d1, const void *d2)
{
    const Enna_Vfs_File *f1 = d1;
    const Enna_Vfs_File *f2 = d2;

    if (!f1->label)
        return 1;

    if (!f2->label)
        return -1;

    return strcasecmp(f1->label, f2->label);
}

static Eina_List *_browse_author_list(int64_t id_m, int64_t id_d)
{
    vh_data_t vh;
    Eina_List *list1 = NULL, *list2 = NULL;
    valhalla_db_item_t search = VALHALLA_DB_SEARCH_TEXT("album", TITLES);
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_INT(IN, id_m, id_d);
    valhalla_db_restrict_t r2 = VALHALLA_DB_RESTRICT_STR(NOTIN, "album", NULL);

    /* albums by author */
    vh.list  = &list1;
    vh.level = BROWSER_LEVEL_AUTHOR_LIST_ALBUM;
    vh.icon  = "icon/album";
    valhalla_db_metalist_get(mod->valhalla, &search, &r1, _result_dir_cb, &vh);

    /* files of the author without album */
    VALHALLA_DB_RESTRICT_LINK(r2, r1);
    valhalla_db_filelist_get(mod->valhalla, VALHALLA_FILE_TYPE_AUDIO,
                             &r1, _result_file_cb, &list2);

    list1 = eina_list_sort(list1, eina_list_count(list1), _sort_cb);
    list2 = eina_list_sort(list2, eina_list_count(list2), _sort_cb);
    return eina_list_merge(list1, list2);
}

static Eina_List *_browse_author(void)
{
    vh_data_t vh;
    Eina_List *list = NULL;
    valhalla_db_item_t search = VALHALLA_DB_SEARCH_TEXT("author", ENTITIES);

    vh.list  = &list;
    vh.level = BROWSER_LEVEL_AUTHOR_LIST;
    vh.icon  = "icon/artist";
    valhalla_db_metalist_get(mod->valhalla, &search, NULL, _result_dir_cb, &vh);

    list = eina_list_sort(list, eina_list_count(list), _sort_cb);
    return list;
}

static Eina_List *_browse_album_file(int64_t id_m, int64_t id_d)
{
    Eina_List *list = NULL;
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_INT(IN, id_m, id_d);

    valhalla_db_filelist_get(mod->valhalla, VALHALLA_FILE_TYPE_NULL,
                             &r1, _result_file_cb, &list);

    list = eina_list_sort(list, eina_list_count(list), _sort_cb);
    return list;
}

static Eina_List *_browse_album(void)
{
    vh_data_t vh;
    Eina_List *list = NULL;
    valhalla_db_item_t search = VALHALLA_DB_SEARCH_TEXT("album", TITLES);

    vh.list  = &list;
    vh.level = BROWSER_LEVEL_ALBUM_LIST;
    vh.icon  = "icon/album";
    valhalla_db_metalist_get(mod->valhalla, &search, NULL, _result_dir_cb, &vh);

    list = eina_list_sort(list, eina_list_count(list), _sort_cb);
    return list;
}

static Eina_List *_browse_genre_list(int64_t id_m, int64_t id_d)
{
    vh_data_t vh;
    Eina_List *list1 = NULL, *list2 = NULL;
    valhalla_db_item_t search = VALHALLA_DB_SEARCH_TEXT("album", TITLES);
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_INT(IN, id_m, id_d);
    valhalla_db_restrict_t r2 = VALHALLA_DB_RESTRICT_STR(NOTIN, "album", NULL);

    /* albums by genre */
    vh.list  = &list1;
    vh.level = BROWSER_LEVEL_GENRE_LIST_ALBUM;
    vh.icon  = "icon/album";
    valhalla_db_metalist_get(mod->valhalla, &search, &r1, _result_dir_cb, &vh);

    /* files of the genre without album */
    VALHALLA_DB_RESTRICT_LINK(r2, r1);
    valhalla_db_filelist_get(mod->valhalla, VALHALLA_FILE_TYPE_AUDIO,
                             &r1, _result_file_cb, &list2);

    list1 = eina_list_sort(list1, eina_list_count(list1), _sort_cb);
    list2 = eina_list_sort(list2, eina_list_count(list2), _sort_cb);
    return eina_list_merge (list1, list2);
}

static Eina_List *_browse_genre(void)
{
    vh_data_t vh;
    Eina_List *list = NULL;
    valhalla_db_item_t search =
        VALHALLA_DB_SEARCH_TEXT("genre", CLASSIFICATION);

    vh.list  = &list;
    vh.level = BROWSER_LEVEL_GENRE_LIST;
    vh.icon  = "icon/genre";
    valhalla_db_metalist_get(mod->valhalla, &search, NULL, _result_dir_cb, &vh);

    list = eina_list_sort(list, eina_list_count(list), _sort_cb);
    return list;
}

static Eina_List *_browse_unclassified_list(void)
{
    Eina_List *list = NULL;
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_STR(NOTIN, "album", NULL);
    valhalla_db_restrict_t r2 = VALHALLA_DB_RESTRICT_STR(NOTIN, "author", NULL);
    valhalla_db_restrict_t r3 = VALHALLA_DB_RESTRICT_STR(NOTIN, "genre", NULL);

    VALHALLA_DB_RESTRICT_LINK(r3, r2);
    VALHALLA_DB_RESTRICT_LINK(r2, r1);
    valhalla_db_filelist_get(mod->valhalla, VALHALLA_FILE_TYPE_AUDIO,
                             &r1, _result_file_cb, &list);

    list = eina_list_sort(list, eina_list_count(list), _sort_cb);
    return list;
}

static Eina_List *_browse_root(void)
{
    Eina_List *list = NULL;
    Enna_Vfs_File *file;
    mod->level = BROWSER_LEVEL_ROOT;
    char str[64];

    snprintf(str, sizeof(str), "%i/0:0", BROWSER_LEVEL_AUTHOR);
    file = enna_vfs_create_directory(str, _("Artists"), "icon/artist", NULL);
    list = eina_list_append(list, file);

    snprintf(str, sizeof(str), "%i/0:0", BROWSER_LEVEL_ALBUM);
    file = enna_vfs_create_directory(str, _("Albums"), "icon/album", NULL);
    list = eina_list_append(list, file);

    snprintf(str, sizeof(str), "%i/0:0", BROWSER_LEVEL_GENRE);
    file = enna_vfs_create_directory(str, _("Genres"), "icon/genre", NULL);
    list = eina_list_append(list, file);

    snprintf(str, sizeof(str), "%i/0:0", BROWSER_LEVEL_UNCLASSIFIED);
    file = enna_vfs_create_directory(str, _("Unclassified"), "icon/other", NULL);
    list = eina_list_append(list, file);

    return list;
}

static Eina_List *_class_browse_up(const char *path, void *cookie)
{
    int64_t id_m, id_d;
    int rc;

    mod->level = BROWSER_LEVEL_ROOT;

    if (!path)
        return _browse_root();

    rc = sscanf(path,
                "%i/%"PRIi64":%"PRIi64, (int *) &mod->level, &id_m, &id_d);

    if (rc != 3)
        return NULL;

    if (mod->vfs)
    {
        enna_vfs_remove(mod->vfs);
        mod->vfs = NULL;
    }

    switch (mod->level)
    {
    case BROWSER_LEVEL_ALBUM_LIST:
        mod->prev_id_m1 = id_m;
        mod->prev_id_d1 = id_d;
        return _browse_album_file(id_m, id_d);

    case BROWSER_LEVEL_AUTHOR_LIST:
        mod->prev_id_m1 = id_m;
        mod->prev_id_d1 = id_d;
        return _browse_author_list(id_m, id_d);

    case BROWSER_LEVEL_GENRE_LIST:
        mod->prev_id_m1 = id_m;
        mod->prev_id_d1 = id_d;
        return _browse_genre_list(id_m, id_d);

    case BROWSER_LEVEL_AUTHOR_LIST_ALBUM:
    case BROWSER_LEVEL_GENRE_LIST_ALBUM:
        mod->prev_id_m2 = id_m;
        mod->prev_id_d2 = id_d;
        return _browse_album_file(id_m, id_d);

    case BROWSER_LEVEL_AUTHOR:
        return _browse_author();

    case BROWSER_LEVEL_ALBUM:
        return _browse_album();

    case BROWSER_LEVEL_GENRE:
        return _browse_genre();

    case BROWSER_LEVEL_UNCLASSIFIED:
        return _browse_unclassified_list();

    default:
        break;
    }

    return NULL;
}

static Eina_List *_class_browse_down(void *cookie)
{
    switch (mod->level)
    {
    case BROWSER_LEVEL_ALBUM_LIST:
        mod->level = BROWSER_LEVEL_ALBUM;
        return _browse_album();

    case BROWSER_LEVEL_AUTHOR_LIST_ALBUM:
        mod->level = BROWSER_LEVEL_AUTHOR_LIST;
        return _browse_author_list(mod->prev_id_m1, mod->prev_id_d1);

    case BROWSER_LEVEL_AUTHOR_LIST:
        mod->level = BROWSER_LEVEL_AUTHOR;
        return _browse_author();

    case BROWSER_LEVEL_GENRE_LIST:
        mod->level = BROWSER_LEVEL_GENRE;
        return _browse_genre();

    case BROWSER_LEVEL_GENRE_LIST_ALBUM:
        mod->level = BROWSER_LEVEL_GENRE_LIST;
        return _browse_genre_list(mod->prev_id_m1, mod->prev_id_d1);

    case BROWSER_LEVEL_ALBUM:
    case BROWSER_LEVEL_AUTHOR:
    case BROWSER_LEVEL_GENRE:
    case BROWSER_LEVEL_UNCLASSIFIED:
        mod->level = BROWSER_LEVEL_ROOT;
        return _browse_root();

    default:
        break;
    }

    return NULL;
}

static Enna_Vfs_File *_class_vfs_get(void *cookie)
{
    char str[128];
    int64_t id_m, id_d;

    switch (mod->level)
    {
    case BROWSER_LEVEL_AUTHOR_LIST_ALBUM:
    case BROWSER_LEVEL_GENRE_LIST_ALBUM:
        id_m = mod->prev_id_m2;
        id_d = mod->prev_id_d2;
        break;

    case BROWSER_LEVEL_ALBUM_LIST:
    case BROWSER_LEVEL_AUTHOR_LIST:
    case BROWSER_LEVEL_GENRE_LIST:
        id_m = mod->prev_id_m1;
        id_d = mod->prev_id_d1;
        break;

    default:
        id_m = 0;
        id_d = 0;
        break;
    }

    snprintf(str, sizeof(str), "%i/%"PRIi64":%"PRIi64, mod->level, id_m, id_d);
    mod->vfs = enna_vfs_create_directory(str, NULL, NULL, NULL);
    return mod->vfs;
}

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_valhalla
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_valhalla",
    N_("Valhalla module"),
    "icon/config",
    N_("Database stuff and metadata retrival from various sources"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

static Enna_Class_Vfs class =
{
    ENNA_MODULE_NAME,
    2,
    N_("Media Library"),
    NULL,
    "icon/library",
    {
        NULL,
        NULL,
        _class_browse_up,
        _class_browse_down,
        _class_vfs_get,
    },
    NULL,
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    int flags = ENNA_CAPS_MUSIC;

    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_libvalhalla));
    if (!mod)
        return;

    mod->em = em;
    mod->level = BROWSER_LEVEL_ROOT;
    mod->valhalla = enna_metadata_get_db ();

    if (!mod->valhalla)
        return;

    enna_vfs_append(ENNA_MODULE_NAME, flags, &class);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    free(mod);
    mod = NULL;
}
