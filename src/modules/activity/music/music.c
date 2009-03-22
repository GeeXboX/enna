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
#include "location.h"
#include "mainmenu.h"
#include "content.h"
#include "list.h"
#include "browser.h"
#include "logs.h"
#include "event_key.h"
#include "smart_player.h"

#define ENNA_MODULE_NAME "music"
#define METADATA_APPLY \
    Enna_Metadata *metadata;\
    metadata = enna_mediaplayer_metadata_get(mod->enna_playlist);\
    enna_metadata_grab (metadata,\
                        ENNA_GRABBER_CAP_AUDIO | ENNA_GRABBER_CAP_COVER);\
    enna_smart_player_metadata_set(mod->o_mediaplayer, metadata);

static void _create_menu();
static void _create_gui();
static void _create_mediaplayer_gui();
static void _browse(void *data);
static void _browser_root_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_selected_cb (void *data, Evas_Object *obj, void *event_info);
static void _browser_browse_down_cb (void *data, Evas_Object *obj, void *event_info);

static void _menu_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src);

static void _next_song(void);
static void _prev_song(void);
static int _show_mediaplayer_cb(void *data);

static void _class_init(int dummy);
static void _class_shutdown(int dummy);
static void _class_show(int dummy);
static void _class_hide(int dummy);
static void _class_event(void *event_info);
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
    Evas_Object *o_location;
    Evas_Object *o_mediaplayer;
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
};

static Enna_Module_Music *mod;

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_ACTIVITY,
    "activity_music"
};

static Enna_Class_Activity class =
{
    "music",
    1,
    "music",
    NULL,
    "icon/music",
    {
	_class_init,
	_class_shutdown,
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
_class_shutdown(int dummy)
{
}

static void
_class_show(int dummy)
{
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
	    "Error State Unknown in music module\n");
    }
}

static void
_class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void
_class_event(void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key(ev);
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Key pressed music : %s\n",
	ev->key);

    if (!mod->accept_ev) return;

    switch (mod->state)
    {
    case MENU_VIEW:
	if (mod->o_mediaplayer)
	{
	    ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
	    mod->timer_show_mediaplayer = ecore_timer_add(10,_show_mediaplayer_cb, NULL);
	}
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
	    _browse(enna_list_selected_data_get(mod->o_list));
	    break;
	default:
	    enna_list_event_key_down(mod->o_list, event_info);
	}
	break;
    case BROWSER_VIEW:
	if (mod->o_mediaplayer)
	{
	    ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
	    mod->timer_show_mediaplayer = ecore_timer_add(10,_show_mediaplayer_cb, NULL);
	}
	enna_browser_event_feed(mod->o_browser, event_info);
	break;
    case MEDIAPLAYER_VIEW:
	switch (key)
	{
	case ENNA_KEY_OK:
	case ENNA_KEY_SPACE:
	    enna_mediaplayer_play(mod->enna_playlist);
	    break;
	case ENNA_KEY_RIGHT:
	    _next_song();
	    break;
	case ENNA_KEY_LEFT:
	    _prev_song();
	    break;
	case ENNA_KEY_CANCEL:
	    ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
	    mod->timer_show_mediaplayer = ecore_timer_add(10,_show_mediaplayer_cb, NULL);
	    enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "Add timer 10s");
	    edje_object_signal_emit(mod->o_edje, "mediaplayer,hide","enna");
	    edje_object_signal_emit(mod->o_edje, "content,show", "enna");
	    if (mod->o_browser)
		mod->state = BROWSER_VIEW;
	    else
		mod->state = MENU_VIEW;
	    break;
	default:
	    break;
	}
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
	enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME, "Remove Timer");
	ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
	mod->timer_show_mediaplayer = ecore_timer_add(10,_show_mediaplayer_cb, NULL);
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

    evas_object_del(mod->o_list);
    mod->o_list = NULL;
    mod->accept_ev = 1;
}

static void
_browser_root_cb (void *data, Evas_Object *obj, void *event_info)
{
    mod->state = MENU_VIEW;
    evas_object_smart_callback_del(mod->o_browser, "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser, "selected", _browser_selected_cb);
    evas_object_smart_callback_del(mod->o_browser, "browse_down", _browser_browse_down_cb);
    mod->accept_ev = 0;

    _create_menu();

    enna_location_remove_nth(mod->o_location,enna_location_count(mod->o_location) - 1);
}

static void
_browser_browse_down_cb (void *data, Evas_Object *obj, void *event_info)
{
    int n;
    const char *label ;

    n = enna_location_count(mod->o_location) - 1;
    label = enna_location_label_get_nth(mod->o_location, n);
    enna_browser_select_label(mod->o_browser, label);
    enna_location_remove_nth(mod->o_location, n);
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
	enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Directory Selected %s\n", ev->file->uri);
	enna_location_append(mod->o_location, ev->file->label, NULL, NULL, NULL, NULL);
    }
    else
    {
	enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME , "File Selected %s\n", ev->file->uri);
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
    mod->o_browser = enna_browser_add(mod->em->evas);
    evas_object_smart_callback_add(mod->o_browser, "root", _browser_root_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "selected", _browser_selected_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "browse_down", _browser_browse_down_cb, NULL);

    evas_object_show(mod->o_browser);
    edje_object_part_swallow(mod->o_edje, "enna.swallow.browser", mod->o_browser);
    enna_browser_root_set(mod->o_browser, vfs);

    enna_location_append(mod->o_location, vfs->label, NULL, NULL, NULL, NULL);
    edje_object_signal_callback_add(mod->o_edje, "list,transition,end", "edje",
	_menu_transition_left_end_cb, NULL);
    edje_object_signal_emit(mod->o_edje, "list,left", "enna");
}



