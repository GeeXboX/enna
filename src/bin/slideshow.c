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

#include <Ecore.h>
#include <Edje.h>

#include "enna_config.h"
#include "slideshow.h"
#include "image.h"
#include "logs.h"

#define SMART_NAME "slideshow"

#define NB_TRANSITIONS_MAX 3.0

#define STOP 0
#define PLAY 1
#define PAUSE 2

typedef struct _Smart_Data Smart_Data;

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
};

/* local subsystem functions */
static void _enna_slideshow_smart_reconfigure(Smart_Data * sd);
static void _enna_slideshow_smart_init(void);
static void _random_transition(Smart_Data *sd);
static void _edje_cb(void *data, Evas_Object *obj, const char *emission,
        const char *source);
static void _switch_images(Smart_Data * sd, Evas_Object * new_slide);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

void enna_slideshow_append_img(Evas_Object *obj, const char *filename)
{
    API_ENTRY return;

    if (filename)
        sd->playlist = eina_list_append (sd->playlist, strdup (filename));
}

void enna_slideshow_append_list(Evas_Object *obj, Eina_List *list)
{
    API_ENTRY return;

    if (list)
    {
        Eina_List *l;
        const char *filename;

        EINA_LIST_FOREACH(list, l, filename)
        {
            enna_slideshow_append_img (obj, filename);
        }
    }
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
            sd->playlist_id = i;
            break;
        }
        i++;
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
        return 1;
    }
    else
    {
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
        return 1;
    }
    else
    {
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

        /* Play */
        sd->state = PLAY;
        filename = eina_list_nth(sd->playlist, sd->playlist_id);
        o = enna_slideshow_create_img (obj, filename);
        if (o)
            _switch_images(sd, o);
        sd->timer = ecore_timer_add(enna->slideshow_delay,
                                    enna_slideshow_timer, sd->obj);
    }
    else
    {

        /* Pause */
        sd->state = PAUSE;
        ENNA_TIMER_DEL(sd->timer);
    }

}

/* local subsystem globals */

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

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;

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
    _random_transition(sd);
}

static void _smart_del(Evas_Object * obj)
{
    Eina_List *l;
    INTERNAL_ENTRY;

    for (l = sd->playlist; l; l = l->next)
        ENNA_FREE (l->data);

    evas_object_del(sd->o_edje);
    evas_object_del(sd->old_slide);
    evas_object_del(sd->slide);
    evas_object_del(sd->o_transition);
    ENNA_TIMER_DEL(sd->timer);
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

