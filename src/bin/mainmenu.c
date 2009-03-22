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
#include "event_key.h"

#define SMART_NAME "enna_mainmenu"
#define API_ENTRY						\
    Smart_Data *sd;						\
    sd = evas_object_smart_data_get(obj);			\
    if ((!obj) || (!sd) || (evas_object_type_get(obj)		\
	    && strcmp(evas_object_type_get(obj), SMART_NAME)))	\

#define MAX_PER_ROW 4

typedef struct _Smart_Data Smart_Data;
typedef struct _Smart_Item Smart_Item;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_smart;
    Evas_Object *o_edje;
    Evas_Object *o_tbl;
    Evas_Object *o_home_button;
    Evas_Object *o_back_button;
    Evas_Object *o_btn_box;
    Evas_Object *o_quitdiag_box;
    Evas_Object *o_quitdiag_yes_button;
    Evas_Object *o_quitdiag_no_button;
    Eina_List *items;
    int selected;
    unsigned char visible : 1;
};

struct _Smart_Item
{
    Smart_Data *sd;
    Evas_Object *o_base;
    Evas_Object *o_icon;
    void (*func)(void *data);
    void *data;
    unsigned char selected : 1;
    Enna_Class_Activity *act;
};

/* local subsystem functions */
static void _home_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _quitdiag_yes_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _quitdiag_no_clicked_cb(void *data, Evas_Object *obj, void *event_info);
static void _smart_activate_cb (void *data);
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
static void _smart_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
enna_mainmenu_add(Evas * evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _e_smart);
}

void enna_mainmenu_append(Evas_Object *obj, const char *icon,
        const char *label, Enna_Class_Activity *act, void (*func) (void *data), void *data)
{
    Smart_Item *si;
    Evas_Object *ic;
    static int i = 0;
    static int j = 0;

    API_ENTRY return;

    if (!act)
        return;

    si = malloc(sizeof(Smart_Item));
    si->sd = sd;
    si->act = act;
    si->o_base = edje_object_add(evas_object_evas_get(sd->o_edje));
    edje_object_file_set(si->o_base, enna_config_theme_get(),
	"enna/mainmenu/item");

    if (label)
	edje_object_part_text_set(si->o_base, "enna.text.label", label);

    ic = elm_icon_add(obj);
    elm_icon_file_set(ic, enna_config_theme_get(), icon);
    evas_object_size_hint_weight_set(ic, 1.0, 1.0);
    elm_icon_scale_set(ic, 0, 0);
    evas_object_show(ic);
    si->o_icon = ic;
    if (ic)
        edje_object_part_swallow(si->o_base, "enna.swallow.content", ic);

    si->func = func;
    si->data = data;
    si->selected = 0;
    sd->items = eina_list_append(sd->items, si);

    evas_object_show(si->o_base);

    evas_object_size_hint_weight_set(si->o_base, 1.0, 1.0);
    evas_object_size_hint_align_set(si->o_base, -1.0, -1.0);

    elm_table_pack(sd->o_tbl, si->o_base, i, j, 1, 1);


    evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
	_smart_event_mouse_up, si);
    evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
	_smart_event_mouse_down, si);
    evas_object_show(si->o_base);

    // FIXME : Ugly !
    i++;
    if (i == MAX_PER_ROW)
    {
	j++;
	i = 0;
    }

}

void enna_mainmenu_load_from_activities(Evas_Object *obj)
{
    Eina_List *activities, *l;
    API_ENTRY return;

    activities = enna_activities_get();

    for (l = activities; l; l = l->next)
    {
        Enna_Class_Activity *act;
	const char *icon_name = NULL;
        act = l->data;

        if (act->icon)
        {
	    icon_name = eina_stringshare_add(act->icon);
        }
        else if (act->icon_file)
        {

	    icon_name = eina_stringshare_add(act->icon_file);
        }
        enna_mainmenu_append(obj, icon_name, act->name, act, _smart_activate_cb, act);
    }

}

