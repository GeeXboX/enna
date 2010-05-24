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
#include <Ecore_Input.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "activity.h"
#include "content.h"
#include "image.h"
#include "mainmenu.h"
#include "logs.h"
#include "vfs.h"
#include "view_list.h"
#include "browser_obj.h"
#include "mediaplayer.h"
#include "volumes.h"
#include "buffer.h"
#include "metadata.h"
#include "utils.h"
#include "mediaplayer_obj.h"
#include "xdg.h"

#include "video.h"
#include "video_flags.h"
#include "video_infos.h"
#include "video_resume.h"
#include "video_picture.h"

#define ENNA_MODULE_NAME "video"

#define TIMER_DELAY 10.0

static void browser_cb_root(void *data, Evas_Object *obj, void *event_info);
static void browser_cb_select(void *data, Evas_Object *obj, void *event_info);
static void browser_cb_delay_hilight(void *data,
                                     Evas_Object *obj, void *event_info);
static void _create_menu(void);
static void _return_to_video_info_gui();

static int _eos_cb(void *data, int type, void *event);
static void video_infos_del(void);

typedef struct _Enna_Module_Video Enna_Module_Video;
typedef enum _VIDEO_STATE VIDEO_STATE;

enum _VIDEO_STATE
{
    BROWSER_VIEW,
    VIDEOPLAYER_VIEW
};

struct _Enna_Module_Video
{
    Evas_Object *o_layout;
    Evas_Object *o_browser;
    Evas_Object *o_backdrop;
    Evas_Object *o_snapshot;
    Evas_Object *o_panel_infos;
    Evas_Object *o_resume;
    Evas_Object *o_video_flags;
    Evas_Object *o_mediaplayer;
    Evas_Object *o_mediacontrols;
    Enna_Module *em;
    VIDEO_STATE state;
    Ecore_Event_Handler *eos_event_handler;
    Enna_Playlist *enna_playlist;
    char *o_current_uri;
    int infos_displayed;
    int resume_displayed;
    int controls_displayed;
    char *uri_hilighted;
    Enna_Volumes_Listener *vl;
    Ecore_Timer *controls_timer;
    Ecore_Event_Handler *mouse_button_event_handler;
    Ecore_Event_Handler *mouse_move_event_handler;
};

static Enna_Module_Video *mod;

static void
update_movies_counter(Eina_List *list)
{
    Enna_Vfs_File *f;
    Eina_List *l;
    int children = 0;
    char label[128] = { 0 };
    Evas_Object *o_edje;

    if (!list)
        goto end;

    EINA_LIST_FOREACH(list, l, f)
    {
        if (!f->is_directory && !f->is_menu)
            children++;
    }
    if (children)
        snprintf(label, sizeof(label), _("%d Movies"), children);
end:
    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_part_text_set(o_edje, "movies.counter.label", label);
}

static int
_controls_timer_cb(void *data)
{
    media_controls_display(0);
    mod->controls_timer = NULL;
    return 0;
}

static void
video_resize(void)
{

    Evas_Coord w, h, x, y;
    Evas_Coord h2 = 0;
#ifndef BUILD_BACKEND_EMOTION
    if (mod->controls_displayed)
        evas_object_geometry_get(mod->o_mediacontrols, NULL, NULL, NULL, &h2);
#endif
    evas_object_geometry_get(mod->o_mediaplayer, &x, &y, &w, &h);
    enna_mediaplayer_video_resize(x, y, w, h - h2);

}

void
media_controls_display(int show)
{
    if (show)
    {
        ENNA_TIMER_DEL(mod->controls_timer);
        mod->controls_timer =
            ecore_timer_add(TIMER_DELAY, _controls_timer_cb, NULL);
    }

    if (show == mod->controls_displayed)
        return;

    mod->controls_displayed = show;
    show ? evas_object_show(mod->o_mediacontrols) : evas_object_hide(mod->o_mediacontrols);

    video_resize();
}

static void
_seek_video(int value)
{
    enna_mediaplayer_seek_relative(value);
    enna_mediaplayer_position_update(mod->o_mediacontrols);
}

