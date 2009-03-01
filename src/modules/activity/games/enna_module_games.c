/* Interface */

#include "enna.h"
#include <efreet/Efreet.h>
#include <dirent.h>

#define ENNA_MODULE_NAME "games"

static void _play();
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

static void _play(void *data, void *data2)
{   
    char* game = data;
    
    mod->state = GAME_VIEW;
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "starting game: %s", game);
    system(game);
    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "game stopped: %s", game);
    mod->state = MENU_VIEW;
}

static void _parse_directory(Evas_Object *list, const char *dir_path)
{
    struct dirent *dp;
    DIR *dir;
    
    if (!(dir = opendir(dir_path))) return;
    
    while ((dp = readdir(dir))) {
        Efreet_Desktop *desktop;
        char dsfile[4096];
        
        if (!ecore_str_has_extension(dp->d_name, "desktop")) continue;
        sprintf(dsfile, "%s/%s", dir_path, dp->d_name);
        desktop = efreet_desktop_get(dsfile);
        if ((desktop = efreet_desktop_get(dsfile))) {
            Eina_List *l;
            const char *cat;
            EINA_LIST_FOREACH(desktop->categories, l, cat) {
                if(strncmp(cat, "Game", strlen("Game")) == 0) {
                    Evas_Object *item;
                    Evas_Object *icon;
                    char *iconpath;
                    
                    if (ecore_file_can_read(desktop->icon)) {
                        iconpath = desktop->icon;
                    } else {
                        //FIXME fails with icons like "gnome-nibbles"
                        iconpath = efreet_icon_path_find(NULL, desktop->icon, 16);
                    }
                    icon = enna_image_add(mod->em->evas);
                    enna_image_file_set(icon, iconpath);
                    item = enna_listitem_add(mod->em->evas);
                    enna_listitem_create_simple(item, icon, desktop->name);
                    enna_list_append(list, item, _play, NULL, desktop->exec, NULL);
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
                    _play(enna_list_selected_data_get(mod->o_menu), NULL);
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
{ "games", 1, "games", NULL, "icon/games",
  { _class_init, _class_shutdown, _class_show, _class_hide,
    _class_event }, NULL };

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

    enna_activity_add(&class);
}

void module_shutdown(Enna_Module *em)
{
    evas_object_del(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_menu);
    free(mod);
}
