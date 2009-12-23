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

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "activity.h"
#include "vfs.h"
#include "mediaplayer.h"
#include "mainmenu.h"
#include "content.h"
#include "view_list.h"
#include "browser.h"
#include "logs.h"
#include "mediaplayer_obj.h"
#include "volumes.h"
#include "panel_lyrics.h"
#include "module.h"

#define ENNA_MODULE_NAME "music"
#define TIMER_VALUE 30
#define TIMER_VOLUME_VALUE 3

static void _create_menu();
static void _create_gui();
static void _create_mediaplayer_gui();
static void _browse(void *data);
static void _browser_root_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_selected_cb (void *data, Evas_Object *obj, void *event_info);
static void _class_event(enna_input event);
static void _class_event_mediaplayer_view(enna_input event);

typedef struct _Enna_Module_Music Enna_Module_Music;
typedef enum _MUSIC_STATE MUSIC_STATE;
typedef struct _Music_Item_Class_Data Music_Item_Class_Data;

struct _Music_Item_Class_Data
{
    const char *icon;
    const char *label;
};

enum _MUSIC_STATE
{
    MENU_VIEW,
    BROWSER_VIEW,
    MEDIAPLAYER_VIEW
};

struct _Enna_Module_Music
{
    Evas_Object *o_edje;
    Evas_Object *o_list;
    Evas_Object *o_browser;
    Evas_Object *o_mediaplayer;
    Evas_Object *o_volume;
    Evas_Object *o_panel_lyrics;
    Ecore_Timer *timer_volume;
    Enna_Module *em;
    MUSIC_STATE state;
    Enna_Playlist *enna_playlist;
    unsigned char  accept_ev : 1;
    Elm_Genlist_Item_Class *item_class;
    int lyrics_displayed;
    Enna_Volumes_Listener *vl;
};

static Enna_Module_Music *mod;

static void
update_songs_counter (Eina_List *list)
{
    Enna_Vfs_File *f;
    Eina_List *l;
    int children = 0;
    char label[128] = { 0 };

    if (!list)
        goto end;

    EINA_LIST_FOREACH(list, l, f)
    {
        if (!f->is_directory)
            children++;
    }
    if (children)
        snprintf(label, sizeof(label), _("%d Songs"), children);
end:
    edje_object_part_text_set(mod->o_edje, "songs.counter.label", label);
}

static void
_class_event_menu_view(enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_LEFT:
        printf("left/right\n");
        enna_mediaplayer_obj_input_feed(mod->o_mediaplayer, event);
        break;
    case ENNA_INPUT_EXIT:
        enna_content_hide();
        enna_mainmenu_show();
        break;
    case ENNA_INPUT_OK:
        _browse(enna_list_selected_data_get(mod->o_list));
        break;
    default:
        enna_list_input_feed(mod->o_list, event);
        break;
    }
}

static void
_class_event_browser_view(enna_input event)
{
    if (event == ENNA_INPUT_EXIT)
        update_songs_counter(NULL);

    switch (event)
    {
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_LEFT:
        if (enna_mediaplayer_show_get(mod->o_mediaplayer))
            mod->state = MEDIAPLAYER_VIEW;
        break;
    default:
        enna_browser_input_feed(mod->o_browser, event);
    }
}

static void
_class_event_mediaplayer_view(enna_input event)
{

    if (!enna_mediaplayer_show_get(mod->o_mediaplayer))
    {
        if (mod->o_browser)
            mod->state = BROWSER_VIEW;
        else
            mod->state = MENU_VIEW;
        return;
    }

    if (enna_mediaplayer_obj_input_feed(mod->o_mediaplayer, event) == ENNA_EVENT_BLOCK)
        return;

    printf("Continue\n");
    switch (event)
    {
    case ENNA_INPUT_LEFT:
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_UP:
    case ENNA_INPUT_DOWN:
        if (mod->o_browser)
            mod->state = BROWSER_VIEW;
        else
            mod->state = MENU_VIEW;
        break;
    default:
        break;
    }

}


static void
_volume_hide_done_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
    edje_object_signal_callback_del(elm_layout_edje_get(enna->layout),
                                    "hide,done", "*", _volume_hide_done_cb);
    ENNA_OBJECT_DEL(mod->o_volume);
    mod->o_volume = NULL;
}

