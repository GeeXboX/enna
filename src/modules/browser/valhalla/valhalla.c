/*
 * Valhalla browser module
 *  Mathieu Schroeter (C) 2009
 */

#include "enna.h"
#include <valhalla.h>

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

typedef struct _Enna_Module_libvalhalla
{
    Evas            *evas;
    Enna_Module     *em;
    valhalla_t      *valhalla;
    browser_level_t  level;
    int64_t          prev_id1;
    int64_t          prev_id2;
} Enna_Module_libvalhalla;

static Enna_Module_libvalhalla *mod;

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static valhalla_db_file_where_t *_args_add(valhalla_db_file_where_t **args)
{
    valhalla_db_file_where_t *new;

    if (!args)
        return NULL;

    if (!*args)
    {
        *args = calloc(1, sizeof(valhalla_db_file_where_t));
        new = *args;
    }
    else
    {
        new = *args;;
        for (; new->next; new = new->next)
            ;
        new->next = calloc(1, sizeof(valhalla_db_file_where_t));
        new = new->next;
    }

    return new;
}

static void _args_free(valhalla_db_file_where_t *args)
{
    valhalla_db_file_where_t *next;

    while (args)
    {
        next = args->next;
        free(args);
        args = next;
    }
}

static void _vfs_add_dir(Eina_List **list, int level,
                         int64_t id, const char *name, const char *icon)
{
    Enna_Vfs_File *file;
    char str[64];

    snprintf(str, sizeof(str), "%i/%"PRIi64, level, id);
    file = enna_vfs_create_directory(str, name, icon, NULL);
    *list = eina_list_append(*list, file);
}

