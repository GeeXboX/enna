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

/*
 * The URIs for accessing to this browser must use the following syntax :
 *
 *  /$activity/valhalla/$root/$meta_name:$data_value/...
 *
 *  where $activity must be "music" or "video",
 *        $root is an entry for the LEVEL_ROOT
 *        $meta_name:$data_value for levels greater than LEVEL_ROOT
 *          see the tree_meta array.
 *
 * for example :
 *
 *  /music/valhalla/Artists
 *  /music/valhalla/Artists/artist:Dido
 *  /music/valhalla/Artists/artist:Dido/album:Life for Rent
 *  /video/valhalla/Years/year:2010
 *  /video/valhalla/Directors/director:James Cameron
 *  /video/valhalla/Unclassified
 *
 *
 * Note that you can not pass a $meta_name which is not referenced in the
 * tree_meta array. Maybe it will be a future feature, which can be easily
 * implemented.
 *
 * FIXME: $meta_name with a slash '/' and/or ':' are not handled correctly.
 *        The content in the browser is not sorted.
 */

#include <string.h>

#include <valhalla.h>

#include "enna.h"
#include "module.h"
#include "vfs.h"
#include "logs.h"
#include "utils.h"
#include "metadata.h"
#include "buffer.h"

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

typedef enum _Meta_Name
{
    META_TITLE = 0,
    META_TRACK,
} Meta_Name;

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
#define VPL(p) VALHALLA_METADATA_PL_##p

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
        valhalla_metadata_pl_t priority;
    } items[4];

} tree_meta[] = {
    /*************************************************************************/
    /*                                AUDIO                                  */
    /*************************************************************************/

    /* Authors */
    { LEVEL_ROOT,  A_FLAG,  {{ META,     "Authors",          0            }, }},
    { LEVEL_ONE,   A_FLAG,  {{ DATALIST, VMD(AUTHOR),        VPL(HIGH)    }, }},
    { LEVEL_TWO,   A_FLAG,  {{ DATALIST, VMD(ALBUM),         VPL(HIGH)    },
                             { FILELIST, VMD(AUTHOR),        VPL(HIGH)    }, }},
    { LEVEL_THREE, A_FLAG,  {{ FILELIST, VMD(ALBUM),         VPL(HIGH)    }, }},

    /* Artists */
    { LEVEL_ROOT,  A_FLAG,  {{ META,     "Artists",          0            }, }},
    { LEVEL_ONE,   A_FLAG,  {{ DATALIST, VMD(ARTIST),        VPL(HIGH)    }, }},
    { LEVEL_TWO,   A_FLAG,  {{ DATALIST, VMD(ALBUM),         VPL(HIGH)    },
                             { FILELIST, VMD(ARTIST),        VPL(HIGH)    }, }},
    { LEVEL_THREE, A_FLAG,  {{ FILELIST, VMD(ALBUM),         VPL(HIGH)    }, }},

    /* Albums */
    { LEVEL_ROOT,  A_FLAG,  {{ META,     "Albums",           0            }, }},
    { LEVEL_ONE,   A_FLAG,  {{ DATALIST, VMD(ALBUM),         VPL(HIGH)    }, }},
    { LEVEL_TWO,   A_FLAG,  {{ FILELIST, VMD(ALBUM),         VPL(HIGH)    }, }},

    /* Genres */
    { LEVEL_ROOT,  A_FLAG,  {{ META,     "Genres",           0            }, }},
    { LEVEL_ONE,   A_FLAG,  {{ DATALIST, VMD(GENRE),         VPL(HIGH)    }, }},
    { LEVEL_TWO,   A_FLAG,  {{ DATALIST, VMD(ALBUM),         VPL(HIGH)    },
                             { FILELIST, VMD(GENRE),         VPL(HIGH)    }, }},
    { LEVEL_THREE, A_FLAG,  {{ FILELIST, VMD(ALBUM),         VPL(HIGH)    }, }},


    /*************************************************************************/
    /*                                VIDEO                                  */
    /*************************************************************************/

    /* Categories */
    { LEVEL_ROOT,  V_FLAG,  {{ META,     "Categories",       0            }, }},
    { LEVEL_ONE,   V_FLAG,  {{ DATALIST, VMD(CATEGORY),      VPL(HIGH)    }, }},
    { LEVEL_TWO,   V_FLAG,  {{ FILELIST, VMD(CATEGORY),      VPL(HIGH)    }, }},

    /* Directors */
    { LEVEL_ROOT,  V_FLAG,  {{ META,     "Directors",        0            }, }},
    { LEVEL_ONE,   V_FLAG,  {{ DATALIST, VMD(DIRECTOR),      VPL(HIGH)    }, }},
    { LEVEL_TWO,   V_FLAG,  {{ FILELIST, VMD(DIRECTOR),      VPL(HIGH)    }, }},

    /* Years */
    { LEVEL_ROOT,  V_FLAG,  {{ META,     "Years",            0            }, }},
    { LEVEL_ONE,   V_FLAG,  {{ DATALIST, VMD(YEAR),          VPL(HIGH)    }, }},
    { LEVEL_TWO,   V_FLAG,  {{ FILELIST, VMD(YEAR),          VPL(HIGH)    }, }},


    /*************************************************************************/
    /*                             AUDIO & VIDEO                             */
    /*************************************************************************/

    /* Unclassified */
    { LEVEL_ROOT,  AV_FLAG, {{ META,     "Unclassified",     0            }, }},
    { LEVEL_ONE,   AV_FLAG, {{ FILELIST, NULL,               VPL(HIGH)    }, }},
};