static void
videoplayer_view_event_no_display (enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_OK:
        media_controls_display(!mod->controls_displayed);
        break;
    case ENNA_INPUT_PLAY:
        enna_mediaplayer_play(mod->enna_playlist);
        break;
    case ENNA_INPUT_RIGHT:
        _seek_video(+10); /* +10s */
        break;
    case ENNA_INPUT_LEFT:
        _seek_video(-10); /* -10s */
        break;
    case ENNA_INPUT_UP:
        _seek_video(+60); /* +60s */
        break;
    case ENNA_INPUT_DOWN:
        _seek_video(-60); /* -60s */
        break;
    case ENNA_INPUT_FORWARD:
        _seek_video(+600); /* +10min */
        break;
    case ENNA_INPUT_REWIND:
        _seek_video(-600); /* -10min */
        break;
    case ENNA_INPUT_MUTE:
        enna_mediaplayer_mute();
        break;
    case ENNA_INPUT_AUDIO_PREV:
        enna_mediaplayer_audio_previous();
        break;
    case ENNA_INPUT_AUDIO_NEXT:
        enna_mediaplayer_audio_next();
        break;
    case ENNA_INPUT_AUDIO_DELAY_PLUS:
        enna_mediaplayer_audio_increase_delay();
        break;
    case ENNA_INPUT_AUDIO_DELAY_MINUS:
        enna_mediaplayer_audio_decrease_delay();
        break;
    case ENNA_INPUT_SUBTITLES:
        enna_mediaplayer_subtitle_set_visibility();
        break;
    case ENNA_INPUT_SUBS_PREV:
        enna_mediaplayer_subtitle_previous();
        break;
    case ENNA_INPUT_SUBS_NEXT:
        enna_mediaplayer_subtitle_next();
        break;
    case ENNA_INPUT_SUBS_ALIGN:
        enna_mediaplayer_subtitle_set_alignment();
        break;
    case ENNA_INPUT_SUBS_POS_PLUS:
        enna_mediaplayer_subtitle_increase_position();
        break;
    case ENNA_INPUT_SUBS_POS_MINUS:
        enna_mediaplayer_subtitle_decrease_position();
        break;
    case ENNA_INPUT_SUBS_SCALE_PLUS:
        enna_mediaplayer_subtitle_increase_scale();
        break;
    case ENNA_INPUT_SUBS_SCALE_MINUS:
        enna_mediaplayer_subtitle_decrease_scale();
        break;
    case ENNA_INPUT_SUBS_DELAY_PLUS:
        enna_mediaplayer_subtitle_increase_delay();
        break;
    case ENNA_INPUT_SUBS_DELAY_MINUS:
        enna_mediaplayer_subtitle_decrease_delay();
        break;
    case ENNA_INPUT_FRAMEDROP:
        enna_mediaplayer_set_framedrop();
        break;
    default:
        break;
    }
}

static void
videoplayer_view_event(enna_input event)
{
    switch (event)
    {
    case ENNA_INPUT_QUIT:
    case ENNA_INPUT_BACK:
    case ENNA_INPUT_STOP:
        _return_to_video_info_gui();
        break;
    default:
        videoplayer_view_event_no_display(event);
        if (mod->controls_displayed)
            media_controls_display(1);
        break;
    }
}

static void
popup_resume_display(int show)
{
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    if (show)
    {
        edje_object_signal_emit(o_edje, "resume,show", "enna");
        mod->resume_displayed = 1;
    }
    else
    {
        edje_object_signal_emit(o_edje, "resume,hide", "enna");
        mod->resume_displayed = 0;
    }
}

static void
_return_to_video_info_gui()
{
    Enna_Metadata *m;
    double pos;

    media_controls_display(0);
    ENNA_TIMER_DEL(mod->controls_timer);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    ENNA_OBJECT_DEL(mod->o_mediacontrols);
    popup_resume_display (0);
    m = enna_mediaplayer_metadata_get(mod->enna_playlist);
    pos = enna_mediaplayer_position_get();
    enna_metadata_set_position(m, pos);
    enna_mediaplayer_stop();
    mod->state = BROWSER_VIEW;
}

