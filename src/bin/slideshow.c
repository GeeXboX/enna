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

#include <string.h>
#include <time.h>

#include <Elementary.h>
#include <Ecore.h>
#include <Edje.h>

#include "enna_config.h"
#include "slideshow.h"
#include "image.h"
#include "logs.h"
#include "vfs.h"

#define SMART_NAME "slideshow"

#define NB_TRANSITIONS_MAX 3.0

#define STOP 0
#define PLAY 1
#define PAUSE 2

#define SLIDESHOW_MOUSE_IDLE_TIMEOUT 5

typedef struct _Smart_Data Smart_Data;

typedef struct _Smart_Btn_Item
{
    Evas_Object *bt;
    Evas_Object *ic;
}Smart_Btn_Item;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_edje;
    Evas_Object *o_transition;
    Evas_Object *obj;
    Eina_List *playlist;
    unsigned int playlist_id;
    Ecore_Timer *timer;
    Evas_Object *old_slide;
    Evas_Object *slide;
    unsigned char state;
    Eina_List *btns;
    Evas_Object *o_btn_box;
    Ecore_Timer *mouse_idle_timer;
    Evas_Object *first_last;
};

/* local subsystem functions */
static void _enna_slideshow_smart_reconfigure(Smart_Data * sd);
static void _enna_slideshow_smart_init(void);
static void _random_transition(Smart_Data *sd);
static void _edje_cb(void *data, Evas_Object *obj, const char *emission,
        const char *source);
static void _switch_images(Smart_Data * sd, Evas_Object * new_slide);
static void _update_infos (Smart_Data *sd);
/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

void enna_slideshow_append_img(Evas_Object *obj, const char *filename)
{
    API_ENTRY return;

    if (filename && !ecore_file_is_dir(filename))
    {
        sd->playlist = eina_list_append (sd->playlist, strdup (filename));
        sd->playlist = eina_list_sort(sd->playlist, eina_list_count(sd->playlist), EINA_COMPARE_CB(strcasecmp));
    }   
}

void enna_slideshow_append_list(Evas_Object *obj, Eina_List *list)
{
    API_ENTRY return;

    if (list)
    {
        Eina_List *l;
        Enna_Vfs_File *file;

        EINA_LIST_FOREACH(list, l, file)
        {
            enna_slideshow_append_img (obj, file->uri + 7);
        }
        
    }
}

static Evas_Object *
enna_slideshow_create_img (Evas_Object *obj, const char *filename)
{
    Evas_Object *o;
    Evas_Coord w, h;

    if (!filename)
        return NULL;

    o = enna_image_add (evas_object_evas_get (obj));
    enna_log (ENNA_MSG_EVENT, NULL, "append : %s", filename);
    enna_image_file_set (o, filename, NULL);
    enna_image_size_get (o, &w, &h);
    enna_image_load_size_set (o, w, h);

    return o;
}

void enna_slideshow_set (Evas_Object *obj, const char *pos)
{
    Eina_List *l, *list;
    const char *filename;
    int i = 0;

    API_ENTRY return;

    if (!pos)
        return;

    list = sd->playlist;
    EINA_LIST_FOREACH(list, l, filename)
    {
        if (!strcmp (filename, pos))
        {
            Evas_Object *o;
            sd->playlist_id = i;
            o = enna_slideshow_create_img (obj, filename);
            if (o)
            {
                _switch_images (sd, o);
                _update_infos (sd);
            }
            break;
        }
        i++;
    }
}

int enna_slideshow_next (Evas_Object *obj)
{
    char *filename;
    Evas_Object *o;

    API_ENTRY
        return 0;

    if (sd->old_slide)
        return 1;

    sd->playlist_id++;
    filename = eina_list_nth (sd->playlist, sd->playlist_id);

    o = enna_slideshow_create_img (obj, filename);
    if (o)
    {
        _switch_images (sd, o);
        _update_infos (sd);
        return 1;
    }
    else
    {
        ENNA_OBJECT_DEL(sd->first_last);
        sd->first_last = elm_icon_add(sd->o_edje);
        elm_icon_file_set(sd->first_last, enna_config_theme_get(), "icon/go_last");
        edje_object_part_swallow(sd->o_edje, "enna.swallow.first_last", sd->first_last);
        edje_object_signal_emit(sd->o_edje, "first_last,pulse", "enna");
        sd->playlist_id--;
        return 0;
    }
}

