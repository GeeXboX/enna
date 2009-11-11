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

#include <dirent.h>

#include <Ecore_File.h>
#include <Ecore_Str.h>
#include <Edje.h>
#include <efreet/Efreet.h>
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

static Enna_Module_Games *mod;

/*****************************************************************************/
/*                              Games Helpers                                */
/*****************************************************************************/

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
        Efreet_Desktop *desktop;
        char dsfile[4096];

        if (!ecore_str_has_extension(dp->d_name, "desktop")) continue;
        sprintf(dsfile, "%s/%s", dir_path, dp->d_name);
        desktop = efreet_desktop_get(dsfile);
        if ((desktop = efreet_desktop_get(dsfile)))
        {
            Eina_List *l;
            const char *cat;

            EINA_LIST_FOREACH(desktop->categories, l, cat)
            {
                if(!strncmp(cat, "Game", strlen("Game")))
                {
                    char *iconpath = NULL;
                    Enna_Vfs_File *item;

                    if (ecore_file_can_read(desktop->icon))
                    {
                        iconpath = desktop->icon;
                    } else {
                        Eina_List *theme_list;
                        Eina_List *l;
                        Efreet_Icon_Theme *theme;

                        theme_list = efreet_icon_theme_list_get();
                        EINA_LIST_FOREACH(theme_list, l, theme)
                        {
                            iconpath = efreet_icon_path_find((theme->name).name, desktop->icon, 64);
                            if(iconpath)
                                break;
                        }
                    }

                    item = calloc(1, sizeof(Enna_Vfs_File));
                    item->icon = (char*)eina_stringshare_add(iconpath);
                    item->label = (char*)eina_stringshare_add(desktop->name);
                    enna_list_file_append(list, item, _play, NULL, desktop->exec);
                    break;
                }
            }
        }
        efreet_desktop_free(desktop);
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

    sprintf(gamesdir, "%s/.enna/games", enna_util_user_home_get());

    /* Populate list */
    _parse_directory(o, gamesdir);
    _parse_directory(o, "/usr/share/applications");

    enna_list_select_nth(o, 0);
    mod->o_menu = o;
    edje_object_part_swallow(mod->o_edje, "enna.swallow.menu", o);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
}

static void _create_gui(void)
{
    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    mod->o_edje = edje_object_add(enna->evas);
    edje_object_file_set(mod->o_edje, enna_config_theme_get(), "module/games");

    _create_menu();
}

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _class_init(int dummy)
{
    efreet_init();
}

static void _class_shutdown(int dummy)
{
    efreet_shutdown();
}

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
    10,
    N_("Games"),
    NULL,
    "icon/games",
    "background/games",
    {
        _class_init,
        NULL,
        _class_shutdown,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "activity_games",
    N_("System games"),
    "icon/games",
    N_("With this module you can play your games directly from enna"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Games));
    mod->em = em;
    em->mod = mod;

    /* Add activity */
    enna_activity_add(&class);
}

void module_shutdown(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);
    evas_object_del(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
    free(mod);
}
