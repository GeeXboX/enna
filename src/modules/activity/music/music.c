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
#include "browser_obj.h"
#include "logs.h"
#include "mediaplayer_obj.h"
#include "volumes.h"
#include "module.h"

#include "music_lyrics.h"

#define ENNA_MODULE_NAME "music"

static void _create_menu();
static void _create_gui();
static void _create_mediaplayer_gui();

static void _browser_selected_cb(void *data,
                                 Evas_Object *obj, void *event_info);
static void _browser_delay_hilight_cb(void *data,
                                      Evas_Object *obj, void *event_info);
static void _class_event(enna_input event);
static void _class_event_mediaplayer_view(enna_input event);
static void panel_lyrics_display(int show);

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
    BROWSER_VIEW,
    MEDIAPLAYER_VIEW
};

struct _Enna_Module_Music
{
    Evas_Object *o_layout;
    Evas_Object *o_pager;
    Evas_Object *o_browser;
    Evas_Object *o_mediaplayer;
    Evas_Object *o_panel_lyrics;
    Enna_Module *em;
    MUSIC_STATE state;
    Enna_Playlist *enna_playlist;
    int lyrics_displayed;
};

static Enna_Module_Music *mod;

static void
update_songs_counter(Eina_List *list)
{
    Enna_File *f;
    Eina_List *l;
    int children = 0;
    char label[128] = { 0 };
    Evas_Object *o_edje;

    DBG(__FUNCTION__);
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
    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_part_text_set(o_edje, "songs.counter.label", label);
}

static void
_class_event_browser_view(enna_input event)
{
    DBG(__FUNCTION__);
    if (event == ENNA_INPUT_BACK)
        update_songs_counter(NULL);

    /* whichever action is, ensure lyrics panel gets hidden */
    if (event != ENNA_INPUT_INFO)
        panel_lyrics_display(0);

    switch (event)
    {
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_LEFT:
        if (enna_mediaplayer_show_get(mod->o_mediaplayer))
            mod->state = MEDIAPLAYER_VIEW;
        break;
    case ENNA_INPUT_INFO:
        panel_lyrics_display(!mod->lyrics_displayed);
        break;
    default:
        enna_browser_obj_input_feed(mod->o_browser, event);
    }
}

static void
_class_event_mediaplayer_view(enna_input event)
{
    DBG(__FUNCTION__);
    /* whichever action is, ensure lyrics panel gets hidden */
    if (event != ENNA_INPUT_INFO)
        panel_lyrics_display(0);

    if (!enna_mediaplayer_show_get(mod->o_mediaplayer))
    {
        mod->state = BROWSER_VIEW;
        return;
    }

    if (enna_mediaplayer_obj_input_feed(mod->o_mediaplayer, event)
        == ENNA_EVENT_BLOCK)
        return;

    switch (event)
    {
    case ENNA_INPUT_LEFT:
    case ENNA_INPUT_RIGHT:
    case ENNA_INPUT_UP:
    case ENNA_INPUT_DOWN:
        mod->state = BROWSER_VIEW;
        break;
    case ENNA_INPUT_INFO:
        panel_lyrics_display(!mod->lyrics_displayed);
        break;
    default:
        break;
    }

}

static void
panel_lyrics_display(int show)
{
    Evas_Object *o_edje;

    DBG(__FUNCTION__);
    o_edje = elm_layout_edje_get(mod->o_layout);

    if (show)
    {
        Enna_Metadata *m;



        m = enna_mediaplayer_metadata_get(mod->enna_playlist);
        enna_panel_lyrics_set_text(mod->o_panel_lyrics, m);
        edje_object_signal_emit(o_edje, "lyrics,show", "enna");
	elm_pager_content_promote(mod->o_pager, mod->o_panel_lyrics);
        mod->lyrics_displayed = 1;
    }
    else
    {
        elm_pager_content_promote(mod->o_pager, mod->o_mediaplayer);
        mod->lyrics_displayed = 0;
    }
}