void enna_mainmenu_activate_nth(Evas_Object *obj, int nth)
{
    Smart_Item *si;
    API_ENTRY
	return;

    si = eina_list_nth(sd->items, nth);
    if (!si)
        return;
    if (si->func)
    {

        Evas_Object *icon;
        si->func(si->data);
        /* Unswallow and delete previous icons */
        icon = edje_object_part_swallow_get(sd->o_edje, "titlebar.swallow.icon");
        edje_object_part_unswallow(sd->o_edje, icon);
        ENNA_OBJECT_DEL(icon);

        edje_object_part_text_set(sd->o_edje, "titlebar.text.label", si->act->name);
        icon = edje_object_add(evas_object_evas_get(sd->o_edje));
        edje_object_file_set(icon, enna_config_theme_get(),si->act->icon);
        edje_object_part_swallow(sd->o_edje, "titlebar.swallow.icon", icon);
        enna_mainmenu_hide(obj);
    }
}

static int enna_mainmenu_get_nr_items(Evas_Object *obj)
{
    API_ENTRY
	return 0;

    return eina_list_count(sd->items);
}

int enna_mainmenu_selected_get(Evas_Object *obj)
{
    API_ENTRY return -1;
    return sd->selected;
}

void enna_mainmenu_select_nth(Evas_Object *obj, int nth)
{
    Smart_Item *new, *prev;

    API_ENTRY return;

    prev = eina_list_nth(sd->items, sd->selected);

    if (!prev)
        return;

    new = eina_list_nth(sd->items, nth);
    if (!new)
        return;

    sd->selected = nth;
    edje_object_signal_emit(new->o_base, "select", "enna");
    if (new != prev)
        edje_object_signal_emit(prev->o_base, "unselect", "enna");

}

Enna_Class_Activity *enna_mainmenu_selected_activity_get(Evas_Object *obj)
{
    Smart_Item *si;
    API_ENTRY return NULL;

    si = eina_list_nth(sd->items, sd->selected);

    if (!si)
	return NULL;

    return si->act;
}

static void enna_mainmenu_select_sibbling (Evas_Object *obj, int way)
{
    Smart_Item *new, *prev;
    int ns = 0;

    API_ENTRY return;

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

void enna_mainmenu_select_next(Evas_Object *obj)
{
    enna_mainmenu_select_sibbling (obj, 0);
}

void enna_mainmenu_select_prev(Evas_Object *obj)
{
    enna_mainmenu_select_sibbling (obj, 1);
}

void enna_mainmenu_event_feed(Evas_Object *obj, void *event_info)
{
    API_ENTRY return;

    enna_key_t key;
    int n, el;

    key = enna_get_key(event_info);
    switch (key)
    {

    case ENNA_KEY_RIGHT:
	enna_mainmenu_select_next(obj);
	break;
    case ENNA_KEY_LEFT:
	enna_mainmenu_select_prev(obj);
	break;
    case ENNA_KEY_UP:
        el = enna_mainmenu_selected_get(obj);
        enna_mainmenu_select_nth(obj, el - MAX_PER_ROW);
        break;
    case ENNA_KEY_DOWN:
        n = enna_mainmenu_get_nr_items(obj);
        el = enna_mainmenu_selected_get(obj);
        /* go to element below or last one of row if none */
        enna_mainmenu_select_nth(obj, (el + MAX_PER_ROW >= n) ?
                                 n - 1 : el + MAX_PER_ROW);
        break;
    default:
	break;
    }

}

void enna_mainmenu_show(Evas_Object *obj)
{
    Evas_Object *icon;

    API_ENTRY return;
    if (sd->visible)
        return;

    sd->visible = 1;
    edje_object_signal_emit(sd->o_edje, "mainmenu,show", "enna");

    /* Unswallow and delete previous icons */
    icon = edje_object_part_swallow_get(sd->o_edje, "titlebar.swallow.icon");
    edje_object_part_unswallow(sd->o_edje, icon);
    ENNA_OBJECT_DEL(icon);

    edje_object_part_text_set(sd->o_edje, "titlebar.text.label", "Enna");

}

void enna_mainmenu_hide(Evas_Object *obj)
{
    API_ENTRY return;
    if (!sd->visible)
        return;

    sd->visible = 0;
    edje_object_signal_emit(sd->o_edje, "mainmenu,hide", "enna");
}

unsigned char enna_mainmenu_visible(Evas_Object *obj)
{
    API_ENTRY return 0;
    return sd->visible;
}

unsigned char enna_quitdiag_visible(Evas_Object *obj)
{
    API_ENTRY return 0;
	return evas_object_visible_get(sd->o_quitdiag_box);
}

/* local subsystem globals */

static void _home_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    evas_event_feed_key_down(enna->evas, "Super_L", "Super_L", "Super_L", NULL, ecore_time_get(), data);
}