static int
_eos_cb(void *data, int type, void *event)
{
    _return_to_video_info_gui();
    return 1;
}

/****************************************************************************/
/*                               Backdrop                                   */
/****************************************************************************/

static void
backdrop_show(Enna_Metadata *m)
{
    char *file = NULL;
    int from_vfs = 1;
    char *backdrop;

    backdrop = enna_metadata_meta_get(m, "fanart", 1);
    if (backdrop)
    {
        char dst[1024] = { 0 };

        if (*backdrop == '/')
            snprintf(dst, sizeof (dst), "%s", backdrop);
        else
            snprintf(dst, sizeof (dst), "%s/fanarts/%s",
                     enna_data_home_get(), backdrop);
        file = strdup(dst);

        enna_video_picture_set(mod->o_backdrop, file, from_vfs);
        evas_object_show(mod->o_backdrop);
        elm_layout_content_set(mod->o_layout,
                               "backdrop.swallow", mod->o_backdrop);
        ENNA_FREE(backdrop);
        ENNA_FREE(file);
    }
    else
        enna_video_picture_unset(mod->o_backdrop);
}

/****************************************************************************/
/*                               Snapshot                                   */
/****************************************************************************/

static void
snapshot_show(Enna_Metadata *m, int dir)
{
    char *file = NULL;
    int from_vfs = 1;
    char *snapshot;

    snapshot = enna_metadata_meta_get(m, "fanart", 1);
    if (snapshot)
    {
        char dst[1024] = { 0 };

        if (*snapshot == '/')
            snprintf(dst, sizeof(dst), "%s", snapshot);
        else
            snprintf(dst, sizeof(dst), "%s/fanarts/%s",
                     enna_data_home_get(), snapshot);
        file = strdup(dst);
    }
    else
    {
        file = strdup(dir ? "cover/video/dir" : "cover/video/file");
        from_vfs = 0;
    }

    enna_video_picture_set(mod->o_snapshot, file, from_vfs);
    evas_object_show(mod->o_snapshot);
    elm_layout_content_set(mod->o_layout,
                           "snapshot.swallow", mod->o_snapshot);

    ENNA_FREE(snapshot);
    ENNA_FREE(file);
}

static void
panel_infos_display(int show)
{
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    if (show)
    {
        edje_object_signal_emit(o_edje, "infos,show", "enna");
        mod->infos_displayed = 1;
    }
    else
    {
        edje_object_signal_emit(o_edje, "infos,hide", "enna");
        mod->infos_displayed = 0;
    }
}

/****************************************************************************/
/*                                Browser                                   */
/****************************************************************************/

static void
browser_view_event(enna_input event)
{
    /* handle resume popup, if any */
    if (mod->resume_displayed)
    {
        if (event == ENNA_INPUT_BACK)
            popup_resume_display(0);
        else
            video_resume_input_feed(mod->o_resume, event);
        return;
    }

    if (event == ENNA_INPUT_INFO)
    {
        panel_infos_display(!mod->infos_displayed);
        return;
    }

    if (event == ENNA_INPUT_BACK)
    {
        video_infos_del();
        update_movies_counter(NULL);
    }
    enna_browser_obj_input_feed(mod->o_browser, event);
}

static void
browser_cb_root(void *data, Evas_Object *obj, void *event_info)
{
    enna_content_hide();
    enna_mainmenu_show();
}

static void
_mediaplayer_resize_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    video_resize();
}

static void
_mediaplayer_mouse_move_cb(void *data,
                           Evas *e, Evas_Object *obj, void *event_info)
{
    media_controls_display(1);
}

static void
_mediaplayer_mouse_up_cb(void *data,
                         Evas *e, Evas_Object *obj, void *event_info)
{
    media_controls_display(1);
}