static Enna_Module_Valhalla *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void
_vfs_add_dir(Enna_Browser *browser, unsigned int it,
             const valhalla_db_metares_t *res, const char *icon)
{
    Enna_Buffer *uri;
    Enna_File *entry;

    uri = enna_buffer_new();
    if (!uri)
        return;

    enna_buffer_appendf(uri, enna_browser_uri_get(browser));
    enna_buffer_appendf(uri, "/%s:%s", res->meta_name, res->data_value);
    entry = enna_browser_create_directory(res->data_value,
                                          uri->buf, res->data_value, icon);
    enna_buffer_free(uri);
    enna_browser_file_add(browser, entry);
}

static void
_vfs_add_file(Enna_Browser *browser, const valhalla_db_fileres_t *file,
              const char *title, const char *track, const char *icon)
{
    Enna_Buffer *uri;
    Enna_File *entry;
    char buf[PATH_BUFFER];
    char name[256];
    char *it;

    uri = enna_buffer_new();
    if (!uri)
        return;

    snprintf(buf, sizeof(buf), "file://%s", file->path);
    it = strrchr(buf, '/');

    if (track)
        snprintf(name, sizeof(name), "%2i - %s",
                 atoi(track), title ? title : it + 1);
    else
        snprintf(name, sizeof(name), "%s", title ? title : it + 1);

    enna_buffer_appendf(uri, enna_browser_uri_get(browser));
    enna_buffer_appendf(uri, "/%s", name);
    entry = enna_browser_create_file(name, uri->buf, buf, name, icon);
    enna_buffer_free(uri);
    enna_browser_file_add(browser, entry);
}

static void
_result_file(Enna_Browser *browser,
             const valhalla_db_fileres_t *res, valhalla_file_type_t ftype)
{
    const valhalla_metadata_pl_t priority = VALHALLA_METADATA_PL_NORMAL;
    valhalla_db_stmt_t *stmt;
    const valhalla_db_metares_t *metares;
    valhalla_db_restrict_t r1 =
        VALHALLA_DB_RESTRICT_STR(EQUAL, "title", NULL, priority);
    valhalla_db_restrict_t r2 =
        VALHALLA_DB_RESTRICT_STR(EQUAL, "track", NULL, priority);
    unsigned int i;

    struct {
        const char *n;
        char *v;
    } map[] = {
        [META_TITLE]     = { "title",              NULL },
        [META_TRACK]     = { "track",              NULL },
    };

    if (!res)
        return;

    if (ftype == VALHALLA_FILE_TYPE_AUDIO)
        VALHALLA_DB_RESTRICT_LINK(r2, r1);

    /* retrieve the track and the title */
    stmt = valhalla_db_file_get(mod->valhalla, res->id, NULL, &r1);
    if (!stmt)
        return;

    while ((metares = valhalla_db_file_read(mod->valhalla, stmt)))
        for (i = 0; i < ARRAY_NB_ELEMENTS(map); i++)
            if (!map[i].v && !strcmp(metares->meta_name, map[i].n))
                map[i].v = strdup(metares->data_value);

    _vfs_add_file
        (browser, res, map[META_TITLE].v, map[META_TRACK].v, "icon/file/music");

    for (i = 0; i < ARRAY_NB_ELEMENTS(map); i++)
        if (map[i].v)
            free(map[i].v);
}