static void
_mediaplayer_info_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    DBG(__FUNCTION__);
    panel_lyrics_display(!mod->lyrics_displayed);
}

static void
_browser_root_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_content_hide();
}
static void
_browser_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    int i = 0;
    Enna_File *file = event_info;
    Eina_List *files = enna_browser_obj_files_get(mod->o_browser);

    DBG(__FUNCTION__);
    if (!file)
        return;

    if (file->is_directory || file->is_menu)
    {
        enna_log(ENNA_MSG_EVENT,
                 ENNA_MODULE_NAME, "Directory Selected %s", file->uri);
        update_songs_counter(files);
    }
    else
    {
        Enna_File *f;
        Eina_List *l;
        enna_log(ENNA_MSG_EVENT,
                 ENNA_MODULE_NAME , "File Selected %s", file->uri);
        enna_mediaplayer_playlist_stop_clear(mod->enna_playlist);
        /* File selected, create mediaplayer */
         EINA_LIST_FOREACH(files, l, f)
         {
             if (!f->is_directory)
             {
                 enna_mediaplayer_file_append(mod->enna_playlist, f);
                 if (!strcmp(f->uri, file->uri))
                 {
                     enna_mediaplayer_select_nth(mod->enna_playlist,i);
                     enna_mediaplayer_obj_event_catch(mod->o_mediaplayer);
                     enna_mediaplayer_play(mod->enna_playlist);
                 }
                 i++;
             }
         }
    }
}

static void
_ondemand_cb_refresh(Enna_File *file, Enna_Metadata_OnDemand ev)
{
    char *uri;
    Enna_Metadata *m;
    DBG(__FUNCTION__);
    if (!file || !file->uri || !mod->enna_playlist)
        return;

    if (file->is_directory || file->is_menu)
        return;

    /*
     * With the music activity, there is nothing to refresh if no file is
     * set to the mediaplayer.
     */
    uri = enna_mediaplayer_get_current_uri();
    if (!uri)
        return;

    if (strcmp(file->uri, uri))
        return;

    m = enna_metadata_meta_new(file->mrl);
    if (!m)
        return;

    switch (ev)
    {
    case ENNA_METADATA_OD_PARSED:
    case ENNA_METADATA_OD_GRABBED:
        enna_panel_lyrics_set_text(mod->o_panel_lyrics, m);
    case ENNA_METADATA_OD_ENDED:
        /*
         * The texts and the cover are handled in mediaplayer_obj contrary
         * to the video activity where all metadata are handled separately.
         */
        enna_mediaplayer_obj_metadata_refresh(mod->o_mediaplayer);
        break;

    default:
        break;
    }

    enna_metadata_meta_free(m);
    free(uri);
}

static void
_browser_delay_hilight_cb(void *data, Evas_Object *obj, void *event_info)
{
    Enna_File *file = event_info;
    DBG(__FUNCTION__);
    if (!file)
        return;


    if (!file->is_directory && !file->is_menu && file->mrl)
        /* ask for on-demand scan for local files */
        if (!strncmp(file->mrl, "file://", 7))
            enna_metadata_ondemand(file, _ondemand_cb_refresh);
}

static void
_create_mediaplayer_gui()
{
    Evas_Object *o;
    Evas_Object *o_edje;

    DBG(__FUNCTION__);
    o_edje = elm_layout_edje_get(mod->o_layout);
    o = enna_mediaplayer_obj_add(enna->evas, mod->enna_playlist);
    evas_object_show(o);
    mod->o_mediaplayer = o;
    elm_pager_content_push(mod->o_pager, mod->o_mediaplayer);
    evas_object_smart_callback_add(mod->o_mediaplayer, "info,clicked",
                                   _mediaplayer_info_clicked_cb, NULL);
    edje_object_signal_emit(o_edje, "mediaplayer,show", "enna");
    edje_object_signal_emit(o_edje, "content,hide", "enna");
}

