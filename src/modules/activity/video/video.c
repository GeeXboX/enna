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

/*
 * FIXME : Remove unused object and fix navigation : it's actually not possible
 * to return from video playback !
 * Fix state machine
 * Enable Position set
 * EOS is not used !!!!
 */

#undef LOCATION

#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "activity.h"
#include "content.h"
#include "image.h"
#include "mainmenu.h"
#include "logs.h"
#include "vfs.h"
#include "list.h"
#include "browser.h"
#ifdef LOCATION
#include "location.h"
#endif
#include "mediaplayer.h"
#include "event_key.h"
#include "backdrop.h"
#include "panel_infos.h"
#include "volumes.h"
#include "buffer.h"
#include "metadata.h"

#define ENNA_MODULE_NAME "video"

static void browser_cb_root (void *data, Evas_Object *obj, void *event_info);
static void browser_cb_select (void *data, Evas_Object *obj, void *event_info);
#ifdef LOCATION
static void browser_cb_enter (void *data, Evas_Object *obj, void *event_info);
#endif
static void browser_cb_hilight (void *data, Evas_Object *obj, void *event_info);
static void browse (void *data);

static void _create_menu(void);
static void _return_to_video_info_gui();
static void _seek_video(double value);

static int _eos_cb(void *data, int type, void *event);
static int _show_mediaplayer_cb(void *data);

typedef struct _Enna_Module_Video Enna_Module_Video;
typedef enum _VIDEO_STATE VIDEO_STATE;
typedef struct _Video_Item_Class_Data Video_Item_Class_Data;

struct _Video_Item_Class_Data
{
    const char *icon;
    const char *label;
};

enum _VIDEO_STATE
{
    MENU_VIEW,
    BROWSER_VIEW,
    VIDEOPLAYER_VIEW
};

struct _Enna_Module_Video
{
    Evas *e;
    Evas_Object *o_edje;
    Evas_Object *o_list;
    Evas_Object *o_browser;
#ifdef LOCATION
    Evas_Object *o_location;
#endif
    Evas_Object *o_backdrop;
    Evas_Object *o_panel_infos;
    Evas_Object *o_flag_video;
    Evas_Object *o_flag_audio;
    Evas_Object *o_flag_studio;
    Evas_Object *o_flag_media;
    Evas_Object *o_mediaplayer;
    Enna_Module *em;
    VIDEO_STATE state;
    Ecore_Timer *timer_show_mediaplayer;
    Ecore_Event_Handler *eos_event_handler;
    Ecore_Event_Handler *browser_refresh_handler;
    Enna_Playlist *enna_playlist;
    Elm_Genlist_Item_Class *item_class;
    char *o_current_uri;
    int infos_displayed;
};

static Enna_Module_Video *mod;

static void
menu_view_event (enna_key_t key, void *event_info)
{
    switch (key)
    {
    case ENNA_KEY_LEFT:
    case ENNA_KEY_CANCEL:
        enna_content_hide ();
        enna_mainmenu_show (enna->o_mainmenu);
        break;
    case ENNA_KEY_RIGHT:
    case ENNA_KEY_OK:
    case ENNA_KEY_SPACE:
        browse (enna_list_selected_data_get(mod->o_list));
        break;
    default:
        enna_list_event_key_down(mod->o_list, event_info);
        break;
    }
}

