/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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

#include <valhalla.h>

#include "enna.h"
#include "module.h"
#include "vfs.h"
#include "logs.h"
#include "utils.h"
#include "metadata.h"

#define PATH_BUFFER 4096

typedef enum _Browser_Level
{
    LEVEL_ROOT  = 0,
    LEVEL_ONE   = 1,
    LEVEL_TWO   = 2,
    LEVEL_THREE = 3,
} Browser_Level;

typedef enum _Item_Type
{
    META,
    DATALIST,
    FILELIST,
} Item_Type;

typedef struct _Callback_Data {
    Eina_List  **list;
    unsigned int it;
    valhalla_file_type_t ftype;
} Callback_Data;

typedef struct _Enna_Module_Valhalla
{
    Enna_Module   *em;
    valhalla_t    *valhalla;
    Enna_Vfs_File *vfs;
    unsigned int   it;

    int64_t prev_id_m1, prev_id_d1;
    int64_t prev_id_m2, prev_id_d2;
} Enna_Module_Valhalla;

#define VMD(m) VALHALLA_METADATA_##m

#define A_FLAG  (1 << 0)  /* audio */
#define V_FLAG  (1 << 1)  /* video */
#define I_FLAG  (1 << 2)  /* image */
#define AV_FLAG (A_FLAG | V_FLAG)

#define CHECK_FLAGS(f, t)                                   \
    (   ((t) == VALHALLA_FILE_TYPE_AUDIO && (f) & A_FLAG)   \
     || ((t) == VALHALLA_FILE_TYPE_VIDEO && (f) & V_FLAG)   \
     || ((t) == VALHALLA_FILE_TYPE_IMAGE && (f) & I_FLAG))

typedef struct _Browser_Item Browser_Item;

/*
 * Levels
 *   0   1   2   3
 *
 *   Root
 *   |-- Meta 1                   META
 *   |   |-- Data 1               DATALIST
 *   |   '-- Data 2               DATALIST
 *   |       |-- Data 2.1         DATALIST
 *   |       |-- Data 2.2         DATALIST
 *   |       |   |-- File 1       FILELIST
 *   |       |   '-- File 2       FILELIST
 *   |       |-- File 1           FILELIST
 *   |       '-- File 2           FILELIST
 *   '-- Meta 2                   META
 *
 * When a FILELIST and a DATALIST exist in the same level, the FILELIST
 * limits the results to all files where no association exists with DATALIST.
 *
 *    For example: if DATALIST is "album", then FILELIST is all files
 *                 without "album".
 *
 * If FILELIST is NULL, then the behaviour is to list the files where no
 * association exist with the DATALIST in all entries in tree_meta for the
 * same level.
 *
 *    For example: if the level is ONE, and DATALIST exist for "album" and
 *                 "author", then the null FILELIST is all files without
 *                 "album" and without "author".
 *
 * The type META must be used only in the root level. The goal is to provide
 * a list of custom entries.
 *
 * TODO: Add support for video and (maybe) photo activities.
 *       Add more metadata in the root ...
 */