int enna_slideshow_prev (Evas_Object *obj)
{
    Evas_Object *o;
    char *filename;

    API_ENTRY
        return 0;

    if (sd->old_slide)
        return 1;

    sd->playlist_id--;
    filename = eina_list_nth (sd->playlist, sd->playlist_id);

    o = enna_slideshow_create_img (obj, filename);
    if (o)
    {
        if (sd->state == PLAY)
        {
            sd->state = PAUSE;
            ENNA_TIMER_DEL(sd->timer);
        }
        _switch_images(sd, o);
        _update_infos (sd);
        return 1;
    }
    else
    {
        ENNA_OBJECT_DEL(sd->first_last);
        sd->first_last = elm_icon_add(sd->o_edje);
        elm_icon_file_set(sd->first_last, enna_config_theme_get(), "icon/go_first");
        edje_object_part_swallow(sd->o_edje, "enna.swallow.first_last", sd->first_last);
        edje_object_signal_emit(sd->o_edje, "first_last,pulse", "enna");
        sd->playlist_id++;
        return 0;
    }
}

static int
enna_slideshow_timer (void *data)
{
    Evas_Object *obj = data;
    return enna_slideshow_next (obj);
}

void enna_slideshow_play(Evas_Object *obj)
{
    API_ENTRY
    return;

    if (!sd->timer)
    {
        char *filename;
        Evas_Object *o;
        Smart_Btn_Item *it;
        /* Play */
        sd->state = PLAY;
        filename = eina_list_nth(sd->playlist, sd->playlist_id);
        o = enna_slideshow_create_img (obj, filename);
        if (o)
        {
            _switch_images(sd, o);
            _update_infos (sd);
        }
        sd->timer = ecore_timer_add(enna->slideshow_delay,
                                    enna_slideshow_timer, sd->obj);

        it = eina_list_nth (sd->btns, 1);
        ENNA_OBJECT_DEL(it->ic);
        it->ic = elm_icon_add(sd->o_edje);
        elm_icon_file_set(it->ic, enna_config_theme_get(), "icon/mp_pause");
        elm_button_icon_set(it->bt, it->ic);
        evas_object_size_hint_min_set(it->bt, 64, 64);
    }
    else
    {
        Smart_Btn_Item *it;
        it = eina_list_nth (sd->btns, 1);
        ENNA_OBJECT_DEL(it->ic);
        it->ic = elm_icon_add(sd->o_edje);
        elm_icon_file_set(it->ic, enna_config_theme_get(), "icon/mp_play");
        elm_button_icon_set(it->bt, it->ic);
        evas_object_size_hint_min_set(it->bt, 64, 64);
        /* Pause */
        sd->state = PAUSE;
        ENNA_TIMER_DEL (sd->timer);
        
    }

}

/* local subsystem globals */

static void _update_infos (Smart_Data *sd)
{
    char buf[1024];
        
    snprintf (buf, sizeof(buf), "%d / %d", 
        sd->playlist_id + 1, 
        eina_list_count(sd->playlist));
    edje_object_part_text_set (sd->o_edje, "number.text", buf);
    edje_object_part_text_set (sd->o_edje, "filename.text", 
        ecore_file_file_get(eina_list_nth(sd->playlist, sd->playlist_id)));
    
}

static void _random_transition(Smart_Data *sd)
{
    unsigned int n;
    static const char * transitions_array[(int) NB_TRANSITIONS_MAX] = {
        "transitions/crossfade",
        "transitions/vswipe",
        "transitions/hslide"
    };

    if (!sd)
        return;

    n = 1 + (int) ( NB_TRANSITIONS_MAX * rand() / (RAND_MAX + 1.0 ));
    if (n > NB_TRANSITIONS_MAX)
      n = NB_TRANSITIONS_MAX;

    if (sd->o_transition)
        evas_object_del(sd->o_transition);
    sd->o_transition = edje_object_add(evas_object_evas_get(sd->obj));
    enna_log(ENNA_MSG_EVENT, NULL, "Transition n°%d: %s",
             n, transitions_array[n-1]);
    edje_object_file_set(sd->o_transition, enna_config_theme_get(),
                         transitions_array[n-1]);

    edje_object_part_swallow(sd->o_edje, "enna.swallow.transition",
            sd->o_transition);
    edje_object_signal_callback_add(sd->o_transition, "*", "*", _edje_cb, sd);
}