static void
_browse_list_data(Enna_Browser *browser,
                  const Browser_Item *item, valhalla_file_type_t ftype,
                  unsigned int it, const char *meta, const char *data)
{
    valhalla_db_stmt_t *stmt;
    const valhalla_db_metares_t *metares;
    valhalla_db_item_t search =
        VALHALLA_DB_SEARCH_TEXT(item->meta, NIL, item->priority);
    valhalla_db_restrict_t r1 =
        VALHALLA_DB_RESTRICT_STR(IN, meta, data, item->priority);
    valhalla_db_restrict_t *r = NULL;

    if (tree_meta[it].level > LEVEL_ONE)
        r = &r1;

    stmt = valhalla_db_metalist_get(mod->valhalla, &search, ftype, r);
    if (!stmt)
        return;

    while ((metares = valhalla_db_metalist_read(mod->valhalla, stmt)))
        _vfs_add_dir(browser, it, metares, NULL);
}

static void
_browse_list_file(Enna_Browser *browser,
                  valhalla_db_restrict_t *rp, valhalla_file_type_t ftype,
                  unsigned int it, const char *meta, const char *data,
                  valhalla_metadata_pl_t priority)
{
    valhalla_db_stmt_t *stmt;
    const valhalla_db_fileres_t *fileres;
    valhalla_db_restrict_t r =
        VALHALLA_DB_RESTRICT_STR(IN, meta, data, priority);

    if (meta && data)
    {
        if (rp)
            VALHALLA_DB_RESTRICT_LINK(*rp, r);
        rp = &r;
    }

    stmt = valhalla_db_filelist_get(mod->valhalla, ftype, rp);
    if (!stmt)
        return;

    while ((fileres = valhalla_db_filelist_read(mod->valhalla, stmt)))
        _result_file(browser, fileres, ftype);
}

static void
_restr_add(valhalla_db_restrict_t **r, valhalla_db_operator_t op,
           const char *meta, valhalla_metadata_pl_t p)
{
    valhalla_db_restrict_t *n, *it;

    n = calloc(1, sizeof(valhalla_db_restrict_t));
    if (!n)
        return;

    n->op            = op;
    n->meta.text     = meta;
    n->meta.type     = VALHALLA_DB_TYPE_TEXT;
    n->meta.priority = p;

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

static void
_browse_list(Enna_Browser *browser,
             const Browser_Item *item, valhalla_file_type_t ftype,
             unsigned int it, const char *meta, const char *data)
{
    if (!item)
        return;

    switch (item->type)
    {
    case META:
    {
        Enna_Buffer *uri;
        Enna_File *entry;

        uri = enna_buffer_new();
        if (!uri)
            break;

        enna_buffer_appendf(uri, enna_browser_uri_get(browser));
        enna_buffer_appendf(uri, "/%s", item->meta);
        entry = enna_browser_create_menu(item->meta,
                                         uri->buf, _(item->meta), NULL);
        enna_buffer_free(uri);
        enna_browser_file_add(browser, entry);
        break;
    }

    case DATALIST:
        _browse_list_data(browser, item, ftype, it, meta, data);
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

            if (!CHECK_FLAGS(tree_meta[j].flags, ftype))
                continue;

            for (i = 0; tree_meta[j].items[i].meta; i++)
            {
                if (tree_meta[j].items[i].type != DATALIST)
                    continue;

                _restr_add(&r, VALHALLA_DB_OPERATOR_NOTIN,
                           tree_meta[j].items[i].meta,
                           tree_meta[j].items[i].priority);
            }
        }

        _browse_list_file(browser, r, ftype, it, meta, data, item->priority);
        _restr_free(r);
        break;
    }
    }
}