static const struct
{
    Browser_Level level;
    int flags;

    struct _Browser_Item
    {
        Item_Type   type;
        const char *meta;
    } items[4];

} tree_meta[] = {
    /***************************************************************/
    /*                          AUDIO                              */
    /***************************************************************/

    /* Authors */
    { LEVEL_ROOT,  A_FLAG,  { { META,     N_("Authors")       }, } },
    { LEVEL_ONE,   A_FLAG,  { { DATALIST, VMD(AUTHOR)         }, } },
    { LEVEL_TWO,   A_FLAG,  { { DATALIST, VMD(ALBUM)          },
                              { FILELIST, VMD(AUTHOR)         }, } },
    { LEVEL_THREE, A_FLAG,  { { FILELIST, VMD(ALBUM)          }, } },

    /* Artists */
    { LEVEL_ROOT,  A_FLAG,  { { META,     N_("Artists")       }, } },
    { LEVEL_ONE,   A_FLAG,  { { DATALIST, VMD(ARTIST)         }, } },
    { LEVEL_TWO,   A_FLAG,  { { DATALIST, VMD(ALBUM)          },
                              { FILELIST, VMD(ARTIST)         }, } },
    { LEVEL_THREE, A_FLAG,  { { FILELIST, VMD(ALBUM)          }, } },

    /* Albums */
    { LEVEL_ROOT,  A_FLAG,  { { META,     N_("Albums")        }, } },
    { LEVEL_ONE,   A_FLAG,  { { DATALIST, VMD(ALBUM)          }, } },
    { LEVEL_TWO,   A_FLAG,  { { FILELIST, VMD(ALBUM)          }, } },

    /* Genres */
    { LEVEL_ROOT,  A_FLAG,  { { META,     N_("Genres")        }, } },
    { LEVEL_ONE,   A_FLAG,  { { DATALIST, VMD(GENRE)          }, } },
    { LEVEL_TWO,   A_FLAG,  { { DATALIST, VMD(ALBUM)          },
                              { FILELIST, VMD(GENRE)          }, } },
    { LEVEL_THREE, A_FLAG,  { { FILELIST, VMD(ALBUM)          }, } },


    /***************************************************************/
    /*                          VIDEO                              */
    /***************************************************************/

    /* Categories */
    { LEVEL_ROOT,  V_FLAG,  { { META,     N_("Categories")    }, } },
    { LEVEL_ONE,   V_FLAG,  { { DATALIST, VMD(CATEGORY)       }, } },
    { LEVEL_TWO,   V_FLAG,  { { FILELIST, VMD(CATEGORY)       }, } },

    /* Directors */
    { LEVEL_ROOT,  V_FLAG,  { { META,     N_("Directors")     }, } },
    { LEVEL_ONE,   V_FLAG,  { { DATALIST, VMD(DIRECTOR)       }, } },
    { LEVEL_TWO,   V_FLAG,  { { FILELIST, VMD(DIRECTOR)       }, } },


    /***************************************************************/
    /*                       AUDIO & VIDEO                         */
    /***************************************************************/

    /* Unclassified */
    { LEVEL_ROOT,  AV_FLAG, { { META,     N_("Unclassified")  }, } },
    { LEVEL_ONE,   AV_FLAG, { { FILELIST, NULL                }, } },
};

static Enna_Module_Valhalla *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void
_vfs_add_dir(Eina_List **list, unsigned int it,
             valhalla_db_metares_t *res, const char *icon)
{
    Enna_Vfs_File *file;
    char str[128];

    snprintf(str, sizeof(str),
             "%u/%"PRIi64":%"PRIi64, it + 1, res->meta_id, res->data_id);
    file = enna_vfs_create_directory(str, res->data_value, icon, NULL);
    *list = eina_list_append(*list, file);
}

static void
_vfs_add_file(Eina_List **list,
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
        if (!track && !strcmp(md_it->meta_name, "track")
            && md_it->group == VALHALLA_META_GRP_ORGANIZATIONAL)
            track = md_it->data_value;
        else if (!title && !strcmp(md_it->meta_name, "title")
                 && md_it->group == VALHALLA_META_GRP_TITLES)
            title = md_it->data_value;

        if (track && title)
            break;
    }

    if (track)
        snprintf(name, sizeof(name), "%2i - %s",
                 atoi(track), title ? title : it + 1);
    else
        snprintf(name, sizeof(name), "%s", title ? title : it + 1);

    f = enna_vfs_create_file(buf, name, icon, NULL);
    *list = eina_list_append(*list, f);
}

static int
_result_dir_cb(void *data, valhalla_db_metares_t *res)
{
    Callback_Data *vh = data;

    if (!data || !res)
        return 0;

    _vfs_add_dir(vh->list, vh->it, res, NULL);
    return 0;
}

static int
_result_file_cb(void *data, valhalla_db_fileres_t *res)
{
    Callback_Data *vh = data;
    valhalla_db_filemeta_t *metadata = NULL;
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_STR(EQUAL, "title", NULL);
    valhalla_db_restrict_t r2 = VALHALLA_DB_RESTRICT_STR(EQUAL, "track", NULL);

    if (!res)
        return 0;

    if (vh->ftype == VALHALLA_FILE_TYPE_AUDIO)
        VALHALLA_DB_RESTRICT_LINK(r2, r1);

    /* retrieve the track and the title */
    valhalla_db_file_get(mod->valhalla, res->id, NULL, &r1, &metadata);
    _vfs_add_file(vh->list, res, metadata, "icon/file/music");
    VALHALLA_DB_FILEMETA_FREE(metadata);
    return 0;
}

static int
_sort_cb(const void *d1, const void *d2)
{
    const Enna_Vfs_File *f1 = d1;
    const Enna_Vfs_File *f2 = d2;

    if (!f1->label)
        return 1;

    if (!f2->label)
        return -1;

    return strcasecmp(f1->label, f2->label);
}