static void _edje_cb(void *data, Evas_Object *obj, const char *emission,
        const char *source)
{
    Smart_Data *sd = (Smart_Data*)data;

    if (!strcmp(emission, "done"))
    {
        edje_object_part_unswallow(sd->o_transition, sd->old_slide);
        evas_object_hide(sd->old_slide);
        sd->old_slide = NULL;
    }
}

static void _switch_images(Smart_Data * sd, Evas_Object * new_slide)
{

    if (!sd || !new_slide || !sd->o_transition)
        return;

    _random_transition(sd);

    edje_object_part_unswallow(sd->o_transition, sd->slide);
    edje_object_part_unswallow(sd->o_transition, sd->old_slide);
    sd->old_slide = sd->slide;
    sd->slide = new_slide;
    edje_object_signal_emit(sd->o_transition, "reset", "enna");
    edje_object_part_swallow(sd->o_transition, "slide.1", sd->old_slide);
    edje_object_part_swallow(sd->o_transition, "slide.2", sd->slide);
    edje_object_signal_emit(sd->o_transition, "show,2", "enna");
    ENNA_OBJECT_DEL (sd->old_slide);
}

static void _enna_slideshow_smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, x, y);
    evas_object_resize(sd->o_edje, w, h);

}

#define ELM_ADD(icon, cb)                                            \
    ic = elm_icon_add(obj);                                          \
    elm_icon_file_set(ic, enna_config_theme_get(), icon);            \
    elm_icon_scale_set(ic, 0, 0);                                    \
    bt = elm_button_add(obj);                                        \
    elm_button_style_set(bt, "simple");                              \
    evas_object_smart_callback_add(bt, "clicked", cb, sd);           \
    elm_button_icon_set(bt, ic);                                     \
    evas_object_size_hint_min_set(bt, 64, 64);                       \
    evas_object_size_hint_weight_set(bt, 0.0, 1.0);                  \
    evas_object_size_hint_align_set(bt, 0.5, 0.5);                   \
    elm_box_pack_end(sd->o_btn_box, bt);                             \
    evas_object_show(bt);                                            \
    evas_object_show(ic);                                            \
    it = calloc(1, sizeof(Smart_Btn_Item));			                 \
    it->bt = bt;						                             \
    it->ic = ic;						                             \
    sd->btns = eina_list_append(sd->btns, it);			             \
    
static void
_button_clicked_play_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    enna_slideshow_play(sd->obj);
}

static void
_button_clicked_prev_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    enna_slideshow_prev(sd->obj); 
}

static void
_button_clicked_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    enna_slideshow_next(sd->obj);   
}

static void
_button_clicked_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    evas_event_feed_key_down(enna->evas, "BackSpace", "BackSpace", "BackSpace", NULL, ecore_time_get(), sd); 
}

static void
_button_clicked_rotate_ccw_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    
    if (!sd->slide) return;
    
    enna_image_orient_set(sd->slide, ENNA_IMAGE_ROTATE_90_CCW);    
    
    
}

static void
_button_clicked_rotate_cw_cb(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    
    if (!sd->slide) return;
    
    enna_image_orient_set(sd->slide, ENNA_IMAGE_ROTATE_90_CW);   
}

static int _mouse_idle_timer_cb(void *data)
{
    Smart_Data *sd = data;

    edje_object_signal_emit(sd->o_edje, "iconbar,hide", "enna");
    enna_log(ENNA_MSG_EVENT, NULL, "hiding icon bar.");
    ENNA_TIMER_DEL(sd->mouse_idle_timer);
    return ECORE_CALLBACK_CANCEL;
}

