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
 * Smart Callback event
 *
 * "root" this event is sent when root is browse
 * "selected" this event is sent when a file or a directory is selected
 * "browse_down" this event is sent when browse down is detected
 *
 */

#include <string.h>

#include <Elementary.h>
#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "browser.h"
#include "view_cover.h"
#include "view_list.h"
#include "view_wall.h"
#include "image.h"
#include "logs.h"
#include "event_key.h"
#include "input.h"

#define SMART_NAME "Enna_Browser"

typedef struct _Smart_Data Smart_Data;
typedef struct _Browse_Data Browse_Data;

struct _Browse_Data
{
    Enna_Vfs_File *file;
    Smart_Data *sd;
};

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *obj;
    Evas_Object *o_edje;
    Evas_Object *o_view;
    Evas_Object *o_letter;
    Eina_List *files;
    Enna_Class_Vfs *vfs;
    Enna_Vfs_File *file;
    Evas *evas;
    Eina_List *visited;
    unsigned int letter_mode;
    Ecore_Timer *letter_timer;
    unsigned int letter_event_nbr;
    char letter_key;
    struct
    {
	    Evas_Object * (*view_add)(Smart_Data *sd);
	    void (*view_append)(
	        Evas_Object *view,
	        Enna_Vfs_File *file,
	        void (*func) (void *data),
	        void *data);
	    void * (*view_selected_data_get)(Evas_Object *view);
	    int (*view_jump_label)(Evas_Object *view, const char *label);
	    Eina_Bool (*view_key_down)(Evas_Object *view, enna_input event);
	    void (*view_select_nth)(Evas_Object *obj, int nth);
	    Eina_List *(*view_files_get)(Evas_Object *obj);
	    void (*view_jump_ascii)(Evas_Object *obj, char k);
    }view_funcs;
    unsigned char accept_ev : 1;
    unsigned char show_file : 1;
};



/* local subsystem functions */
static void _list_transition_right_end_cb(void *data, Evas_Object *o, const char *sig, const char *src);
static void _list_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src);
static void _view_hilight_cb (void *data, Evas_Object *obj, void *event_info);

static void _smart_reconfigure(Smart_Data * sd);
static void _smart_init(void);
static void _smart_add(Evas_Object * obj);
static void _smart_del(Evas_Object * obj);
static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object * obj);
static void _smart_hide(Evas_Object * obj);
static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void _smart_clip_unset(Evas_Object * obj);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;

/* Browser View */



static Evas_Object *
_browser_view_list_add(Smart_Data *sd)
{
    Evas_Object *view;

    if (!sd) return NULL;

    view = enna_list_add(sd->evas);

    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", view);
    /* View */
    edje_object_signal_emit(view, "list,right,now", "enna");

    return view;
}


static Evas_Object *
_browser_view_cover_add(Smart_Data *sd)
{
    Evas_Object *view;

    if (!sd) return NULL;

    view = enna_view_cover_add(evas_object_evas_get(sd->o_edje));

    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", view);
    evas_object_smart_callback_add(view, "hilight", _view_hilight_cb, sd);
    return view;
}

static Evas_Object *
_browser_view_wall_add(Smart_Data *sd)
{
    Evas_Object *view;

    if (!sd) return NULL;

    view = enna_wall_add(evas_object_evas_get(sd->o_edje));

    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", view);
    evas_object_smart_callback_add(view, "hilight", _view_hilight_cb, sd);
    return view;
}

static void
_browser_view_wall_select_nth(Evas_Object *view, int nth)
{
    enna_wall_select_nth(view, nth, 0);
}


/* externally accessible functions */
Evas_Object *
enna_browser_add(Evas * evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _smart);
}

static void
_view_hilight_cb (void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    Browser_Selected_File_Data *ev;
    Browse_Data *bd = event_info;

    if (!sd || !bd) return;

    ev = calloc(1, sizeof(Browser_Selected_File_Data));
    ev->file = bd->file;
    ev->files = NULL;

    evas_object_smart_callback_call (sd->obj, "hilight", ev);
}