static void _back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{

    evas_event_feed_key_down(enna->evas, "BackSpace", "BackSpace", "BackSpace", NULL, ecore_time_get(), data);
}

static void _quitdiag_yes_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	enna->do_quit = 1;
	enna_log(ENNA_MSG_WARNING, "quitdiag button clicked", "button: yes");
    evas_event_feed_key_down(enna->evas, "Escape", "Escape", "Escape", NULL, ecore_time_get(), data);
}

static void _quitdiag_no_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Smart_Data* sd = data;
	enna->do_quit = 0;
	edje_object_signal_emit (sd->o_edje, "quitdiag,hide", "enna");
	enna_log(ENNA_MSG_WARNING, "quitdiag button clicked", "button: no");
}

static void _smart_activate_cb(void *data)
{
    Enna_Class_Activity *act;

    if (!data)
        return;
    act = data;

    enna_content_select(act->name);

}

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

static void _smart_init(void)
{
    if (_e_smart)
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
    _e_smart = evas_smart_class_new(&sc);
}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;
    Evas_Object *o, *ic, *bt, *bx;
    Evas *e;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;

    e = evas_object_evas_get(obj);

    sd->o_edje = evas_object_image_add(e);
    sd->x = 0;
    sd->y = 0;
    sd->w = 0;
    sd->h = 0;

    sd->items = NULL;
    sd->visible = 0;
    sd->selected = 0;
    o = edje_object_add(e);
    edje_object_file_set(o, enna_config_theme_get(), "enna/mainmenu");
    sd->o_edje = o;
    evas_object_show(o);

    o = elm_table_add(obj);
    sd->o_tbl = o;
    edje_object_part_swallow(sd->o_edje, "enna.swallow.box", sd->o_tbl);

    sd->o_btn_box = elm_box_add(obj);
    elm_box_homogenous_set(sd->o_btn_box, 0);
    elm_box_horizontal_set(sd->o_btn_box, 1);
    evas_object_size_hint_align_set(sd->o_btn_box, 0, 0.5);
    evas_object_size_hint_weight_set(sd->o_btn_box, 1.0, 1.0);
    edje_object_part_swallow(sd->o_edje, "titlebar.swallow.button", sd->o_btn_box);

    ic = elm_icon_add(obj);
    elm_icon_file_set(ic, enna_config_theme_get(), "icon/home_mini");
    elm_icon_scale_set(ic, 0, 0);
    bt = elm_button_add(obj);
    evas_object_smart_callback_add(bt, "clicked", _home_button_clicked_cb, sd);
    elm_button_icon_set(bt, ic);
    elm_box_pack_end(sd->o_btn_box, bt);
    evas_object_show(bt);
    evas_object_show(ic);

    ic = elm_icon_add(obj);
    elm_icon_file_set(ic, enna_config_theme_get(), "icon/arrow_left");
    elm_icon_scale_set(ic, 0, 0);
    bt = elm_button_add(obj);
    evas_object_smart_callback_add(bt, "clicked", _back_button_clicked_cb, sd);
    elm_button_icon_set(bt, ic);
    elm_box_pack_end(sd->o_btn_box, bt);
    evas_object_show(bt);
    evas_object_show(ic);

	/* Add Quit Dialog */
    bx = elm_box_add(obj);
	sd->o_quitdiag_box = bx;
    elm_box_horizontal_set(bx, 1);
    edje_object_part_swallow(sd->o_edje, "enna.quitdiag.buttonbox", bx);
	evas_object_show(bx);

	bt = elm_button_add(sd->o_edje);
	sd->o_quitdiag_yes_button=bt;
    evas_object_smart_callback_add(bt, "clicked", _quitdiag_yes_clicked_cb, sd);
	elm_button_label_set(bt, "(Y)es, I want to quit.");
	ic = elm_icon_add(sd->o_edje);
	elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_stop");
	elm_button_icon_set(bt, ic);
	elm_object_scale_set(bt, 2.0);
	elm_box_pack_end(sd->o_quitdiag_box, bt);
	evas_object_show(bt);

	bt = elm_button_add(sd->o_edje);
	sd->o_quitdiag_no_button=bt;
    evas_object_smart_callback_add(bt, "clicked", _quitdiag_no_clicked_cb, sd);
	elm_button_label_set(bt, "(N)o, I want to stay.");
	ic = elm_icon_add(sd->o_edje);
	elm_icon_file_set(ic, enna_config_theme_get(), "icon/mp_play");
	elm_button_icon_set(bt, ic);
	elm_object_scale_set(bt, 2.0);
	elm_box_pack_end(sd->o_quitdiag_box, bt);
	evas_object_show(bt);
	evas_object_show(sd->o_quitdiag_box);

    sd->o_smart = obj;
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    Smart_Data *sd;
    Eina_List *l;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;

    for (l = sd->items; l; l = l->next)
    {
        Smart_Item *si;
        si = l->data;
        evas_object_del(si->o_base);
        evas_object_del(si->o_icon);
    }
    eina_list_free(sd->items);
    evas_object_del(sd->o_edje);
    evas_object_del(sd->o_tbl);
    evas_object_del(sd->o_btn_box);
    evas_object_del(sd->o_quitdiag_box);
    evas_object_del(sd->o_quitdiag_yes_button);
    evas_object_del(sd->o_quitdiag_no_button);
    free(sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;

    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_hide(sd->o_edje);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd)
        return;
    evas_object_clip_unset(sd->o_edje);
}

