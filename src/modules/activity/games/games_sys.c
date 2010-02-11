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

#include <Ecore.h>
#include <Elementary.h>
#include <Edje.h>

#include "enna.h"
#include "logs.h"
#include "view_list.h"
#include "ini_parser.h"
#include "games.h"
#include "games_sys.h"
#include "xdg.h"

#define ENNA_MODULE_NAME "games_system"


typedef struct _Games_Service_System {
    Evas_Object *o_edje;
    Evas_Object *o_list;
    int          count;
} Games_Service_System;

typedef struct _Game_Entry {
    char *name;
    char *icon;
    char *exec;
} Game_Entry;

static Games_Service_System *mod;

/****************************************************************************/
/*                         System Games Functions                           */
/****************************************************************************/

static char *
_check_icon(const char *icon, const char *dir, const char *ext)
{
    char tmp[PATH_MAX];
    char *format;
   
    if (ext)
        format = "%s/%s.%s";
    else
        format = "%s/%s";

    snprintf(tmp, sizeof (tmp), format, dir, icon, ext);

    if (!ecore_file_is_dir(tmp) && ecore_file_can_read(tmp))
        return strdup(tmp);
    else
        return NULL;
}

static char *
_check_icons(const char *icon, const char *dir)
{
    char *iconpath;

    if ((iconpath = _check_icon(icon, dir, NULL)))
        return iconpath;
    else if ((iconpath = _check_icon(icon, dir, "png")))
        return iconpath;
    else if ((iconpath = _check_icon(icon, dir, "xpm")))
        return iconpath;
    else if ((iconpath = _check_icon(icon, dir, "svg")))
        return iconpath;
    else
        return NULL;
}

/* Warning: this is NOT a proper implementation of the Icon Theme 
   Specification; it is hugely simplified to cover only our needs */
static char *
_find_icon(const char *icon, const char *dir)
{
    char *iconpath;
    char tmp[PATH_MAX];

    if ((iconpath = _check_icons(icon, dir)))
        return iconpath;

    snprintf(tmp, sizeof (tmp), "%s/hicolor/128x128/apps", dir);
    if ((iconpath = _check_icons(icon, strdup(tmp))))
        return iconpath;

    snprintf(tmp, sizeof (tmp), "%s/hicolor/64x64/apps", dir);
    if ((iconpath = _check_icons(icon, strdup(tmp))))
        return iconpath;
       
    snprintf(tmp, sizeof (tmp), "%s/hicolor/48x48/apps", dir);
    if ((iconpath = _check_icons(icon, strdup(tmp))))
        return iconpath;

    snprintf(tmp, sizeof (tmp), "%s/hicolor/scalable/apps", dir);
    if ((iconpath = _check_icons(icon, strdup(tmp))))
        return iconpath;

    return NULL;
}

static Game_Entry *
_parse_desktop_game(const char *file)
{
    Game_Entry *game = NULL;
    const char *categories;
    ini_t *ini = ini_new (file);

    ini_parse(ini);
  
    categories = ini_get_string(ini, "Desktop Entry", "Categories");
    if (categories && strstr(categories, "Game"))
    {
        char *tmpicon;
        const char *name = ini_get_string(ini, "Desktop Entry", "Name");
        const char *icon = ini_get_string(ini, "Desktop Entry", "Icon");
        const char *exec = ini_get_string(ini, "Desktop Entry", "Exec");
        
        game = ENNA_NEW(Game_Entry, 1);
        game->name = strdup(name);
        game->exec = strdup(exec);

        if (!ecore_file_is_dir(icon) && ecore_file_can_read(icon))
            game->icon = strdup(icon);
        else if ((tmpicon = _find_icon(icon, "/usr/share/icons")))
            game->icon = tmpicon;
        else if ((tmpicon = _find_icon(icon, "/usr/share/pixmaps")))
            game->icon = tmpicon;
    }
  
    ini_free(ini);
  
    return game;
}

static void
_play(void *data)
{
    char* game = data;

    games_service_exec(game, _("<c>System Games</c><br>Game running..."));
}

static void
_parse_directory(Evas_Object *list, const char *dir_path)
{
    struct dirent *dp;
    DIR *dir;

    if (!(dir = opendir(dir_path))) return;

    while ((dp = readdir(dir)))
    {
        Game_Entry *game;
        char dsfile[4096];

        if (!eina_str_has_extension(dp->d_name, "desktop")) continue;
        sprintf(dsfile, "%s/%s", dir_path, dp->d_name);

        if ((game = _parse_desktop_game(dsfile)))
        {
            Enna_Vfs_File *item;

            item = ENNA_NEW(Enna_Vfs_File, 1);
            item->label = strdup(game->name);
            item->is_menu = 1;
            if (game->icon) item->icon = strdup(game->icon);

            enna_list_file_append(list, item, _play, strdup(game->exec));//TODO free this dup
            mod->count++;
            ENNA_FREE(game->name);
            ENNA_FREE(game->icon);
            ENNA_FREE(game->exec);
            ENNA_FREE(game);
        }
    }
    closedir(dir);
}

static void
games_sys_create_games_list(void)
{
    Evas_Object *o;
    char gamesdir[4096];

    /* Create List */
    ENNA_OBJECT_DEL(mod->o_list);
    o = enna_list_add(enna->evas);

    /* Populate list */
    mod->count = 0;
    sprintf(gamesdir, "%s/games", enna_config_home_get());
    _parse_directory(o, gamesdir);
    _parse_directory(o, "/usr/share/applications");

    /* Select the first and 'show' */
    enna_list_select_nth(o, 0);
    mod->o_list = o;
    edje_object_part_swallow(mod->o_edje, "service.browser.swallow", o);
    games_service_total_show(mod->count);
}

/****************************************************************************/
/*                         Private Service API                              */
/****************************************************************************/

static Eina_Bool
games_sys_event(Evas_Object *edje, enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed SystemGames : %d", event);
    return enna_list_input_feed(mod->o_list, event);
}

static void
games_sys_show(Evas_Object *edje)
{

    mod = ENNA_NEW(Games_Service_System, 1);
    mod->o_edje = edje;

    games_sys_create_games_list();
}

static void
games_sys_hide(Evas_Object *edje)
{
    ENNA_OBJECT_DEL(mod->o_list);
    ENNA_FREE(mod);
}

/****************************************************************************/
/*                         Public Service API                               */
/****************************************************************************/

Games_Service games_sys = {
    N_("System Games"),
    "background/games",
    "icon/games",
    NULL,
    NULL,
    games_sys_show,
    games_sys_hide,
    games_sys_event
};