static int
_mediaplayer_mouse_move_libplayer_cb(void *data, int type, void *event)
{
    Ecore_Event_Mouse_Move *e = event;
    Evas_Coord y;

    evas_object_geometry_get(mod->o_mediacontrols, NULL, &y, NULL, NULL);

    if ((e->window != enna->ee_winid) && (e->y >= y))
    {
        media_controls_display(1);
        return 1;
    }
    return 0;
}

static int
_mediaplayer_mouse_down_libplayer_cb(void *data, int type, void *event)
{
    Ecore_Event_Mouse_Button *e = event;
    Evas_Coord y;

    evas_object_geometry_get(mod->o_mediacontrols, NULL, &y, NULL, NULL);

    if ((e->window != enna->ee_winid) && (e->y >= y))
    {
        media_controls_display(1);
        return 1;
    }
    return 0;
}

void
movie_start_playback(int resume)
{
    const Evas_Object *ed;
    Evas_Object *o_edje;
    Evas_Coord x, y, w, h;

    panel_infos_display(0);
    mod->state = VIDEOPLAYER_VIEW;

    ENNA_EVENT_HANDLER_DEL(mod->mouse_button_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->mouse_move_event_handler);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    mod->o_mediaplayer = evas_object_rectangle_add(enna->evas);
    evas_object_color_set(mod->o_mediaplayer, 0, 0, 0, 255);
    elm_layout_content_set(mod->o_layout,
                           "fullscreen.swallow", mod->o_mediaplayer);
    evas_object_event_callback_add(mod->o_mediaplayer, EVAS_CALLBACK_RESIZE,
                                   _mediaplayer_resize_cb, NULL);

    ENNA_OBJECT_DEL(mod->o_mediacontrols);

    mod->o_mediacontrols =
        enna_mediaplayer_obj_add(enna->evas, mod->enna_playlist);
    
    o_edje = elm_layout_edje_get(mod->o_layout);
    ed = edje_object_part_object_get(o_edje, "controls.swallow");
    evas_object_geometry_get(ed, &x, &y, &w, &h);
    evas_object_move(mod->o_mediacontrols, x, y);
    evas_object_resize(mod->o_mediacontrols, w, h);
    evas_object_show(mod->o_mediacontrols);

    /*edje_object_part_swallow(mod->o_edje,
                             "controls.swallow", mod->o_mediacontrols);*/
    enna_mediaplayer_obj_layout_set(mod->o_mediacontrols, "layout,video");

    mod->mouse_button_event_handler =
        ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                _mediaplayer_mouse_down_libplayer_cb, NULL);
    mod->mouse_move_event_handler =
        ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                _mediaplayer_mouse_move_libplayer_cb, NULL);
#ifdef BUILD_BACKEND_LIBPLAYER
    evas_object_event_callback_add(mod->o_mediacontrols,
                                   EVAS_CALLBACK_MOUSE_MOVE,
                                   _mediaplayer_mouse_move_cb, NULL);
    evas_object_event_callback_add(mod->o_mediacontrols,
                                   EVAS_CALLBACK_MOUSE_UP,
                                   _mediaplayer_mouse_up_cb, NULL);
    evas_object_event_callback_add(mod->o_mediacontrols,
                                   EVAS_CALLBACK_RESIZE,
                                   _mediaplayer_resize_cb, NULL);
#else
    evas_object_event_callback_add(enna_mediaplayer_obj_get(),
                                   EVAS_CALLBACK_MOUSE_MOVE,
                                   _mediaplayer_mouse_move_cb, NULL);
    evas_object_event_callback_add(enna_mediaplayer_obj_get(),
                                   EVAS_CALLBACK_MOUSE_UP,
                                   _mediaplayer_mouse_up_cb, NULL);
    evas_object_event_callback_add(enna_mediaplayer_obj_get(),
                                   EVAS_CALLBACK_RESIZE,
                                   _mediaplayer_resize_cb, NULL);
#endif
    enna_mediaplayer_stop();
    enna_mediaplayer_obj_event_catch(mod->o_mediacontrols);
    enna_mediaplayer_play(mod->enna_playlist);
