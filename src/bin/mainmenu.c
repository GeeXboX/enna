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

#include <Eina.h>
#include <Edje.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "mainmenu.h"
#include "content.h"
#include "input.h"
#include "logs.h"
#include "view_cover.h"

#define MAX_PER_ROW 3

typedef struct _Menu_Data Menu_Data;
typedef struct _Menu_Item Menu_Item;

struct _Menu_Data
{
    Evas_Object *o_tbl;
    Evas_Object *o_home_button;
    Evas_Object *o_back_button;
    Evas_Object *o_btn_box;
    Evas_Object *o_icon;
    Eina_List *items;
    int selected;
    unsigned char visible : 1;
    unsigned char exit_visible: 1;
};

struct _Menu_Item
{
    Menu_Data *sd;
    Evas_Object *o_base;
    Evas_Object *o_icon;
    void (*func)(void *data);
    void *data;
    Enna_Class_Activity *act;
};

/* local subsystem functions */
static void _home_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static Evas_Object *_add_button(const char *icon_name, void (*cb) (void *data, Evas_Object *obj, void *event_info));
static Eina_Bool _input_events_cb(void *data, enna_input event);
static void _item_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _item_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);


/* local subsystem globals */
static Menu_Data *sd = NULL;
static Input_Listener *listener = NULL;

/* externally accessible functions */
Evas_Object *
enna_mainmenu_add(Evas * evas)
{
    if (sd) return sd->o_tbl;

    sd = calloc(1, sizeof(Menu_Data));
    if (!sd) return NULL;
    sd->items = NULL;
    sd->visible = 0;
    sd->exit_visible = 0;
    sd->selected = 0;

    // main menu table
    //sd->o_tbl = elm_table_add(enna->layout);

    sd->o_tbl = enna_view_cover_add(evas_object_evas_get(enna->layout));
    elm_layout_content_set(enna->layout, "enna.mainmenu.swallow", sd->o_tbl);

    // button box
    sd->o_btn_box = elm_box_add(enna->win);
    elm_box_homogenous_set(sd->o_btn_box, 0);
    elm_box_horizontal_set(sd->o_btn_box, 1);
    evas_object_size_hint_align_set(sd->o_btn_box, -1.0, -1.0);
    evas_object_size_hint_weight_set(sd->o_btn_box, 1.0, 1.0);
    evas_object_show(sd->o_btn_box);
    elm_layout_content_set(enna->layout, "titlebar.swallow.button", sd->o_btn_box);
    
    sd->o_home_button = _add_button("icon/home_mini", _home_button_clicked_cb);
    elm_box_pack_start(sd->o_btn_box, sd->o_home_button);

    sd->o_back_button = _add_button("icon/arrow_left", _back_button_clicked_cb);
    elm_box_pack_end(sd->o_btn_box, sd->o_back_button);

    // connect to the input signal
    listener = enna_input_listener_add("mainmenu", _input_events_cb, NULL);

    return sd->o_tbl;
}

void
enna_mainmenu_shutdown(void)
{
    Menu_Item *it;

    enna_input_listener_del(listener);

    EINA_LIST_FREE(sd->items, it)
    {
        ENNA_OBJECT_DEL(it->o_base);
        ENNA_OBJECT_DEL(it->o_icon);
        ENNA_FREE(it);
    }
    ENNA_OBJECT_DEL(sd->o_home_button);
    ENNA_OBJECT_DEL(sd->o_back_button);
    ENNA_OBJECT_DEL(sd->o_tbl);
    ENNA_OBJECT_DEL(sd->o_btn_box);
    ENNA_FREE(sd);
}