static void _mousemove_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    if (!sd->mouse_idle_timer)
    {
        edje_object_signal_emit(sd->o_edje, "iconbar,show", "enna");
        enna_log(ENNA_MSG_EVENT, NULL, "unhiding iconbar.");
        sd->mouse_idle_timer = ecore_timer_add(SLIDESHOW_MOUSE_IDLE_TIMEOUT, _mouse_idle_timer_cb, sd);
    }
    else
    {
        ENNA_TIMER_DEL(sd->mouse_idle_timer);
        sd->mouse_idle_timer = ecore_timer_add(SLIDESHOW_MOUSE_IDLE_TIMEOUT, _mouse_idle_timer_cb, sd);
    }
  
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;
    Evas_Object *ic, *bt;
    Smart_Btn_Item *it;
        
    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;

    srand(time(NULL));

    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "transitions");
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;

    sd->obj = obj;

    sd->h = 0;
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);

    sd->playlist = NULL;
    sd->playlist_id = 0;
    sd->timer = NULL;
    sd->old_slide = NULL;
    sd->slide = NULL;
    sd->state = STOP;
    sd->o_transition = NULL;
    
    sd->o_btn_box = elm_box_add(sd->o_edje);
    elm_box_homogenous_set(sd->o_btn_box, 0);
    elm_box_horizontal_set(sd->o_btn_box, 1);
    evas_object_size_hint_align_set(sd->o_btn_box, 0.5, 0.5);
    evas_object_size_hint_weight_set(sd->o_btn_box, 0.0, -1.0);
    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", sd->o_btn_box);
    
    ELM_ADD ("icon/mp_prev",    _button_clicked_prev_cb);
    ELM_ADD ("icon/mp_play",    _button_clicked_play_cb);
    ELM_ADD ("icon/mp_next",    _button_clicked_next_cb);
    ELM_ADD ("icon/mp_stop",    _button_clicked_stop_cb);
    ELM_ADD ("icon/rotate_ccw", _button_clicked_rotate_ccw_cb);
    ELM_ADD ("icon/rotate_cw",  _button_clicked_rotate_cw_cb);
    
    _random_transition(sd);
    edje_object_signal_emit(sd->o_edje, "iconbar,show", "enna");
    enna_log(ENNA_MSG_EVENT, NULL, "unhiding iconbar.");
    sd->mouse_idle_timer = ecore_timer_add(SLIDESHOW_MOUSE_IDLE_TIMEOUT, _mouse_idle_timer_cb, sd);
    evas_object_event_callback_add(sd->o_edje, EVAS_CALLBACK_MOUSE_MOVE, _mousemove_cb, sd);
    evas_object_event_callback_add(sd->o_edje, EVAS_CALLBACK_MOUSE_DOWN, _mousemove_cb, sd);
}

static void _smart_del(Evas_Object * obj)
{
    Eina_List *l;
    INTERNAL_ENTRY;

    for (l = sd->playlist; l; l = l->next)
        ENNA_FREE (l->data);
    while (sd->btns)
    {
        Smart_Btn_Item *it = sd->btns->data;
        sd->btns = eina_list_remove_list(sd->btns, sd->btns);
        evas_object_del(it->ic);
        evas_object_del(it->bt);
        free(it);
    }
    evas_object_del(sd->first_last);
    evas_object_del(sd->o_btn_box);
    evas_object_del(sd->o_edje);
    evas_object_del(sd->old_slide);
    evas_object_del(sd->slide);
    evas_object_del(sd->o_transition);
    ENNA_TIMER_DEL(sd->timer);
    ENNA_TIMER_DEL(sd->mouse_idle_timer);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _enna_slideshow_smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _enna_slideshow_smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_edje);
}

static void _enna_slideshow_smart_init(void)
{
    static const Evas_Smart_Class sc =
    {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _smart_add,
        _smart_del,
        _smart_move,
        _smart_resize,
        _smart_show,
        _smart_hide,
        _smart_color_set,
        _smart_clip_set,
        _smart_clip_unset,
        NULL,
        NULL
    };

    if (!_e_smart)
       _e_smart = evas_smart_class_new(&sc);
}

/* externally accessible functions */
Evas_Object *
enna_slideshow_add(Evas * evas)
{
    _enna_slideshow_smart_init();
    return evas_object_smart_add(evas, _e_smart);
}