void enna_browser_view_add(Evas_Object *obj, Enna_Browser_View_Type view_type)
{
    API_ENTRY return;

    switch(view_type)
    {
    case ENNA_BROWSER_VIEW_LIST:
	    sd->view_funcs.view_add = _browser_view_list_add;
	    sd->view_funcs.view_append =  enna_list_file_append;
	    sd->view_funcs.view_selected_data_get =  enna_list_selected_data_get;
	    sd->view_funcs.view_jump_label =  enna_list_jump_label;
	    sd->view_funcs.view_key_down = enna_list_input_feed;
	    sd->view_funcs.view_select_nth = enna_list_select_nth;
	    sd->view_funcs.view_files_get = enna_list_files_get;
	    sd->view_funcs.view_jump_ascii = enna_list_jump_ascii;
	    break;
    case ENNA_BROWSER_VIEW_COVER:
	    sd->view_funcs.view_add = _browser_view_cover_add;
	    sd->view_funcs.view_append =  enna_view_cover_file_append;
	    sd->view_funcs.view_selected_data_get =  enna_view_cover_selected_data_get;
	    sd->view_funcs.view_jump_label = enna_view_cover_jump_label;
	    sd->view_funcs.view_key_down = enna_view_cover_input_feed;
	    sd->view_funcs.view_select_nth = enna_view_cover_select_nth;
	    sd->view_funcs.view_files_get = enna_view_cover_files_get;
	    sd->view_funcs.view_jump_ascii = enna_view_cover_jump_ascii;
	    break;
    case ENNA_BROWSER_VIEW_WALL:
	    sd->view_funcs.view_add = _browser_view_wall_add;
	    sd->view_funcs.view_append =  enna_wall_file_append;
	    sd->view_funcs.view_selected_data_get =  enna_wall_selected_data_get;
	    sd->view_funcs.view_jump_label =  enna_wall_jump_label;
	    sd->view_funcs.view_key_down = enna_wall_input_feed;
	    sd->view_funcs.view_select_nth = _browser_view_wall_select_nth;
	    sd->view_funcs.view_files_get = enna_wall_files_get;
  	    sd->view_funcs.view_jump_ascii = enna_wall_jump_ascii;
        default:
	break;
    }
    evas_object_smart_callback_del(sd->o_view, "hilight", _view_hilight_cb);
    ENNA_OBJECT_DEL(sd->o_view);
    sd->o_view = sd->view_funcs.view_add(sd);

}

void enna_browser_show_file_set(Evas_Object *obj, unsigned char show)
{
    API_ENTRY return;

    sd->show_file = show;
}

int
enna_browser_select_label(Evas_Object *obj, const char *label)
{

    API_ENTRY return -1;

    if (!sd || !sd->o_view) return -1;

    if (sd->view_funcs.view_jump_label)
	    sd->view_funcs.view_jump_label(sd->o_view, label);

    return 0;

}

Eina_List *
enna_browser_files_get(Evas_Object *obj)
{
     API_ENTRY return NULL;
     if (!sd->o_view) return NULL;

     if (sd->view_funcs.view_files_get)
	    return sd->view_funcs.view_files_get(sd->o_view);

     return NULL;
}

/* local subsystem globals */
static void _smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, x, y);
    evas_object_resize(sd->o_edje, w, h);
}

/* Class Item interface */

static void _smart_init(void)
{
    if (_smart)
        return;
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
    _smart = evas_smart_class_new(&sc);
}



static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;

    sd->evas = evas_object_evas_get(obj);
    sd->o_edje = edje_object_add(sd->evas);
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "enna/browser");

    sd->view_funcs.view_add = _browser_view_list_add;
    sd->view_funcs.view_append =  enna_list_file_append;
    sd->view_funcs.view_selected_data_get =  enna_list_selected_data_get;
    sd->view_funcs.view_jump_label =  enna_list_jump_label;
    sd->view_funcs.view_key_down = enna_list_input_feed;
    sd->view_funcs.view_select_nth = enna_list_select_nth;

    sd->o_view = sd->view_funcs.view_add(sd);
    evas_object_smart_callback_add(sd->o_view, "hilight", _view_hilight_cb, sd);

    edje_object_signal_emit(sd->o_edje, "letter,hide", "enna");
    sd->o_letter =  elm_button_add(obj);
    elm_button_label_set(sd->o_letter, "");
    elm_object_scale_set(sd->o_letter, 6.0);
    evas_object_show(sd->o_letter);
    edje_object_part_swallow(sd->o_edje, "enna.swallow.letter", sd->o_letter);

    sd->accept_ev = 1;
    sd->show_file = 1;
    evas_object_smart_member_add(sd->o_edje, obj);
    sd->obj = obj;
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    edje_object_signal_callback_del(sd->o_edje, "list,transition,end", "edje", _list_transition_right_end_cb);
    edje_object_signal_callback_del(sd->o_edje, "list,transition,end", "edje", _list_transition_left_end_cb);
    ENNA_OBJECT_DEL(sd->o_view);
    evas_object_del(sd->o_edje);
    evas_object_del(sd->o_letter);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
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

