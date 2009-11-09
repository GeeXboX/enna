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
#define METADATA_APPLY \
    Enna_Metadata *metadata;\
    metadata = enna_mediaplayer_metadata_get(mod->enna_playlist);\
    enna_smart_player_metadata_set(mod->o_mediaplayer, metadata);\
    enna_metadata_meta_free(metadata);

#define TIMER_VALUE 30
#define TIMER_VOLUME_VALUE 3

static void _create_menu();
static void _create_gui();
static void _create_mediaplayer_gui();
static void _browse(void *data);
static void _browser_root_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_selected_cb (void *data, Evas_Object *obj, void *event_info);

static void _menu_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src);

static int _show_mediaplayer_cb(void *data);

static void _class_init(int dummy);
static void _class_show(int dummy);
static void _class_hide(int dummy);
static void _class_event(enna_input event);
static int em_init(Enna_Module *em);
static int em_shutdown(Enna_Module *em);

/*Events from mediaplayer*/
static int _eos_cb(void *data, int type, void *event);
static int _prev_cb(void *data, int type, void *event);
static int _next_cb(void *data, int type, void *event);
static int _seek_cb(void *data, int type, void *event);

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
    Ecore_Timer *timer;
    Enna_Module *em;
    MUSIC_STATE state;
    Ecore_Timer *timer_show_mediaplayer;
    Ecore_Event_Handler *eos_event_handler;
    Ecore_Event_Handler *next_event_handler;
    Ecore_Event_Handler *prev_event_handler;
    Ecore_Event_Handler *seek_event_handler;
    Enna_Playlist *enna_playlist;
    unsigned char  accept_ev : 1;
    Elm_Genlist_Item_Class *item_class;
    int lyrics_displayed;
};

static Enna_Module_Music *mod;

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "activity_music",
    "Music core",
    "icon/music",
    "This is the main music module",
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