#if 0
    if (resume)
    {
        Enna_Metadata *m;
        m = enna_mediaplayer_metadata_get(mod->enna_playlist);
        enna_mediaplayer_position_set(m->position);
    }
#endif
    popup_resume_display(0);
}

static void
browser_cb_select(void *data, Evas_Object *obj, void *event_info)
{
    int i = 0;
    Enna_Vfs_File *file = event_info;
    Eina_List *l;

    if (!file)
        return;

    if (file->is_directory || file->is_menu)
    {
        enna_log (ENNA_MSG_EVENT,
                  ENNA_MODULE_NAME, "Directory Selected %s", file->uri);
        update_movies_counter(enna_browser_obj_files_get(mod->o_browser));
    }
    else
    {
        Enna_Metadata *m;
        Enna_File *f;
        enna_log(ENNA_MSG_EVENT,
                 ENNA_MODULE_NAME, "File Selected %s", file->uri);
        enna_mediaplayer_playlist_clear(mod->enna_playlist);

        /* File selected, create mediaplayer */
        EINA_LIST_FOREACH(enna_browser_obj_files_get(mod->o_browser), l, f)
        {
            if (!f->is_directory && !f->is_menu)
            {
                enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                         "Append : %s %s to playlist", f->label, f->uri);
                enna_mediaplayer_file_append(mod->enna_playlist, f);

                if (!strcmp(f->uri, file->uri))
                {
                    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                              "Select : %s %d in playlist", f->uri, i);
                    enna_mediaplayer_select_nth(mod->enna_playlist,i);

                    if (mod->o_current_uri)
                        free(mod->o_current_uri);
                    mod->o_current_uri = strdup(f->uri);
                }
                i++;
            }
        }

        /* fetch new stream's metadata */
        m = enna_mediaplayer_metadata_get(mod->enna_playlist);
#if 0
        if (m->position)
        {
            /* stream has already been played once, show resume popup */
            popup_resume_display(1);
        }
        else
#endif
            movie_start_playback(0);
    }
}

static void
video_infos_display_title(const Enna_Vfs_File *file, const Enna_Metadata *m)
{
    char *title;
    const char *label;
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    title = enna_metadata_meta_get(m, "title", 1);
    label = title ? title : file->label;
    edje_object_part_text_set(o_edje, "title.label", label);

    free(title);
}

static void
video_infos_display_genre(const Enna_Vfs_File *file, const Enna_Metadata *m)
{
    char *categories;
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    categories = enna_metadata_meta_get(m, "category", 5);
    edje_object_part_text_set(o_edje, "genre.label",
                              categories ? categories : "");

    free(categories);
}

static void
video_infos_display_length(const Enna_Vfs_File *file, const Enna_Metadata *m)
{
    char *length;
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    length = enna_metadata_meta_duration_get(m);
    edje_object_part_text_set(o_edje, "length.label",
                              length ? length : "");

    free(length);
}

static void
video_infos_display_synopsis(const Enna_Vfs_File *file, const Enna_Metadata *m)
{
    char *synopsis;
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    synopsis = enna_metadata_meta_get(m, "synopsis", 1);
    edje_object_part_text_set(o_edje, "synopsis.textblock",
                              synopsis ? synopsis : "");
    edje_object_signal_emit(o_edje, synopsis ?
                            "separator,show" : "separator,hide", "enna");

    free(synopsis);
}

static void
video_infos_display(const Enna_Vfs_File *file)
{
    Enna_Metadata *m;

    if (!file)
        return;

    /* If m is NULL, the panels will be cleaned. */
    m = enna_metadata_meta_new(file->mrl);

    video_infos_display_title(file, m);
    video_infos_display_genre(file, m);
    video_infos_display_length(file, m);
    video_infos_display_synopsis(file, m);

    backdrop_show(m);
    snapshot_show(m, file->is_directory);

    enna_video_flags_update(mod->o_video_flags, m);

    enna_panel_infos_set_cover(mod->o_panel_infos, m);
    enna_panel_infos_set_text(mod->o_panel_infos, m);
    enna_panel_infos_set_rating(mod->o_panel_infos, m);

    enna_metadata_meta_free(m);
}