static void
_list_transition_default_up_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{
    Smart_Data *sd = data;
    if (!data) return;

    sd->accept_ev = 1;

    sd->view_funcs.view_select_nth(sd->o_view, 0);
    edje_object_signal_callback_del(sd->o_edje, "list,transition,default,end", "edje",
	    _list_transition_default_up_end_cb);
}

static void
_list_transition_default_down_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{
    Smart_Data *sd = data;
    Enna_Vfs_File *last;
    int selected = -1;
    if (!data) return;

    sd->accept_ev = 1;

    last = eina_list_nth(sd->visited, eina_list_count(sd->visited) - 1);

    /* Remove last entry in visited files*/
    sd->visited = eina_list_remove_list(sd->visited, eina_list_last(sd->visited));



    if (last && last->label)
    {
        printf("will try to select : %s\n", last->label);
        selected = sd->view_funcs.view_jump_label(sd->o_view, last->label);
        if (selected == -1)
            sd->view_funcs.view_select_nth(sd->o_view, 0);
    }
    else
        sd->view_funcs.view_select_nth(sd->o_view, 0);

    edje_object_signal_callback_del(sd->o_edje, "list,transition,default,end", "edje",
	    _list_transition_default_down_end_cb);
}

static  void _browse(void *data)
{
    Smart_Data *sd;
    Browse_Data *bd = data;
    Enna_Vfs_File *visited;
    if (!bd)
	return;

    sd = bd->sd;
    sd->file = bd->file;

    if (!sd || !sd->vfs)
        return;

    /* FIXME : List / Gen List */
    sd->accept_ev = 0;
    if (sd->vfs->func.class_browse_up)
    {
        Browser_Selected_File_Data *ev = calloc(1, sizeof(Browser_Selected_File_Data));
        ev->vfs = sd->vfs;
        ev->file = sd->file;

        if (sd->file && sd->file->is_directory)
        {
            /* File selected is a directory */
            sd->files = sd->vfs->func.class_browse_up(sd->file->uri, sd->vfs->cookie);
            /* No media found */
           if (!eina_list_count(sd->files))
            {

                sd->file = enna_vfs_create_directory(sd->file->uri, _("No media found !"), "icon_nofile", NULL);
                sd->files = NULL;
                sd->files = eina_list_append(sd->files,sd->file);

            }
            else
            {
                ev->file = sd->file;
                ev->files = sd->files;
                evas_object_smart_callback_call (sd->obj, "selected", ev);
            }
        }
        else if (sd->show_file)
        {
            /* File selected is a regular file */
            Enna_Vfs_File *prev_vfs;
            char *prev_uri;
            prev_vfs = sd->vfs->func.class_vfs_get(sd->vfs->cookie);
	        prev_uri = prev_vfs->uri ? strdup(prev_vfs->uri) : NULL;
            sd->files = sd->vfs->func.class_browse_up(prev_uri, sd->vfs->cookie);
            ENNA_FREE(prev_uri);
            ev->files = sd->files;
            sd->accept_ev = 1;
            evas_object_smart_callback_call (sd->obj, "selected", ev);
            return;
        }

        /* Add last selected file in visited list */
        visited = calloc(1, sizeof(Enna_Vfs_File));
        visited->label = strdup(sd->file->label);
        visited->uri = strdup(sd->file->uri);
        sd->visited = eina_list_append(sd->visited, visited);

        /* Clear list and add new items */
        edje_object_signal_callback_add(sd->o_edje, "list,transition,end", "edje",
	    _list_transition_left_end_cb, sd);
        edje_object_signal_emit(sd->o_edje, "list,left", "enna");
    }
}

static void _browse_down(void *data)
{
    Smart_Data *sd = data;

    if (!sd) return;

    sd->accept_ev = 0;

    if (sd->vfs && sd->vfs->func.class_browse_down)
    {
        sd->files = sd->vfs->func.class_browse_down(sd->vfs->cookie);
        sd->file = sd->vfs->func.class_vfs_get(sd->vfs->cookie);
        if (!sd->files)
        {
            evas_object_smart_callback_call (sd->obj, "root", NULL);
            return;
        }

        /* Clear list and add new items */
        edje_object_signal_callback_add(sd->o_edje, "list,transition,end", "edje",
            _list_transition_right_end_cb, sd);
        edje_object_signal_emit(sd->o_edje, "list,right", "enna");

    }
}