void
enna_mainmenu_append(const char *icon, const char *label,
                Enna_Class_Activity *act, void (*func) (void *data), void *data)
{
    Menu_Item *it;
    static int i = 0;
    static int j = 0;
    Enna_Vfs_File *f;

    it = malloc(sizeof(Menu_Item));
    if (!it) return;
    it->act = act;
    it->o_base = edje_object_add(enna->evas);
    edje_object_file_set(it->o_base, enna_config_theme_get(), "enna/mainmenu/item");

    // label
    if (label)
        edje_object_part_text_set(it->o_base, "enna.text.label", gettext(label));

    // icon
    if (icon)
    {
        Evas_Object *ic;

        ic = elm_icon_add(enna->layout);
        elm_icon_file_set(ic, enna_config_theme_get(), icon);
        evas_object_size_hint_weight_set(ic, 1.0, 1.0);
        evas_object_show(ic);
        it->o_icon = ic;
        if (ic)
            edje_object_part_swallow(it->o_base, "enna.swallow.content", ic);
    }

    // item data
    it->func = func;
    it->data = data;
    sd->items = eina_list_append(sd->items, it);

    evas_object_size_hint_weight_set(it->o_base, 1.0, 1.0);
    evas_object_size_hint_align_set(it->o_base, -1.0, -1.0);

    f = calloc(1, sizeof(Enna_Vfs_File));
    f->label = eina_stringshare_add(label);
    f->icon = eina_stringshare_add(icon);
    enna_view_cover_file_append(sd->o_tbl, f, func, data);
    //elm_table_pack(sd->o_tbl, it->o_base, i, j, 1, 1);

    evas_object_event_callback_add(it->o_base, EVAS_CALLBACK_MOUSE_UP,
                                   _item_event_mouse_up, it);
    evas_object_event_callback_add(it->o_base, EVAS_CALLBACK_MOUSE_DOWN,
                                   _item_event_mouse_down, it);
    evas_object_show(it->o_base);


    // FIXME : Ugly !
    i++;
    if (i == MAX_PER_ROW)
    {
        j++;
        i = 0;
    }
}

static void
_activate (void *data)
{
    Enna_Class_Activity *act = data;
    printf("activate %s", act->label);
}


void
enna_mainmenu_load_from_activities(void)
{
    Eina_List *activities, *l;
    Enna_Class_Activity *act;

    activities = enna_activities_get();

    EINA_LIST_FOREACH(activities, l, act)
        enna_mainmenu_append(act->icon ? act->icon : act->icon_file,
                             act->label, act, _activate, act);
}

void
enna_mainmenu_activate(Menu_Item *it)
{
    // hide the mainmenu
    enna_mainmenu_hide();
    edje_object_part_text_set(elm_layout_edje_get(enna->layout),
                              "titlebar.text.label", "");

    // update icon
    ENNA_OBJECT_DEL(sd->o_icon)
    sd->o_icon = elm_icon_add(enna->layout);
    elm_icon_file_set(sd->o_icon, enna_config_theme_get(), it->act->icon);
    evas_object_show(sd->o_icon);
    elm_layout_content_set(enna->layout, "titlebar.swallow.icon", sd->o_icon);

    // run the activity_show cb. that is responsable of showing the
    // content using enna_content_select("name")
    enna_activity_show(it->act->name);
}

void
enna_mainmenu_activate_nth(int nth)
{
    Menu_Item *it;

    if (!sd) return;

    it = eina_list_nth(sd->items, nth);
    if (!it) return;

    enna_mainmenu_activate(it);
}

static int
enna_mainmenu_get_nr_items()
{
    if (!sd) return 0;

    return eina_list_count(sd->items);
}

int
enna_mainmenu_selected_get(void)
{
    if (!sd) return 0;
    return sd->selected;
}

void
enna_mainmenu_select_nth(int nth)
{
    Menu_Item *new, *prev;

    if (!sd) return;

    prev = eina_list_nth(sd->items, sd->selected);
    if (!prev) return;

    new = eina_list_nth(sd->items, nth);
    if (!new) return;

    sd->selected = nth;
    edje_object_signal_emit(new->o_base, "select", "enna");
    if (new != prev)
        edje_object_signal_emit(prev->o_base, "unselect", "enna");
}

Enna_Class_Activity *
enna_mainmenu_selected_activity_get(void)
{
    Menu_Item *it;

    if (!sd) return NULL;

    it = eina_list_nth(sd->items, sd->selected);
    if (!it) return NULL;

    return it->act;
}

static void
enna_mainmenu_select_sibbling (int way)
{
    Menu_Item *new, *prev;
    int ns = 0;

    if (!sd) return;

    ns = sd->selected;
    prev = eina_list_nth(sd->items, ns);
    if (!prev)
        return;
    if (way) ns--; else ns++;
    new = eina_list_nth(sd->items, ns);
    if (!new)
    {
        ns = way ? eina_list_count(sd->items) - 1 : 0;
        new = eina_list_nth(sd->items, ns);
        if (!new)
            return;
    }
    sd->selected = ns;
    edje_object_signal_emit(new->o_base, "select", "enna");
    edje_object_signal_emit(prev->o_base, "unselect", "enna");
}