static void
videoplayer_view_event (enna_key_t key)
{
    switch (key)
    {
    case ENNA_KEY_QUIT:
    case ENNA_KEY_CANCEL:
    case ENNA_KEY_OK:
        _return_to_video_info_gui ();
        break;
    case ENNA_KEY_SPACE:
        enna_mediaplayer_play (mod->enna_playlist);
        break;
    case ENNA_KEY_RIGHT:
        _seek_video (+1);
        break;
    case ENNA_KEY_LEFT:
        _seek_video (-1);
        break;
    case ENNA_KEY_UP:
        _seek_video (+5);
        break;
    case ENNA_KEY_DOWN:
        _seek_video (-5);
        break;
    case ENNA_KEY_PLUS:
        enna_mediaplayer_default_increase_volume ();
        break;
    case ENNA_KEY_MINUS:
        enna_mediaplayer_default_decrease_volume ();
        break;
    case ENNA_KEY_M:
        enna_mediaplayer_mute ();
        break;
    case ENNA_KEY_S:
        enna_mediaplayer_subtitle_set_visibility ();
        break;
    case ENNA_KEY_G:
        enna_mediaplayer_subtitle_previous ();
        break;
    case ENNA_KEY_Y:
        enna_mediaplayer_subtitle_next ();
        break;
    case ENNA_KEY_A:
        enna_mediaplayer_subtitle_set_alignment ();
        break;
    case ENNA_KEY_T:
        enna_mediaplayer_subtitle_increase_position ();
        break;
    case ENNA_KEY_R:
        enna_mediaplayer_subtitle_decrease_position ();
        break;
    case ENNA_KEY_J:
        enna_mediaplayer_subtitle_increase_scale ();
        break;
    case ENNA_KEY_I:
        enna_mediaplayer_subtitle_decrease_scale ();
        break;
    case ENNA_KEY_X:
        enna_mediaplayer_subtitle_increase_delay ();
        break;
    case ENNA_KEY_Z:
        enna_mediaplayer_subtitle_decrease_delay ();
        break;
    default:
        break;
    }
}

static int
_show_mediaplayer_cb(void *data)
{
    if (mod->o_mediaplayer)
    {
        mod->state = BROWSER_VIEW;
        edje_object_signal_emit(mod->o_edje, "mediaplayer,show", "enna");
        edje_object_signal_emit(mod->o_edje, "content,hide", "enna");
        ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
        mod->timer_show_mediaplayer = NULL;
    }

    return 0;
}

static void
_seek_video(double value)
{
    int pos = 0;
    double seek = 0.0;

    pos = enna_mediaplayer_position_percent_get();
    seek = ((double) pos + value) / 100.0;
    enna_mediaplayer_seek(seek);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Seek value : %f", seek);
}