static void
_list_transition_core(Smart_Data *sd, unsigned char direction)
{

    Eina_List *l;
    Eina_List *files = sd->files;

    if (!direction)
    {
        edje_object_signal_callback_del(sd->o_edje, "list,transition,end", "edje",
            _list_transition_left_end_cb);
        edje_object_signal_callback_add(sd->o_edje, "list,transition,default,end", "edje",
	        _list_transition_default_up_end_cb, sd);
    }
    else
    {
        edje_object_signal_callback_del(sd->o_edje, "list,transition,end", "edje",
            _list_transition_right_end_cb);
        edje_object_signal_callback_add(sd->o_edje, "list,transition,default,end", "edje",
	        _list_transition_default_down_end_cb, sd);
    }


    ENNA_OBJECT_DEL(sd->o_view);
    sd->o_view = sd->view_funcs.view_add(sd);

    if (direction == 0)
    {

        edje_object_signal_emit(sd->o_edje, "list,right,now", "enna");
    }
    else
    {
	    evas_object_smart_callback_call (sd->obj, "browse_down", NULL);
        edje_object_signal_emit(sd->o_edje, "list,left,now", "enna");
    }

    if (eina_list_count(files))
    {
        int i = 0;

        /* Create list of files */
        for (l = files, i = 0; l; l = l->next, i++)
        {
            Enna_Vfs_File *f;
            Evas_Object *icon = NULL;
            Browse_Data *bd;

            f = l->data;

            if (!f->is_directory && !sd->show_file)
                continue;

            if (f->icon_file && f->icon_file[0] == '/')
            {
                icon = enna_image_add(sd->evas);
                enna_image_file_set(icon, f->icon_file, NULL);
            }
            else
            {
                icon = edje_object_add(sd->evas);
                edje_object_file_set(icon, enna_config_theme_get(), f->icon);
            }

            bd = calloc(1, sizeof(Browse_Data));
            bd->file = f;
            bd->sd = sd;
	        sd->view_funcs.view_append(sd->o_view, f, _browse, bd);
        }

    }
    else
    {
        /* Browse down and no file detected : Root */
        sd->vfs = NULL;

    }
    edje_object_signal_emit(sd->o_edje, "list,default", "enna");
    //sd->accept_ev = 1;
}

static void
_list_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{
    _list_transition_core(data, 0);
}

static void
_list_transition_right_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{
    _list_transition_core(data, 1);
}

static int _letter_timer_cb(void *data)
{
    Smart_Data *sd;

    sd = data;
    if (!sd) return 0;

    edje_object_signal_emit(sd->o_edje, "letter,hide", "enna");
    sd->letter_mode = 0;
    ENNA_TIMER_DEL(sd->letter_timer);
    return ECORE_CALLBACK_CANCEL;
}

void enna_browser_root_set(Evas_Object *obj, Enna_Class_Vfs *vfs)
{
    API_ENTRY return;

    if (!vfs) return;

    if (vfs->func.class_browse_up)
    {
        /* create Root menu */
        sd->files = vfs->func.class_browse_up(NULL, vfs->cookie);
        sd->vfs = vfs;
        edje_object_signal_callback_add(sd->o_edje, "list,transition,end", "edje", _list_transition_left_end_cb, sd);
        edje_object_signal_emit(sd->o_edje, "list,left", "enna");
    }
}

static void _jump_to_ascii(Smart_Data *sd, char k)
{
    if (!sd) return;

     if (sd->view_funcs.view_jump_ascii)
	    sd->view_funcs.view_jump_ascii(sd->o_view, k);
/*
    EINA_LIST_FOREACH(sd->items, l, it)
    {
        if (it->label[0] == k || it->label[0] == k - 32)
        {
            _smart_select_item(sd, i);
            return;
        }
        i++;
    }*/
}

void
_browser_letter_show(Smart_Data *sd, const char *letter)
{
    ENNA_TIMER_DEL(sd->letter_timer);

    elm_button_label_set(sd->o_letter, letter);
    edje_object_part_text_set(sd->o_edje, "enna.text.letter", letter);
    edje_object_signal_emit(sd->o_edje, "letter,show", "enna");

    sd->letter_timer = ecore_timer_add(1.5, _letter_timer_cb, sd);

    _jump_to_ascii(sd, letter[0]);
}

