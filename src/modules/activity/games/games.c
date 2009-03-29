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
#include "list.h"
#include "utils.h"
#include "content.h"
#include "mainmenu.h"
#include "event_key.h"

#define ENNA_MODULE_NAME "games"

static void _play(void *data);
static void _parse_directory();
static void _create_menu();
static void _create_gui();

typedef enum _GAMES_STATE GAMES_STATE;
typedef struct _Game_Item_Class_Data Game_Item_Class_Data;

struct _Game_Item_Class_Data
{
    const char *icon;
    const char *label;
};


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
    Elm_Genlist_Item_Class *item_class;
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
                    Game_Item_Class_Data *item;

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

                    item = calloc(1, sizeof(Game_Item_Class_Data));
                    item->icon = eina_stringshare_add(iconpath);
                    item->label = eina_stringshare_add(desktop->name);
                    enna_list_append(list, mod->item_class, item, item->label, _play, desktop->exec);
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
    o = enna_list_add(mod->em->evas);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");

    sprintf(gamesdir, "%s/.enna/games", enna_util_user_home_get());

    /* Populate list */
    _parse_directory(o, gamesdir);
    _parse_directory(o, "/usr/share/applications");

    enna_list_selected_set(o, 0);
    mod->o_menu = o;
    edje_object_part_swallow(mod->o_edje, "enna.swallow.menu", o);
    edje_object_signal_emit(mod->o_edje, "menu,show", "enna");
}

static void _create_gui(void)
{
    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    mod->o_edje = edje_object_add(mod->em->evas);
    edje_object_file_set(mod->o_edje, enna_config_theme_get(), "module/games");

    _create_menu();
}

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static void _class_init(int dummy)
{
    efreet_init();
    _create_gui();
    enna_content_append("games", mod->o_edje);
}

static void _class_shutdown(int dummy)
{
    efreet_shutdown();
}

static void _class_show(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
}

static void _class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void _class_event(void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key(ev);

    switch (mod->state)
    {
        case MENU_VIEW:
            switch (key)
            {
                case ENNA_KEY_LEFT:
                case ENNA_KEY_CANCEL:
                    enna_content_hide();
                    enna_mainmenu_show(enna->o_mainmenu);
                    break;
                case ENNA_KEY_RIGHT:
                case ENNA_KEY_OK:
                case ENNA_KEY_SPACE:
                    _play(enna_list_selected_data_get(mod->o_menu));
                   break;
                default:
                   enna_list_event_key_down(mod->o_menu, event_info);
            }
            break;
        default:
            break;
    }

}

static Enna_Class_Activity
class =
{
    "games",
    10,
    "games",
    NULL,
    "icon/games",
    {
        _class_init,
        _class_shutdown,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/* Class Item interface */
static char *_genlist_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Game_Item_Class_Data *item = data;

    if (!item) return NULL;

    return strdup(item->label);
}

static Evas_Object *_genlist_icon_get(const void *data, Evas_Object *obj, const char *part)
{
    const Game_Item_Class_Data *item = data;

    if (!item) return NULL;

    if (!strcmp(part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        ic = elm_icon_add(obj);
        elm_icon_file_set(ic, item->icon, NULL);

        evas_object_size_hint_min_set(ic, 64, 64);
        evas_object_show(ic);
        return ic;
    }

    return NULL;
}

static Evas_Bool _genlist_state_get(const void *data, Evas_Object *obj, const char *part)
{
   return 0;
}

static void _genlist_del(const void *data, Evas_Object *obj)
{
}

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_ACTIVITY,
    "activity_games"
};

void module_init(Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof(Enna_Module_Games));
    mod->em = em;
    em->mod = mod;

    /* Create Class Item */
    mod->item_class = calloc(1, sizeof(Elm_Genlist_Item_Class));

    mod->item_class->item_style     = "default";
    mod->item_class->func.label_get = _genlist_label_get;
    mod->item_class->func.icon_get  = _genlist_icon_get;
    mod->item_class->func.state_get = _genlist_state_get;
    mod->item_class->func.del = _genlist_del;

    /* Add activity */
    enna_activity_add(&class);
}

void module_shutdown(Enna_Module *em)
{
    evas_object_del(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
    free(mod);
}