static void
_return_to_video_info_gui()
{
    Enna_Metadata *m;
    double pos;

    m = enna_mediaplayer_metadata_get(mod->enna_playlist);
    pos = enna_mediaplayer_position_get();
    enna_metadata_set_position (m, pos);
    enna_mediaplayer_stop();
    mod->state = BROWSER_VIEW;
    edje_object_signal_emit(mod->o_edje, "mediaplayer,show",
    "enna");
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
backdrop_show (Enna_Metadata *m)
{
    char *snap_file = NULL;
    int from_vfs = 1;

    if (!m)
    {
        snap_file = "backdrop/default";
        from_vfs = 0;
    }

    if (m && m->type != ENNA_METADATA_VIDEO)
    {
        snap_file = "backdrop/default";
        from_vfs = 0;
    }

    if (!snap_file)
      snap_file = (m && m->backdrop) ? m->backdrop : m->snapshot;

    if (!snap_file)
	return;

    enna_backdrop_set (mod->o_backdrop, snap_file, from_vfs);
    evas_object_show (mod->o_backdrop);
    edje_object_part_swallow (mod->o_edje,
                              "enna.swallow.backdrop", mod->o_backdrop);
}

/****************************************************************************/
/*                          Information Flags                               */
/****************************************************************************/

static const struct {
    const char *name;
    int min_height;
} flag_video_mapping[] = {
    { "flags/video/1080p",   1080 },
    { "flags/video/720p",     720 },
    { "flags/video/576p",     576 },
    { "flags/video/540p",     540 },
    { "flags/video/480p",     480 },
    { NULL,                     0 }
};

static const struct {
    const char *name;
    int channels;
} flag_audio_mapping[] = {
    { "flags/audio/mono",     1 },
    { "flags/audio/dd20",     2 },
    { "flags/audio/dd51",     5 },
    { "flags/audio/dd71",     7 },
    { NULL,                   0 }
};

static const struct {
    const char *name;
    const char *fullname;
} flag_studio_mapping[] = {
    { NULL,                   0 }
};

static void
infos_flags_set (Enna_Metadata *m)
{
    Evas_Object *v, *a, *s, *md;
    const char *v_str = NULL, *a_str = NULL, *s_str = NULL, *m_str = NULL;

    if (!m)
        goto flags_set;

    if (m && m->type != ENNA_METADATA_VIDEO)
        goto flags_set;

    /* try to guess video flag */
    if (m->video)
    {
        int i;

        for (i = 0; flag_video_mapping[i].name; i++)
            if (m->video->height >= flag_video_mapping[i].min_height)
            {
                v_str = flag_video_mapping[i].name;
                break;
            }

        if (!v_str)
            v_str = "flags/video/sd";

        v = edje_object_add (mod->em->evas);
        edje_object_file_set (v, enna_config_theme_get (), v_str);
    }

    /* try to guess audio flag (naive method atm) */
    if (m->music)
    {
        int i;

        for (i = 0; flag_audio_mapping[i].name; i++)
            if (m->music->channels >= flag_audio_mapping[i].channels)
                a_str = flag_audio_mapping[i].name;

        a = edje_object_add (mod->em->evas);
        edje_object_file_set (a, enna_config_theme_get (), a_str);
    }

    /* try to match an existing studio */
    if (m->studio)
    {
        int i;

        for (i = 0; flag_studio_mapping[i].name; i++)
            if (!strcmp (m->studio, flag_studio_mapping[i].fullname))
            {
                s_str = flag_studio_mapping[i].name;
                break;
            }

        s = edje_object_add (mod->em->evas);
        edje_object_file_set (s, enna_config_theme_get (), s_str);
    }

    /* detect media type: no idea how to that atm, alwasy use default one */
    m_str = "flags/media/divx";
    md = edje_object_add (mod->em->evas);
    edje_object_file_set (md, enna_config_theme_get (), m_str);

 flags_set:
    ENNA_OBJECT_DEL (mod->o_flag_video);
    mod->o_flag_video = v;
    edje_object_part_swallow (mod->o_edje, "infos.flags.video.swallow", v);

    ENNA_OBJECT_DEL (mod->o_flag_audio);
    mod->o_flag_audio = a;
    edje_object_part_swallow (mod->o_edje, "infos.flags.audio.swallow", a);

    ENNA_OBJECT_DEL (mod->o_flag_studio);
    mod->o_flag_studio = s;
    edje_object_part_swallow (mod->o_edje, "infos.flags.studio.swallow", s);

    ENNA_OBJECT_DEL (mod->o_flag_media);
    mod->o_flag_media = md;
    edje_object_part_swallow (mod->o_edje, "infos.flags.media.swallow", md);
}


static void
panel_infos_display (int show)
{
    if (show)
    {
        edje_object_signal_emit (mod->o_edje, "infos,show", "enna");
        mod->infos_displayed = 1;
    }
    else
    {
        edje_object_signal_emit (mod->o_edje, "infos,hide", "enna");
        mod->infos_displayed = 0;
    }
}

/****************************************************************************/
/*                                Browser                                   */
/****************************************************************************/

static void
browser_view_event (enna_key_t key, void *event_info)
{
    if (key == ENNA_KEY_I)
    {
        panel_infos_display (!mod->infos_displayed);
        return;
    }

    if (mod->o_mediaplayer)
    {
        ENNA_TIMER_DEL (mod->timer_show_mediaplayer);
        mod->timer_show_mediaplayer =
            ecore_timer_add (10, _show_mediaplayer_cb, NULL);
    }

    enna_browser_event_feed (mod->o_browser, event_info);
}

static int
browser_cb_refresh (void *data, int type, void *event)
{
    if (mod->state == MENU_VIEW)
    {
	ENNA_OBJECT_DEL(mod->o_list);
	mod->o_list = NULL;
	_create_menu ();
    }

    return 1;
}

static void
browser_cb_root (void *data, Evas_Object *obj, void *event_info)
{
    mod->state = MENU_VIEW;
    evas_object_smart_callback_del (mod->o_browser,
                                    "root", browser_cb_root);
    evas_object_smart_callback_del (mod->o_browser,
                                    "selected", browser_cb_select);
#ifdef LOCATION
    evas_object_smart_callback_del (mod->o_browser,
                                    "browse_down", browser_cb_enter);
#endif
    evas_object_smart_callback_del (mod->o_browser,
                                    "hilight", browser_cb_hilight);

    ENNA_OBJECT_DEL (mod->o_browser);
    mod->o_browser = NULL;

    ENNA_OBJECT_DEL(mod->o_panel_infos);
    mod->o_panel_infos = NULL;

    _create_menu ();
#ifdef LOCATION
    enna_location_remove_nth (mod->o_location,
                              enna_location_count (mod->o_location) - 1);
#endif
}

#ifdef LOCATION
static void
browser_cb_enter (void *data, Evas_Object *obj, void *event_info)
{
    int n;
    const char *label ;

    n = enna_location_count (mod->o_location) - 1;
    label = enna_location_label_get_nth (mod->o_location, n);
    enna_browser_select_label (mod->o_browser, label);
    enna_location_remove_nth (mod->o_location, n);
}
#endif

static void
browser_cb_select (void *data, Evas_Object *obj, void *event_info)
{
    int i = 0;
    Enna_Vfs_File *f;
    Eina_List *l;
    Browser_Selected_File_Data *ev = event_info;

    if (!ev || !ev->file) return;

    if (ev->file->is_directory)
    {
        enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                  "Directory Selected %s", ev->file->uri);
#ifdef LOCATION
        enna_location_append (mod->o_location, ev->file->label,
                              NULL, NULL, NULL, NULL);
#endif
    }
    else
    {
        enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                  "File Selected %s", ev->file->uri);
        enna_mediaplayer_playlist_clear (mod->enna_playlist);

        /* File selected, create mediaplayer */
        EINA_LIST_FOREACH(ev->files, l, f)
        {
            if (!f->is_directory)
            {
                enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                          "Append : %s %s to playlist", f->label, f->uri);
                enna_mediaplayer_uri_append (mod->enna_playlist,
                                             f->uri, f->label);

                if (!strcmp (f->uri, ev->file->uri))
                {
                    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
                              "Select : %s %d in playlist", f->uri, i);
                    enna_mediaplayer_select_nth (mod->enna_playlist,i);

                    if (mod->o_current_uri)
                        free (mod->o_current_uri);
                    mod->o_current_uri = strdup(f->uri);
                }
                i++;
            }
        }

        mod->state = VIDEOPLAYER_VIEW;
	enna_mediaplayer_play (mod->enna_playlist);