void enna_browser_input_feed(Evas_Object *obj, enna_input event)
{
    API_ENTRY return;

    printf("INPUT.. to browser %d\n", event);
    if (!sd->accept_ev) return;

    edje_object_signal_callback_del(sd->o_edje, "list,transition,end", "edje",
        _list_transition_left_end_cb);
    edje_object_signal_callback_del(sd->o_edje, "list,transition,end", "edje",
        _list_transition_right_end_cb);

    switch (event)
    {
    case ENNA_INPUT_EXIT:
    case ENNA_INPUT_LEFT:
        _browse_down(sd);
        break;
    case ENNA_INPUT_OK:
    case ENNA_INPUT_RIGHT:
    {
        /* FIXME */
        _browse(sd->view_funcs.view_selected_data_get(sd->o_view));
        break;
    }
    case ENNA_INPUT_UP:
    case ENNA_INPUT_DOWN:
    case ENNA_INPUT_NEXT:
    case ENNA_INPUT_PREV:
    case ENNA_INPUT_HOME:
    case ENNA_INPUT_END:
        if (sd->view_funcs.view_key_down)
            sd->view_funcs.view_key_down(sd->o_view, event);
        break;


    case ENNA_INPUT_KEY_0: _browser_letter_show(sd, "0"); return; break;
    case ENNA_INPUT_KEY_1: _browser_letter_show(sd, "1"); return; break;
    case ENNA_INPUT_KEY_2: _browser_letter_show(sd, "2"); return; break;
    case ENNA_INPUT_KEY_3: _browser_letter_show(sd, "3"); return; break;
    case ENNA_INPUT_KEY_4: _browser_letter_show(sd, "4"); return; break;
    case ENNA_INPUT_KEY_5: _browser_letter_show(sd, "5"); return; break;
    case ENNA_INPUT_KEY_6: _browser_letter_show(sd, "6"); return; break;
    case ENNA_INPUT_KEY_7: _browser_letter_show(sd, "7"); return; break;
    case ENNA_INPUT_KEY_8: _browser_letter_show(sd, "8"); return; break;
    case ENNA_INPUT_KEY_9: _browser_letter_show(sd, "9"); return; break;

    case ENNA_INPUT_KEY_A: _browser_letter_show(sd, "a"); return; break;
    case ENNA_INPUT_KEY_B: _browser_letter_show(sd, "b"); return; break;
    case ENNA_INPUT_KEY_C: _browser_letter_show(sd, "c"); return; break;
    case ENNA_INPUT_KEY_D: _browser_letter_show(sd, "d"); return; break;
    case ENNA_INPUT_KEY_E: _browser_letter_show(sd, "e"); return; break;
    case ENNA_INPUT_KEY_F: _browser_letter_show(sd, "f"); return; break;
    case ENNA_INPUT_KEY_G: _browser_letter_show(sd, "g"); return; break;
    case ENNA_INPUT_KEY_H: _browser_letter_show(sd, "h"); return; break;
    case ENNA_INPUT_KEY_I: _browser_letter_show(sd, "i"); return; break;
    case ENNA_INPUT_KEY_J: _browser_letter_show(sd, "j"); return; break;
    case ENNA_INPUT_KEY_K: _browser_letter_show(sd, "k"); return; break;
    case ENNA_INPUT_KEY_L: _browser_letter_show(sd, "l"); return; break;
    case ENNA_INPUT_KEY_M: _browser_letter_show(sd, "m"); return; break;
    case ENNA_INPUT_KEY_N: _browser_letter_show(sd, "n"); return; break;
    case ENNA_INPUT_KEY_O: _browser_letter_show(sd, "o"); return; break;
    case ENNA_INPUT_KEY_P: _browser_letter_show(sd, "p"); return; break;
    case ENNA_INPUT_KEY_Q: _browser_letter_show(sd, "q"); return; break;
    case ENNA_INPUT_KEY_R: _browser_letter_show(sd, "r"); return; break;
    case ENNA_INPUT_KEY_S: _browser_letter_show(sd, "s"); return; break;
    case ENNA_INPUT_KEY_T: _browser_letter_show(sd, "t"); return; break;
    case ENNA_INPUT_KEY_U: _browser_letter_show(sd, "u"); return; break;
    case ENNA_INPUT_KEY_V: _browser_letter_show(sd, "v"); return; break;
    case ENNA_INPUT_KEY_W: _browser_letter_show(sd, "w"); return; break;
    case ENNA_INPUT_KEY_Z: _browser_letter_show(sd, "z"); return; break;
    
    default:
        break;
    }
}