static int
_update_position_timer(void *data)
{
    if(enna_mediaplayer_state_get()!=PAUSE)
    {
        double pos;
        double length;

        length = enna_mediaplayer_length_get();
        pos = enna_mediaplayer_position_get();
        enna_smart_player_position_set(mod->o_mediaplayer, pos, length);
        enna_log(ENNA_MSG_EVENT, NULL, "Position %f %f",pos,length);
    }
    return 1;
}

static void
_next_song()
{
    enna_mediaplayer_next(mod->enna_playlist);
}

static void
_prev_song()
{
    enna_mediaplayer_prev(mod->enna_playlist);
}

static int
_eos_cb(void *data, int type, void *event)
{
    /* EOS received, update metadata */
    _next_song();
    return 1;
}

static void
_create_mediaplayer_gui()
{
    Evas_Object *o;

    mod->state = MEDIAPLAYER_VIEW;

    if (mod->o_mediaplayer)
    {
        ENNA_TIMER_DEL(mod->timer);
        ecore_event_handler_del(mod->eos_event_handler);
        evas_object_del(mod->o_mediaplayer);
        ecore_event_handler_del(mod->next_event_handler);
        ecore_event_handler_del(mod->prev_event_handler);
        ecore_event_handler_del(mod->seek_event_handler);
    }

    mod->eos_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_EOS, _eos_cb, NULL);
    mod->next_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_NEXT, _next_cb, NULL);
    mod->prev_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_PREV, _prev_cb, NULL);
    mod->seek_event_handler = ecore_event_handler_add(
            ENNA_EVENT_MEDIAPLAYER_SEEK, _seek_cb, NULL);

    o = enna_smart_player_add(mod->em->evas,mod->enna_playlist);
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
    mod->o_browser = NULL;
    o = enna_list_add(mod->em->evas);
    categories = enna_vfs_get(ENNA_CAPS_MUSIC);
    EINA_LIST_FOREACH(categories, l, cat)
    {
   	Music_Item_Class_Data *item;

	item = calloc(1, sizeof(Music_Item_Class_Data));
	item->icon = eina_stringshare_add(cat->icon);
	item->label = eina_stringshare_add(cat->label);
        enna_list_append(o, mod->item_class, item, item->label, _browse, cat);
    }

    enna_list_selected_set(o, 0);
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
    Evas_Object *icon;

    /* Set default state */
    mod->state = MENU_VIEW;

    /* Create main edje object */
    o = edje_object_add(mod->em->evas);
    edje_object_file_set(o, enna_config_theme_get(), "module/music_video");
    mod->o_edje = o;

    _create_menu();

    /* Create Location bar */
    o = enna_location_add(mod->em->evas);
    edje_object_part_swallow(mod->o_edje, "enna.swallow.location", o);

    icon = edje_object_add(mod->em->evas);
    edje_object_file_set(icon, enna_config_theme_get(), "icon/music_mini");
    enna_location_append(o, "Music", icon, NULL, NULL, NULL);
    mod->o_location = o;

    evas_object_event_callback_add(mod->o_edje, EVAS_CALLBACK_MOUSE_DOWN,
	_event_mouse_down, NULL);

}

/* Class Item interface */
static char *_genlist_label_get(const void *data, Evas_Object *obj, const char *part)
{
    const Music_Item_Class_Data *item = data;

    if (!item) return NULL;

    return strdup(item->label);
}

static Evas_Object *_genlist_icon_get(const void *data, Evas_Object *obj, const char *part)
{
    const Music_Item_Class_Data *item = data;

    if (!item) return NULL;

    if (!strcmp(part, "elm.swallow.icon"))
     {
	 Evas_Object *ic;

	 ic = elm_icon_add(obj);
	 elm_icon_file_set(ic, enna_config_theme_get(), item->icon);
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

/* Module interface */

static int
em_init(Enna_Module *em)
{
    mod = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;

    /* Create Class Item */
    mod->item_class = calloc(1, sizeof(Elm_Genlist_Item_Class));

    mod->item_class->item_style     = "default";
    mod->item_class->func.label_get = _genlist_label_get;
    mod->item_class->func.icon_get  = _genlist_icon_get;
    mod->item_class->func.state_get = _genlist_state_get;
    mod->item_class->func.del       = _genlist_del;

    /* Add activity */
    enna_activity_add(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
    return 1;
}

static int
em_shutdown(Enna_Module *em)
{
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_list);
    evas_object_smart_callback_del(mod->o_browser, "root", _browser_root_cb);
    evas_object_smart_callback_del(mod->o_browser, "selected", _browser_selected_cb);
    evas_object_smart_callback_del(mod->o_browser, "browse_down", _browser_browse_down_cb);
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_location);
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
    enna_smart_player_position_set(mod->o_mediaplayer, pos, length);
    return 1;
}