static int
_volume_hide_cb(void *data)
{
    edje_object_signal_callback_add(elm_layout_edje_get(enna->layout),
                                    "popup,hide,done", "*",
                                    _volume_hide_done_cb, NULL);
    edje_object_signal_emit(elm_layout_edje_get(enna->layout), "popup,hide","enna");
    mod->timer_volume = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void
_volume_gui_update()
{
    double vol = 0.0;
    if (enna_mediaplayer_mute_get())
	edje_object_signal_emit(mod->o_volume, "mute,show","enna");
    else
	edje_object_signal_emit(mod->o_volume, "mute,hide","enna");
    vol =  enna_mediaplayer_volume_get() / 100.0;
    edje_object_part_drag_value_set(mod->o_volume, "enna.dragable.pos", vol,  0.0);

}

static void
_volume_core(enna_input event)
{
    if (!mod->o_volume)
    {
        /* Volume popup doesn't exists, create it */
        mod->o_volume = edje_object_add(enna->evas);
        edje_object_file_set(mod->o_volume, enna_config_theme_get(), "enna/volume");
        elm_layout_content_set(enna->layout, "enna.popup.swallow", mod->o_volume);
    }

    /* Show volume popup */
    edje_object_signal_emit(elm_layout_edje_get(enna->layout), "popup,show","enna");
    //~ edje_object_signal_callback_del(elm_layout_edje_get(enna->layout), 
                                    //~ "hide,done", "*", _volume_hide_done_cb); //needed?

    /* Reset Timer */
    ENNA_TIMER_DEL(mod->timer_volume);
    mod->timer_volume = ecore_timer_add(TIMER_VOLUME_VALUE, _volume_hide_cb, NULL);

    /* Performs volume action*/
    switch (event)
    {
    case ENNA_INPUT_KEY_M:
        enna_mediaplayer_mute();
        break;
    case ENNA_INPUT_PLUS:
        enna_mediaplayer_default_increase_volume();
        break;
    case ENNA_INPUT_MINUS:
        enna_mediaplayer_default_decrease_volume();
    default:
        break;
    }
    _volume_gui_update();

}

static void
panel_lyrics_display (int show)
{
    if (show)
    {
        Enna_Metadata *m;

        m = enna_mediaplayer_metadata_get (mod->enna_playlist);
        enna_panel_lyrics_set_text (mod->o_panel_lyrics, m);
        edje_object_signal_emit (mod->o_edje, "lyrics,show", "enna");
        mod->lyrics_displayed = 1;
    }
    else
    {
        enna_panel_lyrics_set_text (mod->o_panel_lyrics, NULL);
        edje_object_signal_emit (mod->o_edje, "lyrics,hide", "enna");
        mod->lyrics_displayed = 0;
    }
}

static void
_browser_root_cb (void *data, Evas_Object *obj, void *event_info)
{
    mod->state = MENU_VIEW;
    evas_object_smart_callback_del(mod->o_browser, "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser, "selected", _browser_selected_cb);
    mod->accept_ev = 0;

    _create_menu();
}

static void
_browser_selected_cb (void *data, Evas_Object *obj, void *event_info)
{
    int i = 0;
    Enna_Vfs_File *f;
    Eina_List *l;
    Browser_Selected_File_Data *ev = event_info;

    if (!ev || !ev->file) return;

    if (ev->file->is_directory || ev->file->is_menu)
    {
        enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Directory Selected %s", ev->file->uri);
        update_songs_counter (ev->files);
    }
    else
    {
        enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME , "File Selected %s", ev->file->uri);
        enna_mediaplayer_playlist_stop_clear(mod->enna_playlist);
        /* File selected, create mediaplayer */
        EINA_LIST_FOREACH(ev->files, l, f)
        {
            if (!f->is_directory)
            {
                enna_mediaplayer_uri_append(mod->enna_playlist,f->uri, f->label);
                if (!strcmp(f->uri, ev->file->uri))
                {
                    enna_mediaplayer_select_nth(mod->enna_playlist,i);
                    enna_mediaplayer_play(mod->enna_playlist);
                }
                i++;
            }
        }
    }
    free(ev);
}

static void
_browse(void *data)
{
    Enna_Class_Vfs *vfs = data;

    if (!vfs) return;

    mod->accept_ev = 0;
    mod->o_browser = enna_browser_add(enna->evas);
    evas_object_smart_callback_add(mod->o_browser, "root", _browser_root_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "selected", _browser_selected_cb, NULL);

    evas_object_show(mod->o_browser);
    edje_object_part_swallow(mod->o_edje, "browser.swallow", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);

    mod->state = BROWSER_VIEW;

    ENNA_OBJECT_DEL(mod->o_list);
    mod->accept_ev = 1;

    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    mod->o_panel_lyrics = enna_panel_lyrics_add (enna->evas);
    edje_object_part_swallow (mod->o_edje,
                              "lyrics.panel.swallow", mod->o_panel_lyrics);
}

static void
_create_mediaplayer_gui()
{
    Evas_Object *o;

    o = enna_mediaplayer_obj_add(enna->evas, mod->enna_playlist);
    edje_object_part_swallow(mod->o_edje, "mediaplayer.swallow", o);
    evas_object_show(o);
    mod->o_mediaplayer = o;

    edje_object_signal_emit(mod->o_edje, "mediaplayer,show", "enna");
    edje_object_signal_emit(mod->o_edje, "content,hide", "enna");
}

static void
_create_menu()
{
    Evas_Object *o;
    Eina_List *l, *categories;
    Enna_Class_Vfs *cat;

    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create List */
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    o = enna_list_add(enna->evas);
    categories = enna_vfs_get(ENNA_CAPS_MUSIC);
    EINA_LIST_FOREACH(categories, l, cat)
    {
        Enna_Vfs_File *item;

        item = calloc(1, sizeof(Enna_Vfs_File));
        item->icon = (char*)eina_stringshare_add(cat->icon);
        item->label = (char*)eina_stringshare_add(gettext(cat->label));
        item->is_menu = 1;
        enna_list_file_append(o, item, _browse, cat);
    }

    enna_list_select_nth(o, 0);
    mod->o_list = o;
    edje_object_part_swallow(mod->o_edje, "browser.swallow", o);
    mod->accept_ev = 1;
}

static void
_refresh_list(void *data, Enna_Volume *volume)
{
    _create_menu();
}

static void
_create_gui()
{
    Evas_Object *o;

    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    o = edje_object_add(enna->evas);
    edje_object_file_set(o, enna_config_theme_get(), "activity/music");
    mod->o_edje = o;

    _create_menu();
    _create_mediaplayer_gui();

    mod->vl = enna_volumes_listener_add("activity_music", _refresh_list, _refresh_list, NULL);

}

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static void
_class_init(int dummy)
{
    _create_gui();
    enna_content_append("music", mod->o_edje);
}

static void
_class_show(int dummy)
{
    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(mod->o_edje, "module,show", "enna");
    switch (mod->state)
    {
    case MENU_VIEW:
        edje_object_signal_emit(mod->o_edje, "content,show", "enna");
        edje_object_signal_emit(mod->o_edje, "mediaplayer,hide", "enna");
        break;
    default:
        enna_log(ENNA_MSG_ERROR, ENNA_MODULE_NAME,
            "Error State Unknown in music module");
    }
}

static void
_class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void
_class_event(enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Key pressed music : %d", event);

    if (!mod->accept_ev) return;

    switch (mod->state)
    {
    case MENU_VIEW:
        _class_event_menu_view (event);
        break;
    case BROWSER_VIEW:
        _class_event_browser_view (event);
        break;
    case MEDIAPLAYER_VIEW:
        _class_event_mediaplayer_view(event);
    default:
        break;
    }
}

static Enna_Class_Activity class =
{
    ENNA_MODULE_NAME,
    1,
    N_("Music"),
    NULL,
    "icon/music",
    "background/music",
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_music
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API  =
{
    ENNA_MODULE_VERSION,
    "activity_music",
    N_("Music core"),
    "icon/music",
    N_("This is the main music module"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    if (!em)
        return;

    mod     = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;
    mod->vl = NULL;

    enna_activity_add(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    enna_volumes_listener_del(mod->vl);
    enna_activity_del(ENNA_MODULE_NAME);
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_list);
    evas_object_smart_callback_del(mod->o_browser, "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser, "selected", _browser_selected_cb);
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
}