static void
_create_menu()
{
    Evas_Object *o_edje;

    DBG(__FUNCTION__);
    /* Set default state */
    mod->state = BROWSER_VIEW;

    /* Create Pager */
    ENNA_OBJECT_DEL(mod->o_pager);
    mod->o_pager = elm_pager_add(mod->o_layout);
    evas_object_size_hint_weight_set(mod->o_pager, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(mod->o_pager);

    elm_layout_content_set(mod->o_layout, "mediaplayer.swallow", mod->o_pager);
    elm_object_style_set(mod->o_pager, "flip");
    
    /* Create List */
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    
    mod->o_browser = enna_browser_obj_add(mod->o_layout);
    enna_browser_obj_view_type_set(mod->o_browser, ENNA_BROWSER_VIEW_LIST);
    enna_browser_obj_root_set(mod->o_browser, "/music");

    evas_object_smart_callback_add(mod->o_browser, "selected",
                                   _browser_selected_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "delay,hilight",
                                   _browser_delay_hilight_cb, NULL);
    evas_object_smart_callback_add(mod->o_browser, "root",
                                   _browser_root_cb, NULL);
   elm_layout_content_set(mod->o_layout, "browser.swallow", mod->o_browser);

    /* Create Lyrics */
    mod->o_panel_lyrics = enna_panel_lyrics_add (enna->evas);
    elm_pager_content_push(mod->o_pager, mod->o_panel_lyrics);

    elm_pager_content_promote(mod->o_pager, mod->o_browser);

}

static void
_create_gui()
{
    DBG(__FUNCTION__);
    /* Set default state */
    mod->state = BROWSER_VIEW;

    /* Create main edje object */
    mod->o_layout = elm_layout_add(enna->layout);
    elm_layout_file_set(mod->o_layout, enna_config_theme_get(), "activity/music");

    _create_menu();
    _create_mediaplayer_gui();


}

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static void
_class_init(void)
{
    DBG(__FUNCTION__);
    _create_gui();
    enna_content_append("music", mod->o_layout);
}

static void
_class_show(void)
{
    Evas_Object *o_edje;

    DBG(__FUNCTION__);
    o_edje = elm_layout_edje_get(mod->o_layout);

    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(o_edje, "module,show", "enna");
    switch (mod->state)
    {
    case BROWSER_VIEW:
        edje_object_signal_emit(o_edje, "content,show", "enna");
        edje_object_signal_emit(o_edje, "mediaplayer,hide", "enna");
        break;
    default:
        enna_log(ENNA_MSG_ERROR,
                 ENNA_MODULE_NAME, "Error State Unknown in music module");
    }
}

static void
_class_hide(void)
{
    Evas_Object *o_edje;

    DBG(__FUNCTION__);
    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_signal_emit(o_edje, "module,hide", "enna");
}

static void
_class_event(enna_input event)
{
    DBG(__FUNCTION__);
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Key pressed music : %d", event);

    switch (mod->state)
    {
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
    ENNA_CAPS_MUSIC,
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    }
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_music
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    DBG(__FUNCTION__);
    if (!em)
        return;

    mod     = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;

    enna_activity_register(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
}

static void
module_shutdown(Enna_Module *em)
{
    DBG(__FUNCTION__);
    enna_activity_unregister(&class);
    ENNA_OBJECT_DEL(mod->o_layout);

    evas_object_smart_callback_del(mod->o_browser,
                                   "selected", _browser_selected_cb);
    evas_object_smart_callback_del(mod->o_browser,
                                   "delay,hilight", _browser_delay_hilight_cb);
    evas_object_smart_callback_del(mod->o_browser,
                                   "root", _browser_root_cb);
    ENNA_OBJECT_DEL(mod->o_pager);
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_panel_lyrics);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_music",
    N_("Music"),
    "icon/music",
    N_("Listen to your music"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