static Eina_List *
_browse_list_data(const char *meta, valhalla_file_type_t ftype,
                  unsigned int it, int64_t id_m, int64_t id_d)
{
    Callback_Data vh;
    Eina_List *l = NULL;
    valhalla_db_item_t search = VALHALLA_DB_SEARCH_TEXT(meta, NIL);
    valhalla_db_restrict_t r1 = VALHALLA_DB_RESTRICT_INT(IN, id_m, id_d);
    valhalla_db_restrict_t *r = NULL;

    vh.list  = &l;
    vh.it    = it;
    vh.ftype = ftype;

    if (tree_meta[it].level > LEVEL_ONE)
        r = &r1;

    valhalla_db_metalist_get(mod->valhalla,
                             &search, ftype, r, _result_dir_cb, &vh);

    l = eina_list_sort(l, eina_list_count(l), _sort_cb);
    return l;
}

static Eina_List *
_browse_list_file(valhalla_db_restrict_t *rp, valhalla_file_type_t ftype,
                  unsigned int it, int64_t id_m, int64_t id_d)
{
    Callback_Data vh;
    Eina_List *l = NULL;
    valhalla_db_restrict_t r = VALHALLA_DB_RESTRICT_INT(IN, id_m, id_d);

    if (rp)
        VALHALLA_DB_RESTRICT_LINK(*rp, r);

    vh.list  = &l;
    vh.it    = it;
    vh.ftype = ftype;

    valhalla_db_filelist_get(mod->valhalla, ftype, &r, _result_file_cb, &vh);

    l = eina_list_sort(l, eina_list_count(l), _sort_cb);
    return l;
}

static void
_restr_add(valhalla_db_restrict_t **r,
           valhalla_db_operator_t op, const char *meta)
{
    valhalla_db_restrict_t *n, *it;

    n = calloc(1, sizeof(valhalla_db_restrict_t));
    if (!n)
        return;

    n->op        = op;
    n->meta.text = meta;
    n->meta.type = VALHALLA_DB_TYPE_TEXT;

    if (*r)
    {
        for (it = *r; it->next; it = it->next)
            ;
        it->next = n;
    }
    else
        *r = n;
}

static void
_restr_free(valhalla_db_restrict_t *r)
{
    valhalla_db_restrict_t *it;

    while (r)
    {
        it = r->next;
        free(r);
        r = it;
    }
}

static Eina_List *
_browse_list(const Browser_Item *item, valhalla_file_type_t ftype,
             unsigned int it, int64_t id_m, int64_t id_d)
{
    Eina_List *l = NULL;

    if (!item)
        return NULL;

    switch (item->type)
    {
    case META:
    {
        char str[64];
        Enna_Vfs_File *entry;

        snprintf(str, sizeof(str), "%u/0:0", it + 1);
        entry = enna_vfs_create_directory(str, item->meta, NULL, NULL);
        l = eina_list_append(l, entry);
        break;
    }

    case DATALIST:
        l = _browse_list_data(item->meta, ftype, it, id_m, id_d);
        break;

    case FILELIST:
    {
        valhalla_db_restrict_t *r = NULL;
        unsigned int i, j = it;
        unsigned int last = it + 1;

        /* special FILELIST */
        if (!item->meta)
        {
            last = ARRAY_NB_ELEMENTS(tree_meta);
            j    = 0;
        }

        for (; j < last; j++)
        {
            if (tree_meta[j].level != tree_meta[it].level)
                continue;

            for (i = 0; tree_meta[j].items[i].meta; i++)
            {
                if (tree_meta[j].items[i].type != DATALIST)
                    continue;

                _restr_add(&r, VALHALLA_DB_OPERATOR_NOTIN,
                           tree_meta[j].items[i].meta);
            }
        }

        l = _browse_list_file(r, ftype, it, id_m, id_d);
        _restr_free(r);
        break;
    }
    }

    return l;
}

static Eina_List *
_browse(valhalla_file_type_t ftype, unsigned int it, int64_t id_m, int64_t id_d)
{
    unsigned int i;
    Eina_List *l = NULL;
    const Browser_Item *item;

    if (it >= ARRAY_NB_ELEMENTS(tree_meta))
        return NULL;

    item = tree_meta[it].items;

    for (i = 0; item[i].meta || (!item[i].meta && !i); i++)
    {
        Eina_List *tmp;

        if (!CHECK_FLAGS(tree_meta[it].flags, ftype))
            continue;

        tmp = _browse_list(&item[i], ftype, it, id_m, id_d);
        l = l ? eina_list_merge(l, tmp) : tmp;

        if (!item[i].meta)
            break;
    }

    return l;
}