static void _vfs_add_file(Eina_List **list,
                          const valhalla_db_file_t *file, const char *icon)
{
    Enna_Vfs_File *f;
    char buf[PATH_BUFFER];
    char title[256];
    char *it;

    snprintf(buf, sizeof(buf), "file://%s", file->path);
    it = strrchr(buf, '/');

    if (file->track)
        snprintf(title, sizeof(title),
                 "%s - %s", file->track, file->title ? file->title : it + 1);
    else
        snprintf(title, sizeof(title),
                 "%s", file->title ? file->title : it + 1);

    f = enna_vfs_create_file(buf, title, icon, NULL);
    *list = eina_list_append(*list, f);
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

static Eina_List *_browse_author_list(int64_t id)
{
    Eina_List *list = NULL;
    valhalla_db_album_t album;
    valhalla_db_file_t file;
    valhalla_db_file_where_t *args = NULL, *new;

    /* albums by author */
    while (!valhalla_db_album(mod->valhalla, &album, id, 0))
    {
        _vfs_add_dir(&list, BROWSER_LEVEL_AUTHOR_LIST_ALBUM,
                     album.id, album.name, "icon/album");
        new = _args_add(&args);
        if (new)
        {
            new->album_id = album.id;
            new->album_w = VALHALLA_DB_NOTEQUAL;
        }
    }

    new = _args_add(&args);
    if (new)
    {
        new->author_id = id;
        new->author_w = VALHALLA_DB_EQUAL;
    }

    /* files of the author without album */
    while (!valhalla_db_file(mod->valhalla, &file, args))
        _vfs_add_file(&list, &file, "icon/file/music");

    _args_free(args);

    return list;
}

static Eina_List *_browse_author(void)
{
    Eina_List *list = NULL;
    valhalla_db_author_t author;

    while (!valhalla_db_author(mod->valhalla, &author, 0))
        _vfs_add_dir(&list, BROWSER_LEVEL_AUTHOR_LIST,
                     author.id, author.name, "icon/artist");

    return list;
}

static Eina_List *_browse_album_file(int64_t id)
{
    Eina_List *list = NULL;
    valhalla_db_file_t file;
    valhalla_db_file_where_t args;

    memset(&args, 0, sizeof(valhalla_db_file_where_t));
    args.album_id = id;
    args.album_w = VALHALLA_DB_EQUAL;

    while (!valhalla_db_file(mod->valhalla, &file, &args))
        _vfs_add_file(&list, &file, "icon/file/music");

    return list;
}

static Eina_List *_browse_album(void)
{
    Eina_List *list = NULL;
    valhalla_db_album_t album;

    while (!valhalla_db_album(mod->valhalla, &album, 0, 0))
        _vfs_add_dir(&list, BROWSER_LEVEL_ALBUM_LIST,
                     album.id, album.name, "icon/album");

    return list;
}

static Eina_List *_browse_genre_list(int64_t id)
{
    Eina_List *list = NULL;
    valhalla_db_album_t album;
    valhalla_db_file_t file;
    valhalla_db_file_where_t *args = NULL, *new;

    /* albums by genre */
    while (!valhalla_db_album(mod->valhalla, &album, id, 1))
    {
        _vfs_add_dir(&list, BROWSER_LEVEL_GENRE_LIST_ALBUM,
                     album.id, album.name, "icon/album");
        new = _args_add(&args);
        if (new)
        {
            new->album_id = album.id;
            new->album_w = VALHALLA_DB_NOTEQUAL;
        }
    }

    new = _args_add(&args);
    if (new)
    {
        new->genre_id = id;
        new->genre_w = VALHALLA_DB_EQUAL;
    }

    /* files of the genre without album */
    while (!valhalla_db_file(mod->valhalla, &file, args))
        _vfs_add_file(&list, &file, "icon/file/music");

    _args_free(args);

    return list;
}

static Eina_List *_browse_genre(void)
{
    Eina_List *list = NULL;
    valhalla_db_genre_t genre;

    while (!valhalla_db_genre(mod->valhalla, &genre))
        _vfs_add_dir(&list, BROWSER_LEVEL_GENRE_LIST,
                     genre.id, genre.name, "icon/genre");

    return list;
}

static Eina_List *_browse_unclassified_list(void)
{
    Eina_List *list = NULL;
    valhalla_db_file_t file;
    valhalla_db_file_where_t args;

    memset(&args, 0, sizeof(valhalla_db_file_where_t));
    args.author_w = VALHALLA_DB_NULL;
    args.album_w  = VALHALLA_DB_NULL;
    args.genre_w  = VALHALLA_DB_NULL;

    while (!valhalla_db_file(mod->valhalla, &file, &args))
        _vfs_add_file(&list, &file, "icon/file/music");

    list = eina_list_sort(list, eina_list_count(list), _sort_cb);
    return list;
}

static Eina_List *_browse_root(void)
{
    Eina_List *list = NULL;
    Enna_Vfs_File *file;
    mod->level = BROWSER_LEVEL_ROOT;
    char str[64];

    snprintf(str, sizeof(str), "%i/0", BROWSER_LEVEL_AUTHOR);
    file = enna_vfs_create_directory(str, "Artists", "icon/artist", NULL);
    list = eina_list_append(list, file);

    snprintf(str, sizeof(str), "%i/0", BROWSER_LEVEL_ALBUM);
    file = enna_vfs_create_directory(str, "Albums", "icon/album", NULL);
    list = eina_list_append(list, file);

    snprintf(str, sizeof(str), "%i/0", BROWSER_LEVEL_GENRE);
    file = enna_vfs_create_directory(str, "Genres", "icon/genre", NULL);
    list = eina_list_append(list, file);

    snprintf(str, sizeof(str), "%i/0", BROWSER_LEVEL_UNCLASSIFIED);
    file = enna_vfs_create_directory(str, "Unclassified", "icon/other", NULL);
    list = eina_list_append(list, file);

    return list;
}

static Eina_List *_class_browse_up(const char *path, void *cookie)
{
    int64_t id;
    int rc;

    mod->level = BROWSER_LEVEL_ROOT;

    if (!path)
        return _browse_root();

    rc = sscanf(path, "%i/%"PRIi64, (int *) &mod->level, &id);

    if (rc != 2)
        return NULL;

    switch (mod->level)
    {
    case BROWSER_LEVEL_ALBUM_LIST:
        mod->prev_id1 = id;
        return _browse_album_file(id);

    case BROWSER_LEVEL_AUTHOR_LIST:
        mod->prev_id1 = id;
        return _browse_author_list(id);

    case BROWSER_LEVEL_GENRE_LIST:
        mod->prev_id1 = id;
        return _browse_genre_list(id);

    case BROWSER_LEVEL_AUTHOR_LIST_ALBUM:
    case BROWSER_LEVEL_GENRE_LIST_ALBUM:
        mod->prev_id2 = id;
        return _browse_album_file(id);

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
        return _browse_author_list(mod->prev_id1);

    case BROWSER_LEVEL_AUTHOR_LIST:
        mod->level = BROWSER_LEVEL_AUTHOR;
        return _browse_author();

    case BROWSER_LEVEL_GENRE_LIST:
        mod->level = BROWSER_LEVEL_GENRE;
        return _browse_genre();

    case BROWSER_LEVEL_GENRE_LIST_ALBUM:
        mod->level = BROWSER_LEVEL_GENRE_LIST;
        return _browse_genre_list(mod->prev_id1);

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
    char str[64];
    int64_t id;

    switch (mod->level)
    {
    case BROWSER_LEVEL_AUTHOR_LIST_ALBUM:
    case BROWSER_LEVEL_GENRE_LIST_ALBUM:
        id = mod->prev_id2;
        break;

    case BROWSER_LEVEL_ALBUM_LIST:
    case BROWSER_LEVEL_AUTHOR_LIST:
    case BROWSER_LEVEL_GENRE_LIST:
        id = mod->prev_id1;
        break;

    default:
        id = 0;
        break;
    }

    snprintf(str, sizeof(str), "%i/%"PRIi64, mod->level, id);
    return enna_vfs_create_directory(str, NULL, NULL, NULL);
}

static int _em_valhalla_init(void)
{
    int rc;
    Enna_Config_Data *cfgdata;
    char *value = NULL;
    char db[PATH_BUFFER];
    Eina_List *path = NULL, *music_ext = NULL, *l;
    int parser_number   = 2;
    int commit_interval = 128;
    int scan_loop       = -1;
    int scan_sleep      = 900;
    int scan_priority   = 19;
    valhalla_verb_t verbosity = VALHALLA_MSG_WARNING;

    cfgdata = enna_config_module_pair_get("enna");
    if (cfgdata)
    {
        Eina_List *list;
        for (list = cfgdata->pair; list; list = list->next)
        {
            Config_Pair *pair = list->data;
            enna_config_value_store (&music_ext, "music_ext",
                                     ENNA_CONFIG_STRING_LIST, pair);
        }
    }

    cfgdata = enna_config_module_pair_get("valhalla");
    if (cfgdata)
    {
        Eina_List *list;

        for (list = cfgdata->pair; list; list = list->next)
        {
            Config_Pair *pair = list->data;

            enna_config_value_store(&parser_number, "parser_number",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&commit_interval, "commit_interval",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_loop, "scan_loop",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_sleep, "scan_sleep",
                                    ENNA_CONFIG_INT, pair);
            enna_config_value_store(&scan_priority, "scan_priority",
                                    ENNA_CONFIG_INT, pair);

            if (!strcmp("path", pair->key))
            {
                enna_config_value_store(&value, "path",
                                        ENNA_CONFIG_STRING, pair);
                if (strstr(value, "file://") == value)
                    path = eina_list_append(path, value + 7);
            }
            else if (!strcmp("verbosity", pair->key))
            {
                enna_config_value_store(&value, "verbosity",
                                        ENNA_CONFIG_STRING, pair);

                if (!strcmp("verbose", value))
                    verbosity = VALHALLA_MSG_VERBOSE;
                else if (!strcmp("info", value))
                    verbosity = VALHALLA_MSG_INFO;
                else if (!strcmp("warning", value))
                    verbosity = VALHALLA_MSG_WARNING;
                else if (!strcmp("error", value))
                    verbosity = VALHALLA_MSG_ERROR;
                else if (!strcmp("critical", value))
                    verbosity = VALHALLA_MSG_CRITICAL;
                else if (!strcmp("none", value))
                    verbosity = VALHALLA_MSG_NONE;
            }
        }
    }

    /* Configuration */
    enna_log(ENNA_MSG_INFO,
             ENNA_MODULE_NAME, "* parser number  : %i", parser_number);
    enna_log(ENNA_MSG_INFO,
             ENNA_MODULE_NAME, "* commit interval: %i", commit_interval);
    enna_log(ENNA_MSG_INFO,
             ENNA_MODULE_NAME, "* scan loop      : %i", scan_loop);
    enna_log(ENNA_MSG_INFO,
             ENNA_MODULE_NAME, "* scan sleep     : %i", scan_sleep);
    enna_log(ENNA_MSG_INFO,
             ENNA_MODULE_NAME, "* scan priority  : %i", scan_priority);
    enna_log(ENNA_MSG_INFO,
             ENNA_MODULE_NAME, "* verbosity      : %i", verbosity);

    valhalla_verbosity(verbosity);

    snprintf(db, sizeof(db),
             "%s/.enna/%s", enna_util_user_home_get(), "valhalla_music.db");

    mod->valhalla = valhalla_init(db, parser_number, commit_interval);
    if (!mod->valhalla)
        goto err;

    /* Add file suffixes */
    for (l = music_ext; l; l = l->next)
    {
        const char *ext = l->data;
        valhalla_suffix_add(mod->valhalla, ext);
    }
    if (music_ext)
        eina_list_free(music_ext);

    /* Add paths */
    for (l = path; l; l = l->next)
    {
        const char *str = l->data;
        valhalla_path_add(mod->valhalla, str, 1);
    }
    if (path)
        eina_list_free(path);

    rc = valhalla_run(mod->valhalla, scan_loop, scan_sleep, scan_priority);
    if (rc)
    {
        enna_log(ENNA_MSG_ERROR,
                 ENNA_MODULE_NAME, "valhalla returns error code: %i", rc);
        valhalla_uninit(mod->valhalla);
        mod->valhalla = NULL;
        goto err;
    }

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Valkyries are running");
    return 0;

 err:
    enna_log(ENNA_MSG_ERROR,
             ENNA_MODULE_NAME, "valhalla module initialization");
    if (music_ext)
        eina_list_free(music_ext);
    if (path)
        eina_list_free(path);
    return -1;
}

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_BROWSER,
    "browser_valhalla",
};

static Enna_Class_Vfs class =
{
    ENNA_MODULE_NAME,
    1,
    "Media Library",
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

void module_init(Enna_Module *em)
{
    int flags = ENNA_CAPS_MUSIC;

    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_libvalhalla));
    if (!mod)
        return;

    mod->em = em;
    mod->evas = em->evas;
    mod->level = BROWSER_LEVEL_ROOT;

    if (_em_valhalla_init())
        return;

    enna_vfs_append(ENNA_MODULE_NAME, flags, &class);
}

void module_shutdown(Enna_Module *em)
{
    valhalla_uninit(mod->valhalla);
    free(mod);
    mod = NULL;
}