static void
video_infos_del (void)
{
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);
    edje_object_part_text_set(o_edje, "title.label", "");
    edje_object_part_text_set(o_edje, "genre.label", "");
    edje_object_part_text_set(o_edje, "length.label", "");
    edje_object_part_text_set(o_edje, "synopsis.textblock", "");
    panel_infos_display(0);
    popup_resume_display(0);
    enna_video_picture_set(mod->o_backdrop, NULL, 0);
    enna_video_picture_set(mod->o_snapshot, NULL, 0);
    enna_video_flags_update(mod->o_video_flags, NULL);
    edje_object_signal_emit(o_edje, "separator,hide", "enna");
}

static void
_ondemand_cb_refresh(const Enna_Vfs_File *file, Enna_Metadata_OnDemand ev)
{
    Enna_Metadata *m;

    if (!file || !file->mrl || !mod->uri_hilighted)
        return;

    if (file->is_directory || file->is_menu)
        return;

    if (strcmp(file->mrl, mod->uri_hilighted))
        return;

    m = enna_metadata_meta_new(file->mrl);
    if (!m)
        return;

    switch (ev)
    {
    case ENNA_METADATA_OD_PARSED:
    case ENNA_METADATA_OD_GRABBED:
        video_infos_display_title(file, m);
        video_infos_display_genre(file, m);
        video_infos_display_length(file, m);
        video_infos_display_synopsis(file, m);
        enna_video_flags_update(mod->o_video_flags, m);
        enna_panel_infos_set_text(mod->o_panel_infos, m);
        enna_panel_infos_set_rating(mod->o_panel_infos, m);
        if (ev == ENNA_METADATA_OD_PARSED)
            break;
    case ENNA_METADATA_OD_ENDED:
        backdrop_show(m);
        snapshot_show(m, file->is_directory);
        enna_panel_infos_set_cover(mod->o_panel_infos, m);
        break;

    default:
        break;
    }

    enna_metadata_meta_free(m);
}

static void
browser_cb_delay_hilight(void *data, Evas_Object *obj, void *event_info)
{
    Enna_File *file = event_info;

    if (!file || !file->mrl)
        return;

    if (!file->is_directory && !file->is_menu)
    {
        video_infos_display(file);

        /* ask for on-demand scan for local files */
        if (!strncmp(file->mrl, "file://", 7))
            enna_metadata_ondemand(file, _ondemand_cb_refresh);
    }


    ENNA_FREE(mod->uri_hilighted);
    mod->uri_hilighted = strdup(file->mrl);
}

static void
_create_menu()
{

    mod->o_browser = enna_browser_obj_add(mod->o_layout);

    enna_browser_obj_view_type_set(mod->o_browser, ENNA_BROWSER_VIEW_LIST);
    evas_object_smart_callback_add(mod->o_browser,
                                   "root", browser_cb_root, NULL);
    evas_object_smart_callback_add(mod->o_browser,
                                   "selected", browser_cb_select, NULL);
    evas_object_smart_callback_add (mod->o_browser, "delay,hilight",
                                    browser_cb_delay_hilight, NULL);

    elm_layout_content_set(mod->o_layout,
                           "browser.swallow", mod->o_browser);
    enna_browser_obj_root_set(mod->o_browser, "/video");

    ENNA_OBJECT_DEL(mod->o_panel_infos);
    mod->o_panel_infos = enna_panel_infos_add(enna->evas);
    elm_layout_content_set(mod->o_layout,
                           "infos.panel.swallow", mod->o_panel_infos);

    ENNA_OBJECT_DEL(mod->o_resume);
    mod->o_resume = video_resume_add(enna->evas);
    elm_layout_content_set(mod->o_layout,
                           "resume.swallow", mod->o_resume);

    ENNA_OBJECT_DEL(mod->o_video_flags);
    mod->o_video_flags = enna_video_flags_add(enna->evas);
    elm_layout_content_set(mod->o_layout,
                           "infos.flags.swallow", mod->o_video_flags);

    mod->state = BROWSER_VIEW;
}