static void _smart_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd;
    Evas_Event_Mouse_Up *ev;
    Smart_Item *si;
    Eina_List *l;
    int i;

    ev = event_info;
    si = data;
    sd = si->sd;

    if (!sd->items)
        return;

    for (l = sd->items, i = 0; l; l = l->next, i++)
    {
        if (l->data == si)
        {
            enna_mainmenu_activate_nth(sd->o_smart, i);
            break;
        }
    }
}

static void _smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd;
    Evas_Event_Mouse_Down *ev;
    Smart_Item *si;
    Smart_Item *prev;
    Eina_List *l = NULL;
    int i;

    ev = event_info;
    si = data;
    sd = si->sd;

    if (!sd->items)
        return;
    prev = eina_list_nth(sd->items, sd->selected);

    for (i = 0, l = sd->items; l; l = l->next, i++)
    {
        if (l->data == si)
        {
            edje_object_signal_emit(si->o_base, "select", "enna");
            edje_object_signal_emit(prev->o_base, "unselect",
		"enna");
            sd->selected = i;
            break;
        }
    }

}

void enna_mainmenu_quitdiag(Evas_Object *obj)
{
	char msg[50];
	char quitdiag_active;
    API_ENTRY return;

	quitdiag_active=evas_object_visible_get(sd->o_quitdiag_box);
	strncpy(msg, quitdiag_active?"quitdiag,hide":"quitdiag,show", sizeof(msg));
	edje_object_signal_emit (sd->o_edje, msg, "enna");
	quitdiag_active=!quitdiag_active;
}