//	enna_mediaplayer_position_set (m->position);
    }
    free (ev);
}

static void
browser_cb_hilight (void *data, Evas_Object *obj, void *event_info)
{
    Enna_Metadata *m = NULL;
    Browser_Selected_File_Data *ev = event_info;
    Enna_Metadata_Request *r;
    const char *label;

    if (!ev || !ev->file)
        return;

    if (!ev->file->is_directory)
    {
        m = enna_metadata_new (ev->file->uri);
        r = calloc (1, sizeof (Enna_Metadata_Request));
        r->metadata = m;
        r->caps  = ENNA_GRABBER_CAP_AUDIO;
        r->caps |= ENNA_GRABBER_CAP_VIDEO;
        r->caps |= ENNA_GRABBER_CAP_COVER;
        enna_metadata_grab_request (r);
        label = m->title ? m->title : ev->file->label;
    }
    else
        label = ev->file->label;

    edje_object_part_text_set (mod->o_edje, "enna.text.label", label);
    edje_object_part_text_set (mod->o_edje, "enna.text.category",
                               (m && m->categories) ? m->categories : "");

    backdrop_show (m);
    infos_flags_set (m);
    
    enna_panel_infos_set_cover(mod->o_panel_infos, m);
    enna_panel_infos_set_text(mod->o_panel_infos, m);
    enna_panel_infos_set_rating(mod->o_panel_infos, m);
}

static void
browse (void *data)
{
    Enna_Class_Vfs *vfs = data;

    if (!vfs)
        return;

    mod->o_browser = enna_browser_add (mod->em->evas);

    enna_browser_view_add (mod->o_browser, ENNA_BROWSER_VIEW_COVER);
    evas_object_smart_callback_add (mod->o_browser,
                                   "root", browser_cb_root, NULL);
    evas_object_smart_callback_add (mod->o_browser,
                                    "selected", browser_cb_select, NULL);
#ifdef LOCATION
    evas_object_smart_callback_add (mod->o_browser, "browse_down",
                                    browser_cb_enter, NULL);
#endif
    evas_object_smart_callback_add (mod->o_browser, "hilight",
                                    browser_cb_hilight, NULL);
    evas_object_show (mod->o_browser);

    edje_object_part_swallow (mod->o_edje,
                              "enna.swallow.browser", mod->o_browser);
    enna_browser_root_set (mod->o_browser, vfs);
    evas_object_del (mod->o_list);
    mod->o_list = NULL;
    
    ENNA_OBJECT_DEL(mod->o_panel_infos);
    mod->o_panel_infos = enna_panel_infos_add(mod->em->evas);
    edje_object_part_swallow (mod->o_edje,
                              "infos.panel.swallow", mod->o_panel_infos);
    
#ifdef LOCATION
    enna_location_append (mod->o_location,
                          gettext(vfs->label), NULL, NULL, NULL, NULL);
#endif
    mod->state = BROWSER_VIEW;
#ifdef LOCATION
    edje_object_signal_emit (mod->o_edje, "location,hide", "enna");
#endif
    edje_object_signal_emit(mod->o_edje, "tile,show", "enna");
    edje_object_signal_emit(mod->o_edje, "infos,flags,show", "enna");
}