/****************************************************************************/
/*                                  GUI                                     */
/****************************************************************************/


static void
_create_gui(void)
{
    mod->state = BROWSER_VIEW;
    mod->o_layout = elm_layout_add(enna->layout);
    elm_layout_file_set(mod->o_layout, enna_config_theme_get(), "activity/video");
    _create_menu();
}

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static void
_class_init(void)
{
    _create_gui();
    enna_content_append(ENNA_MODULE_NAME, mod->o_layout);
}

static void
_class_show(void)
{
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);

    enna_content_select(ENNA_MODULE_NAME);
    edje_object_signal_emit(o_edje, "module,show", "enna");

    switch (mod->state)
    {
    case BROWSER_VIEW:
        edje_object_signal_emit(o_edje, "content,show", "enna");
        break;

    case VIDEOPLAYER_VIEW:
        break;
    default:
        enna_log(ENNA_MSG_ERROR,
                 ENNA_MODULE_NAME,  "State Unknown in video module");
    }
}

static void
_class_hide(void)
{
    Evas_Object *o_edje;

    o_edje = elm_layout_edje_get(mod->o_layout);

    _return_to_video_info_gui();
    edje_object_signal_emit(o_edje, "module,hide", "enna");
}

static void
_class_event(enna_input event)
{
    switch (mod->state)
    {
    case BROWSER_VIEW:
        browser_view_event(event);
        break;
    case VIDEOPLAYER_VIEW:
        videoplayer_view_event(event);
        break;
    default:
        break;
    }

}

static Enna_Class_Activity class =
{
    ENNA_MODULE_NAME,
    1,
    N_("Video"),
    NULL,
    "icon/video",
    "background/video",
    ENNA_CAPS_VIDEO,
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event
    }
};

static void
em_init(Enna_Module *em)
{
    mod = calloc(1, sizeof(Enna_Module_Video));
    mod->em = em;
    em->mod = mod;

    mod->infos_displayed = 0;
    mod->resume_displayed = 0;
    mod->controls_displayed = 0;
    mod->o_backdrop = enna_video_picture_add(enna->evas);
    mod->o_snapshot = enna_video_picture_add(enna->evas);
    mod->eos_event_handler =
        ecore_event_handler_add(ENNA_EVENT_MEDIAPLAYER_EOS, _eos_cb, NULL);
    enna_activity_register(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
}

static void
em_shutdown(Enna_Module *em)
{
    enna_activity_unregister(&class);
    ENNA_EVENT_HANDLER_DEL(mod->eos_event_handler);
    ENNA_OBJECT_DEL(mod->o_layout);
    evas_object_smart_callback_del(mod->o_browser, "root", browser_cb_root);
    evas_object_smart_callback_del(mod->o_browser,
                                   "selected", browser_cb_select);
    evas_object_smart_callback_del(mod->o_browser,
                                   "delay,hilight", browser_cb_delay_hilight);
    ENNA_EVENT_HANDLER_DEL(mod->mouse_button_event_handler);
    ENNA_EVENT_HANDLER_DEL(mod->mouse_move_event_handler);
    ENNA_OBJECT_DEL(mod->o_browser);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    ENNA_OBJECT_DEL(mod->o_mediacontrols);
    ENNA_OBJECT_DEL(mod->o_backdrop);
    ENNA_OBJECT_DEL(mod->o_snapshot);
    ENNA_OBJECT_DEL(mod->o_panel_infos);
    ENNA_OBJECT_DEL(mod->o_resume);
    ENNA_OBJECT_DEL(mod->o_video_flags);
    ENNA_FREE(mod->o_current_uri);
    ENNA_FREE(mod->uri_hilighted);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
}

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_video
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    em_init(em);
}

static void
module_shutdown(Enna_Module *em)
{
    em_shutdown(em);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_video",
    N_("Video"),
    "icon/video",
    N_("Play your videos"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