static Eina_List *
_browse_root(valhalla_file_type_t ftype)
{
    unsigned int i;
    Eina_List *l = NULL;

    for (i = 0; i < ARRAY_NB_ELEMENTS(tree_meta); i++)
    {
        Eina_List *tmp;

        if (tree_meta[i].level != LEVEL_ROOT)
            continue;

        if (!CHECK_FLAGS(tree_meta[i].flags, ftype))
            continue;

        tmp = _browse_list(&tree_meta[i].items[0], ftype, i, 0, 0);
        l = l ? eina_list_merge(l, tmp) : tmp;
    }

    return l;
}

static Eina_List *
_class_browse_up(const char *path, valhalla_file_type_t ftype)
{
    int64_t id_m, id_d;
    int rc;

    mod->it = 0;

    if (!path)
        return _browse_root(ftype);

    rc = sscanf(path, "%u/%"PRIi64":%"PRIi64, &mod->it, &id_m, &id_d);
    if (rc != 3)
        return NULL;

    if (mod->vfs)
    {
        enna_vfs_remove(mod->vfs);
        mod->vfs = NULL;
    }

    switch (tree_meta[mod->it].level)
    {
    case LEVEL_TWO:
        mod->prev_id_m1 = id_m;
        mod->prev_id_d1 = id_d;
        break;

    case LEVEL_THREE:
        mod->prev_id_m2 = id_m;
        mod->prev_id_d2 = id_d;
        break;

    default:
        break;
    }

    return _browse(ftype, mod->it, id_m, id_d);
}

static Eina_List *
_class_browse_up_music(const char *path, void *cookie)
{
    return _class_browse_up(path, VALHALLA_FILE_TYPE_AUDIO);
}

static Eina_List *
_class_browse_up_video(const char *path, void *cookie)
{
    return _class_browse_up(path, VALHALLA_FILE_TYPE_VIDEO);
}

static Eina_List *
_class_browse_down(valhalla_file_type_t ftype)
{
    unsigned int it;
    int64_t id_m = 0, id_d = 0;

    it = mod->it;

    if (mod->it)
      mod->it--;

    switch (tree_meta[it].level)
    {
    case LEVEL_THREE:
        id_m = mod->prev_id_m1;
        id_d = mod->prev_id_d1;
        break;

    case LEVEL_ONE:
        return _browse_root(ftype);

    case LEVEL_ROOT:
        return NULL;

    default:
        break;
    }

    return _browse(ftype, mod->it, id_m, id_d);
}

static Eina_List *
_class_browse_down_music(void *cookie)
{
    return _class_browse_down(VALHALLA_FILE_TYPE_AUDIO);
}

static Eina_List *
_class_browse_down_video(void *cookie)
{
    return _class_browse_down(VALHALLA_FILE_TYPE_VIDEO);
}

static Enna_Vfs_File *
_class_vfs_get(void *cookie)
{
    char str[128];
    int64_t id_m = 0, id_d = 0;

    switch (tree_meta[mod->it].level)
    {
    case LEVEL_THREE:
        id_m = mod->prev_id_m2;
        id_d = mod->prev_id_d2;
        break;

    case LEVEL_TWO:
        id_m = mod->prev_id_m1;
        id_d = mod->prev_id_d1;
        break;

    default:
        break;
    }

    snprintf(str, sizeof(str), "%u/%"PRIi64":%"PRIi64, mod->it, id_m, id_d);
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
    N_("Build a browseable catalog of your media files"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

static Enna_Class_Vfs class_music =
{
    "valhalla_music",
    2,
    N_("Media library"),
    NULL,
    "icon/library",
    {
        NULL,
        NULL,
        _class_browse_up_music,
        _class_browse_down_music,
        _class_vfs_get,
    },
    NULL,
};

static Enna_Class_Vfs class_video =
{
    "valhalla_video",
    2,
    N_("Media library"),
    NULL,
    "icon/library",
    {
        NULL,
        NULL,
        _class_browse_up_video,
        _class_browse_down_video,
        _class_vfs_get,
    },
    NULL,
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Valhalla));
    if (!mod)
        return;

    mod->em = em;
    mod->it = 0;
    mod->valhalla = enna_metadata_get_db();

    if (!mod->valhalla)
        return;

    enna_vfs_append("valhalla_music", ENNA_CAPS_MUSIC, &class_music);
    enna_vfs_append("valhalla_video", ENNA_CAPS_VIDEO, &class_video);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    free(mod);
    mod = NULL;
}