/****************************************************************************/
/*                                  GUI                                     */
/****************************************************************************/

static void
_create_menu (void)
{
    Evas_Object *o;
    Eina_List *l, *categories;
    Enna_Class_Vfs *cat;

    /* Create List */
    o = enna_list_add(mod->em->evas);
    edje_object_signal_emit(mod->o_edje, "list,right,now", "enna");

    categories = enna_vfs_get(ENNA_CAPS_VIDEO);
    EINA_LIST_FOREACH(categories, l, cat)
    { 
        Video_Item_Class_Data *item;

        item = calloc(1, sizeof(Video_Item_Class_Data));
        item->icon = eina_stringshare_add(cat->icon);
        item->label = eina_stringshare_add(gettext(cat->label));
        enna_list_append(o, mod->item_class, item, item->label, browse, cat);
    }

    enna_list_selected_set(o, 0);
    mod->o_list = o;
    edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o);
    edje_object_signal_emit(mod->o_edje, "list,default", "enna");
#ifdef LOCATION
    edje_object_signal_emit(mod->o_edje, "location,show", "enna");
#endif
    edje_object_signal_emit(mod->o_edje, "tile,hide", "enna");
    edje_object_signal_emit(mod->o_edje, "infos,flags,hide", "enna");
    edje_object_part_text_set (mod->o_edje, "enna.text.label", "");
    edje_object_part_text_set (mod->o_edje, "enna.text.category", "");
    panel_infos_display (0);
    enna_backdrop_set (mod->o_backdrop, NULL, 0);
}

static void
_create_gui (void)
{
    Evas_Object *o;
#ifdef LOCATION
    Evas_Object *icon;
#endif

    mod->state = MENU_VIEW;
    o = edje_object_add(mod->em->evas);
    edje_object_file_set(o, enna_config_theme_get(), "module/video");
    mod->o_edje = o;
    _create_menu();
#ifdef LOCATION
    /* Create Location bar */
    o = enna_location_add(mod->em->evas);
    edje_object_part_swallow(mod->o_edje, "enna.swallow.location", o);
    icon = edje_object_add(mod->em->evas);
    edje_object_file_set(icon, enna_config_theme_get(), "icon/video_mini");
    enna_location_append(o, _("Video"), icon, NULL, NULL, NULL);
    mod->o_location = o;
#endif
}

/****************************************************************************/
/*                           List Management                               */
/****************************************************************************/

static char *
_genlist_label_get (const void *data, Evas_Object *obj, const char *part)
{
    const Video_Item_Class_Data *item = data;

    return item ? strdup (item->label) : NULL;
}

static Evas_Object *
_genlist_icon_get (const void *data, Evas_Object *obj, const char *part)
{
    const Video_Item_Class_Data *item = data;

    if (!item)
        return NULL;

    if (!strcmp (part, "elm.swallow.icon"))
    {
        Evas_Object *ic;

        ic = elm_icon_add (obj);
        elm_icon_file_set (ic, enna_config_theme_get (), item->icon);
        evas_object_size_hint_min_set (ic, 64, 64);
        evas_object_show (ic);
        return ic;
    }

    return NULL;
}

static Evas_Bool
_genlist_state_get (const void *data, Evas_Object *obj, const char *part)
{
    return 0;
}

static void
_genlist_del(const void *data, Evas_Object *obj)
{
    /* not yet implemented */
}

/****************************************************************************/
/*                         Private Module API                               */
/****************************************************************************/

static void
_class_init (int dummy)
{
    _create_gui ();
    enna_content_append ("video", mod->o_edje);
}