static void
_browse(Enna_Browser *browser, valhalla_file_type_t ftype,
        unsigned int it, const char *meta, const char *data)
{
    unsigned int i;
    const Browser_Item *item;

    if (it >= ARRAY_NB_ELEMENTS(tree_meta))
        return;

    item = tree_meta[it].items;

    for (i = 0; item[i].meta || (!item[i].meta && !i); i++)
    {
        if (!CHECK_FLAGS(tree_meta[it].flags, ftype))
            continue;

        _browse_list(browser, &item[i], ftype, it, meta, data);

        if (!item[i].meta)
            break;
    }
}

static void
_browse_root(Enna_Browser *browser, valhalla_file_type_t ftype)
{
    unsigned int i;

    for (i = 0; i < ARRAY_NB_ELEMENTS(tree_meta); i++)
    {
        if (tree_meta[i].level != LEVEL_ROOT)
            continue;

        if (!CHECK_FLAGS(tree_meta[i].flags, ftype))
            continue;

        _browse_list(browser, &tree_meta[i].items[0], ftype, i, NULL, NULL);
    }
}

static void *
_class_browse_add(Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    /* TODO use it */
    return NULL;
}

static void
_class_browse_get_children(void *priv, Eina_List *tokens,
                           Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    int level, it = 0, i = 0;
    valhalla_file_type_t ftype;
    Eina_List *l;
    const char *p, *meta = NULL, *data = NULL;
    char buf[256];

    switch (caps)
    {
    case ENNA_CAPS_MUSIC:
        ftype = VALHALLA_FILE_TYPE_AUDIO;
        break;

    case ENNA_CAPS_VIDEO:
        ftype = VALHALLA_FILE_TYPE_VIDEO;
        break;

    default:
        return;
    }

    level = enna_browser_level_get(browser);

    if (level == 2)
    {
        _browse_root(browser, ftype);
        return;
    }

    if (level < 2) /* should never happen ? */
      return;

    /* Retrieve the Iterator, the meta and the data */
    EINA_LIST_FOREACH(tokens, l, p)
        switch (++i)
        {
        case 1:
        case 2:
            continue;

        case 3: /* Root entity */
        {
            unsigned int j;
            for (j = 0; j < ARRAY_NB_ELEMENTS(tree_meta); j++)
            {
                if (tree_meta[j].level != LEVEL_ROOT)
                    continue;

                if (!CHECK_FLAGS(tree_meta[j].flags, ftype))
                    continue;

                if (strcmp(tree_meta[j].items[0].meta, p))
                    continue;

                it = j + 1; /* set iterator for LEVEL_ROOT */
                break;
            }
            break;
        }

        default: /* Level one and more with the meta prepended */
        {
            unsigned int j;
            for (j = 0; j < ARRAY_NB_ELEMENTS(tree_meta); j++)
            {
                unsigned int k = 0;

                if (tree_meta[j].level != i - 3)
                    continue;

                if (!CHECK_FLAGS(tree_meta[j].flags, ftype))
                    continue;

                for (k = 0; tree_meta[j].items[k].meta; k++)
                {
                    const char *chr;

                    chr = strchr(p, ':');
                    if (!chr)
                        continue;

                    if (chr - p >= sizeof(buf)) /* stupid length */
                        break;

                    strncpy(buf, p, chr - p);
                    buf[chr - p] = '\0';

                    if (!strcmp(tree_meta[j].items[k].meta, buf)) /* found */
                    {
                        meta = buf;
                        data = chr + 1;
                        break;
                    }
                }
            }
            it++;
            break;
        }
        }

    enna_log(ENNA_MSG_EVENT, "valhalla", "%u %s %s", it, meta, data);

    _browse(browser, ftype, it, meta, data);
}

static void
_class_browse_del(void *priv)
{

}


/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_valhalla
#endif /* USE_STATIC_MODULES */

static Enna_Vfs_Class class =
{
    "valhalla",
    2,
    N_("Media library"),
    NULL,
    "icon/library",
    {
        _class_browse_add,
        _class_browse_get_children,
        _class_browse_del,
    },
    NULL,
};

static void
module_init(Enna_Module *em)
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

    enna_vfs_register(&class, ENNA_CAPS_MUSIC | ENNA_CAPS_VIDEO);
}

static void
module_shutdown(Enna_Module *em)
{
    free(mod);
    mod = NULL;
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_valhalla",
    N_("Valhalla module"),
    "icon/config",
    N_("Build a browseable catalog of your media files"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