void
enna_mainmenu_select_next(void)
{
    enna_mainmenu_select_sibbling (0);
}

void
enna_mainmenu_select_prev(void)
{
    enna_mainmenu_select_sibbling (1);
}


static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    int el, n;
    if (!sd) return ENNA_EVENT_CONTINUE;

    if (event == ENNA_INPUT_FULLSCREEN)
    {
        enna->run_fullscreen = ~enna->run_fullscreen;
        elm_win_fullscreen_set(enna->win, enna->run_fullscreen);
        return ENNA_EVENT_BLOCK;
    }

    if (sd->visible)
    {
        switch (event)
        {
            case ENNA_INPUT_RIGHT:
            case ENNA_INPUT_LEFT:
            case ENNA_INPUT_UP:
            case ENNA_INPUT_DOWN:
            case ENNA_INPUT_OK:
                enna_view_cover_input_feed(sd->o_tbl, event);
                return ENNA_EVENT_BLOCK;
                break;
            default:
                break;
        }
    }
    else if (event == ENNA_INPUT_MENU)
    {
        enna_content_hide();
        enna_mainmenu_show();
        return ENNA_EVENT_BLOCK;
    }
    if (!sd->visible)
    {
        enna_activity_event(enna_mainmenu_selected_activity_get(), event);
    }

    return ENNA_EVENT_CONTINUE;
}

void
enna_mainmenu_show(void)
{
    if (!sd) return;
    sd->visible = 1;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,show", "enna");
    edje_object_part_text_set(elm_layout_edje_get(enna->layout),
                              "titlebar.text.label", "enna");
    ENNA_OBJECT_DEL(sd->o_icon);
}

void
enna_mainmenu_hide(void)
{
    if (!sd) return;
    sd->visible = 0;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "mainmenu,hide", "enna");
}

unsigned char
enna_mainmenu_visible(void)
{
    if (!sd) return 0;
    return sd->visible;
}

unsigned char
enna_mainmenu_exit_visible(void)
{
    if (!sd) return 0;
    return sd->exit_visible;
}

/* local subsystem functions */
static void
_home_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_input_event_emit(ENNA_INPUT_MENU);
}

static void
_back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    enna_input_event_emit(ENNA_INPUT_EXIT);
}

static Evas_Object *
_add_button(const char *icon_name, void (*cb) (void *data, Evas_Object *obj, void *event_info))
{
    Evas_Object *ic, *bt;
    
    ic = elm_icon_add(enna->layout);
    elm_icon_file_set(ic, enna_config_theme_get(), icon_name);
    evas_object_show(ic);
    
    bt = elm_button_add(enna->layout);
    evas_object_smart_callback_add(bt, "clicked", cb, sd);
    elm_button_icon_set(bt, ic);
    evas_object_size_hint_weight_set(bt, 1.0, 1.0);
    //~ evas_object_size_hint_align_set(bt, -1.0, -1.0);
    evas_object_size_hint_min_set(bt, 55, 55);
    evas_object_show(bt);

    return bt;
}

static void
_item_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Up *ev;
    Menu_Item *it;
    Eina_List *l;
    int i;

    ev = event_info;
    it = data;

    if (!sd->items)
        return;

    for (l = sd->items, i = 0; l; l = l->next, i++)
    {
        if (l->data == it)
        {
            enna_mainmenu_activate(it);
            break;
        }
    }
}

static void
_item_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Down *ev;
    Menu_Item *it;
    Menu_Item *prev;
    Eina_List *l = NULL;
    int i;

    ev = event_info;
    it = data;

    if (!sd->items) return;

    prev = eina_list_nth(sd->items, sd->selected);
    for (i = 0, l = sd->items; l; l = l->next, i++)
    {
        if (l->data == it)
        {
            edje_object_signal_emit(it->o_base, "select", "enna");
            edje_object_signal_emit(prev->o_base, "unselect", "enna");
            sd->selected = i;
            break;
        }
    }
}