static void
_class_show (int dummy)
{
    edje_object_signal_emit (mod->o_edje, "module,show", "enna");

    switch (mod->state)
    {
    case BROWSER_VIEW:
	edje_object_signal_emit (mod->o_edje, "content,show", "enna");
        edje_object_signal_emit (mod->o_edje, "mediaplayer,hide", "enna");
#ifdef LOCATION
        edje_object_signal_emit (mod->o_edje, "location,hide", "enna");
#endif

    case MENU_VIEW:
        edje_object_signal_emit (mod->o_edje, "content,show", "enna");
        edje_object_signal_emit (mod->o_edje, "mediaplayer,hide", "enna");
#ifdef LOCATION
	edje_object_signal_emit (mod->o_edje, "location,show", "enna");
#endif

        break;

    case VIDEOPLAYER_VIEW:
        break;
    default:
        enna_log (ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                  "State Unknown in video module");
    }
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit (mod->o_edje, "module,hide", "enna");
}

static void
_class_event (void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key (ev);

    switch (mod->state)
    {
    case MENU_VIEW:
        menu_view_event (key, event_info);
        break;
    case BROWSER_VIEW:
        browser_view_event (key, event_info);
        break;
    case VIDEOPLAYER_VIEW:
        videoplayer_view_event (key);
        break;
    default:
        break;
    }
}

static Enna_Class_Activity class =
{
    "video",
    1,
    N_("Video"),
    NULL,
    "icon/video",
    {
        _class_init,
        NULL,
        NULL,
        _class_show,
        _class_hide,
        _class_event,
	NULL
    },
    NULL
};

static void
em_init(Enna_Module *em)
{
    mod = calloc(1, sizeof(Enna_Module_Video));
    mod->em = em;
    em->mod = mod;

    /* Create Class Item */
    mod->item_class = calloc(1, sizeof(Elm_Genlist_Item_Class));

    mod->item_class->item_style     = "default";
    mod->item_class->func.label_get = _genlist_label_get;
    mod->item_class->func.icon_get  = _genlist_icon_get;
    mod->item_class->func.state_get = _genlist_state_get;
    mod->item_class->func.del       = _genlist_del;

    mod->infos_displayed = 0;
    mod->o_backdrop = enna_backdrop_add (mod->em->evas);
    mod->browser_refresh_handler =
	ecore_event_handler_add(ENNA_EVENT_REFRESH_BROWSER, browser_cb_refresh, NULL);
    enna_activity_add(&class);
    mod->enna_playlist = enna_mediaplayer_playlist_create();
}

static void
em_shutdown(Enna_Module *em)
{
    ENNA_EVENT_HANDLER_DEL(mod->browser_refresh_handler);
    ENNA_EVENT_HANDLER_DEL(mod->eos_event_handler);
    ENNA_OBJECT_DEL(mod->o_edje);
    ENNA_OBJECT_DEL(mod->o_list);
    evas_object_smart_callback_del(mod->o_browser, "root", browser_cb_root);
    evas_object_smart_callback_del(mod->o_browser, "selected", browser_cb_select);
#ifdef LOCATION
    evas_object_smart_callback_del(mod->o_browser, "browse_down", browser_cb_enter);
#endif
    evas_object_smart_callback_del(mod->o_browser, "hilight", browser_cb_hilight);
    ENNA_OBJECT_DEL(mod->o_browser);
#ifdef LOCATION
    ENNA_OBJECT_DEL(mod->o_location);
#endif
    ENNA_TIMER_DEL(mod->timer_show_mediaplayer);
    ENNA_OBJECT_DEL(mod->o_mediaplayer);
    ENNA_OBJECT_DEL(mod->o_backdrop);
    ENNA_OBJECT_DEL(mod->o_panel_infos);
    ENNA_OBJECT_DEL(mod->o_flag_video);
    ENNA_OBJECT_DEL(mod->o_flag_audio);
    ENNA_OBJECT_DEL(mod->o_flag_studio);
    ENNA_OBJECT_DEL(mod->o_flag_media);
    ENNA_FREE(mod->o_current_uri);
    enna_mediaplayer_playlist_free(mod->enna_playlist);
    free(mod);
}

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_ACTIVITY,
    "activity_video"
};

void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    em_init(em);
}

void
module_shutdown(Enna_Module *em)
{
    em_shutdown(em);
}