static Enna_Class_Activity class =
{
    ENNA_MODULE_NAME,
    1,
    N_("Music"),
    NULL,
    "icon/music",
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
    case MEDIAPLAYER_VIEW:
        edje_object_signal_emit(mod->o_edje, "mediaplayer,show", "enna");
        edje_object_signal_emit(mod->o_edje, "content,hide", "enna");
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
_class_event_menu_view(enna_input event)
{
    if (mod->o_mediaplayer)
    {
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
        mod->timer_show_mediaplayer =
            ecore_timer_add (TIMER_VALUE, _show_mediaplayer_cb, NULL);
    }

    switch (event)
    {
    case ENNA_INPUT_LEFT:
    case ENNA_INPUT_EXIT:
        enna_content_hide();
        enna_mainmenu_show();
        break;
    case ENNA_INPUT_RIGHT:
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
    if (mod->o_mediaplayer)
    {
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
        mod->timer_show_mediaplayer =
            ecore_timer_add(TIMER_VALUE,_show_mediaplayer_cb, NULL);
    }
    enna_browser_input_feed(mod->o_browser, event);
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
_class_event_mediaplayer_view(enna_input event)
{

    switch (event)
    {
    case ENNA_INPUT_OK:
        enna_mediaplayer_play(mod->enna_playlist);
        break;
    case ENNA_INPUT_UP:
        panel_lyrics_display (0);
        enna_mediaplayer_prev(mod->enna_playlist);
        break;
    case ENNA_INPUT_DOWN:
        panel_lyrics_display (0);
        enna_mediaplayer_next(mod->enna_playlist);
        break;
    case ENNA_INPUT_LEFT:
        enna_mediaplayer_default_seek_backward ();
        break;
    case ENNA_INPUT_RIGHT:
        enna_mediaplayer_default_seek_forward ();
        break;
    case ENNA_INPUT_EXIT:
        panel_lyrics_display (0);
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
        mod->timer_show_mediaplayer = ecore_timer_add(TIMER_VALUE,_show_mediaplayer_cb, NULL);
        edje_object_signal_emit(mod->o_edje, "mediaplayer,hide","enna");
        edje_object_signal_emit(mod->o_edje, "content,show", "enna");
        mod->state = (mod->o_browser) ? BROWSER_VIEW : MENU_VIEW;
        break;
    case ENNA_INPUT_STOP:
    case ENNA_INPUT_KEY_S:
        panel_lyrics_display (0);
        enna_mediaplayer_playlist_stop_clear(mod->enna_playlist);
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
        edje_object_signal_emit(mod->o_edje, "mediaplayer,hide","enna");
        edje_object_signal_emit(mod->o_edje, "content,show", "enna");
        mod->state = (mod->o_browser) ? BROWSER_VIEW : MENU_VIEW;
        break;
    case ENNA_INPUT_KEY_M:
    case ENNA_INPUT_PLUS:
    case ENNA_INPUT_MINUS:
        _volume_core(event);
        break;
    case ENNA_INPUT_KEY_I:
        panel_lyrics_display (!mod->lyrics_displayed);
        break;
    default:
        break;
    }

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
        _class_event_mediaplayer_view (event);
        break;
    default:
        break;
    }
}

static void _event_mouse_down(void *data, Evas *evas, Evas_Object *obj,
        void *event_info)
{
    if (mod->o_mediaplayer)
    {
        enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Remove Timer");
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
        mod->timer_show_mediaplayer = ecore_timer_add(TIMER_VALUE,_show_mediaplayer_cb, NULL);
    }
}

static int
_show_mediaplayer_cb(void *data)
{
    if (mod->o_mediaplayer)
    {
        mod->state = MEDIAPLAYER_VIEW;
        edje_object_signal_emit(mod->o_edje, "mediaplayer,show", "enna");
        edje_object_signal_emit(mod->o_edje, "content,hide", "enna");
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
    }
    return 0;
}

static void
_menu_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{
    edje_object_signal_callback_del(mod->o_edje, "list,transition,end", "edje",
            _menu_transition_left_end_cb);
    mod->state = BROWSER_VIEW;

    //ENNA_OBJECT_DEL(mod->o_list);
    mod->accept_ev = 1;
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

    if (ev->file->is_directory)
    {
        enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Directory Selected %s", ev->file->uri);
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
        _create_mediaplayer_gui();
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
    edje_object_part_swallow(mod->o_edje, "enna.swallow.browser", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);

    edje_object_signal_callback_add(mod->o_edje, "list,transition,end", "edje",
        _menu_transition_left_end_cb, NULL);
    edje_object_signal_emit(mod->o_edje, "list,left", "enna");

    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    mod->o_panel_lyrics = enna_panel_lyrics_add (enna->evas);
    edje_object_part_swallow (mod->o_edje,
                              "lyrics.panel.swallow", mod->o_panel_lyrics);
}


static int
_update_position_timer(void *data)
{
    if(enna_mediaplayer_state_get()!=PAUSE)
    {
        double pos;
        double length;
        double percent;

        length = enna_mediaplayer_length_get();
        pos = enna_mediaplayer_position_get();
        percent = (double) enna_mediaplayer_position_percent_get() / 100.0;
        enna_smart_player_position_set(mod->o_mediaplayer, pos, length, percent);
        enna_log(ENNA_MSG_EVENT, NULL, "Position %f %f",pos,length);
    }
    return 1;
}

static int
_eos_cb(void *data, int type, void *event)
{
  /* EOS received, update metadata */
    enna_mediaplayer_next(mod->enna_playlist);
    return 1;
}

static void
_create_mediaplayer_gui()
{
    Evas_Object *o;

    mod->state = MEDIAPLAYER_VIEW;

    ENNA_TIMER_DEL(mod->timer);
    ENNA_EVENT_HANDLER_DEL(mod->eos_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->next_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->prev_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->seek_event_handler);

    if (mod->o_mediaplayer)
        evas_object_del(mod->o_mediaplayer);

    mod->eos_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_EOS, _eos_cb, NULL);
    mod->next_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_NEXT, _next_cb, NULL);
    mod->prev_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_PREV, _prev_cb, NULL);
    mod->seek_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_SEEK, _seek_cb, NULL);

    o = enna_smart_player_add(enna->evas, mod->enna_playlist);
    edje_object_part_swallow(mod->o_edje, "enna.swallow.mediaplayer", o);
    evas_object_show(o);

    mod->o_mediaplayer = o;

    METADATA_APPLY;

    mod->timer = ecore_timer_add(1, _update_position_timer, NULL);

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
        enna_list_file_append(o, item, _browse, cat);
    }

    enna_list_select_nth(o, 0);
    mod->o_list = o;
    edje_object_signal_emit(mod->o_edje, "list,left,now", "enna");
    edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o);
    edje_object_signal_emit(mod->o_edje, "list,default", "enna");
    mod->accept_ev = 1;
}

static void
_create_gui()
{
    Evas_Object *o;

    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    o = edje_object_add(enna->evas);
    edje_object_file_set(o, enna_config_theme_get(), "module/music_video");
    mod->o_edje = o;

    _create_menu();

    evas_object_event_callback_add(mod->o_edje, EVAS_CALLBACK_MOUSE_DOWN,
        _event_mouse_down, NULL);

}

/* Module interface */

static int
em_init(Enna_Module *em)
{
    mod = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;

    /* Add activity */
    enna_activity_add(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
    return 1;
}

static int
em_shutdown(Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_list);
    evas_object_smart_callback_del(mod->o_browser, "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser, "selected", _browser_selected_cb);
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
    ENNA_TIMER_DEL(mod->timer);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
    return 1;
}

void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    if (!em_init(em))
        return;
}

void
module_shutdown(Enna_Module *em)
{
    em_shutdown(em);
}

static int
_next_cb(void *data, int type, void *event)
{
    METADATA_APPLY;
    return 1;
}

static int
_prev_cb(void *data, int type, void *event)
{
    METADATA_APPLY;
    return 1;
}

static int
_seek_cb(void *data, int type, void *event)
{
    Enna_Event_Mediaplayer_Seek_Data *ev;
    double pos;
    double length;
    double percent;
    ev=event;
    percent=ev->seek_value;
    length = enna_mediaplayer_length_get();
    pos=length*percent;
    enna_smart_player_position_set(mod->o_mediaplayer, pos, length, percent);
    return 1;
}
