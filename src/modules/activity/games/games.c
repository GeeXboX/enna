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

#include <dirent.h>

#include <Ecore_File.h>
#include <Ecore_Str.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "logs.h"
#include "view_list.h"
#include "utils.h"
#include "content.h"
#include "mainmenu.h"
#include "input.h"
#include "ini_parser.h"
#include "xdg.h"

#define ENNA_MODULE_NAME "games"

static void _play(void *data);
static void _parse_directory();
static void _create_menu();
static void _create_gui();

typedef enum _GAMES_STATE GAMES_STATE;

enum _GAMES_STATE
{
    MENU_VIEW,
    GAME_VIEW
};

typedef struct _Enna_Module_Games
{
    Evas *e;
    Evas_Object *o_edje;
    Evas_Object *o_menu;
    GAMES_STATE state;
    Enna_Module *em;
} Enna_Module_Games;

typedef struct _Game_Entry
{
    const char *name;
    const char *icon;
    const char *exec;
} Game_Entry;

static Enna_Module_Games *mod;

/*****************************************************************************/
/*                              Games Helpers                                */
/*****************************************************************************/

static const char * _check_icon(const char *icon, const char *dir, const char *ext)
{
    char tmp[PATH_MAX];
    char *format;
   
    if (ext)
        format = "%s/%s.%s";
    else
        format = "%s/%s";

    snprintf(tmp, sizeof (tmp), format, dir, icon, ext);

    if (!ecore_file_is_dir(tmp) && ecore_file_can_read(tmp))
        return strdup (tmp);
    else
        return NULL;
}

static const char * _check_icons(const char *icon, const char *dir)
{
    char *iconpath;

    if ((iconpath = _check_icon(icon, dir, NULL)))
        return iconpath;
    else if ((iconpath = _check_icon(icon, dir, "svg")))
        return iconpath;
    else if ((iconpath = _check_icon(icon, dir, "png")))
        return iconpath;
    else if ((iconpath = _check_icon(icon, dir, "xpm")))
        return iconpath;
    else
        return NULL;
}

/* Warning: this is NOT a proper implementation of the Icon Theme 
   Specification; it is hugely simplified to cover only our needs */
static const char * _find_icon(const char *icon, const char *dir)
{
    char *iconpath;

    if ((iconpath = _check_icons(icon, dir)))
        return iconpath;
    else
    {
        char tmp[PATH_MAX];
        
        snprintf(tmp, sizeof (tmp), "%s/hicolor/scalable/apps", dir);
        if ((iconpath = _check_icons(icon, strdup(tmp))))
            return iconpath;
        else
        {
            snprintf(tmp, sizeof (tmp), "%s/hicolor/64x64/apps", dir);
            if ((iconpath = _check_icons(icon, strdup(tmp))))
                return iconpath;
            else
            {
                snprintf(tmp, sizeof (tmp), "%s/hicolor/48x48/apps", dir);
                if ((iconpath = _check_icons(icon, strdup(tmp))))
                    return iconpath;            
            }
        }
    }
    
    return NULL;
}

static Game_Entry * _parse_desktop_game(const char *file)
{
    Game_Entry *game = NULL;
    char *categories;
    ini_t *ini = ini_new (file);
  
    ini_parse (ini);
  
    categories = ini_get_string(ini, "Desktop Entry", "Categories");
    if (categories && strstr(categories, "Game"))
    {
        char *tmpicon;
        char *name = ini_get_string(ini, "Desktop Entry", "Name");
        char *icon = ini_get_string(ini, "Desktop Entry", "Icon");
        char *exec = ini_get_string(ini, "Desktop Entry", "Exec");
        
        game = calloc(1, sizeof(Game_Entry));
        game->name = strdup (name);
        game->exec = strdup (exec);       

        if (!ecore_file_is_dir(icon) && ecore_file_can_read(icon))
            game->icon = strdup (icon);
        else if ((tmpicon = _find_icon(icon, "/usr/share/icons")))
            game->icon = strdup (tmpicon);
        else if ((tmpicon = _find_icon(icon, "/usr/share/pixmaps")))
            game->icon = strdup (tmpicon);
    }
  
    ini_free (ini);
  
    return game;
}

static void _play(void *data)
{
    int ret;
    char* game = data;

    mod->state = GAME_VIEW;
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "starting game: %s", game);
    ret = system(game);
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "game stopped: %s", game);
    mod->state = MENU_VIEW;
}

static void _parse_directory(Evas_Object *list, const char *dir_path)
{
    struct dirent *dp;
    DIR *dir;

    if (!(dir = opendir(dir_path))) return;

    while ((dp = readdir(dir)))
    {
        Game_Entry *game;
        char dsfile[4096];

        if (!ecore_str_has_extension(dp->d_name, "desktop")) continue;
        sprintf(dsfile, "%s/%s", dir_path, dp->d_name);

        if ((game = _parse_desktop_game(dsfile)))
        {
            Enna_Vfs_File *item; 
            item = calloc(1, sizeof(Enna_Vfs_File));
            if (game->icon)
                item->icon = (char*)eina_stringshare_add(game->icon);
            item->label = (char*)eina_stringshare_add(game->name);
            item->is_menu = 1;
            enna_list_file_append(list, item, _play, strdup(game->exec));

            free (game->name);
            free (game->icon);
            free (game->exec);
            free (game);
        }
    }
    closedir(dir);
}

static void _create_menu()
{
    Evas_Object *o;
    char gamesdir[4096];

    /* Create List */
    o = enna_list_add(enna->evas);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");

    sprintf(gamesdir, "%s/games", enna_config_home_get());

    /* Populate list */
    _parse_directory(o, gamesdir);
    _parse_directory(o, "/usr/share/applications");

    enna_list_select_nth(o, 0);
    mod->o_menu = o;
    edje_object_part_swallow(mod->o_edje, "menu.swallow", o);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
}

static void _create_gui(void)
{
    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    mod->o_edje = edje_object_add(enna->evas);
    edje_object_file_set(mod->o_edje, enna_config_theme_get(), "activity/games");

    _create_menu();
}

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _class_show(int dummy)
{
    if (!mod->o_edje)
    {
        _create_gui();
        enna_content_append(ENNA_MODULE_NAME, mod->o_edje);
    }

    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
}

static void _class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void _class_event(enna_input event)
{
    switch (mod->state)
    {
        case MENU_VIEW:
            switch (event)
            {
                case ENNA_INPUT_LEFT:
                case ENNA_INPUT_EXIT:
                    enna_content_hide();
                    enna_mainmenu_show();
                    break;
                case ENNA_INPUT_RIGHT:
                case ENNA_INPUT_OK:
                    _play(enna_list_selected_data_get(mod->o_menu));
                   break;
                default:
                   enna_list_input_feed(mod->o_menu, event);
            }
            break;
        default:
            break;
    }

}

static Enna_Class_Activity
class =
{
    ENNA_MODULE_NAME,
    9,
    N_("Games"),
    NULL,
    "icon/games",
    "background/games",
    {
        NULL,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_games
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_games",
    N_("Games"),
    "icon/games",
    N_("Play all your games directly from Enna"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Games));
    mod->em = em;
    em->mod = mod;

    /* Add activity */
    enna_activity_add(&class);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);
    evas_object_del(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
    free(mod);
}
