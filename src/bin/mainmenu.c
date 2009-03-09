/*
 * enna_mainmenu.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna_mainmenu.c is free software copyrighted by Nicolas Aguirre.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Nicolas Aguirre'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * enna_mainmenu.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Nicolas Aguirre OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* derived from e_icon */

#include "mainmenu.h"
#include "enna.h"

#define SMART_NAME "enna_mainmenu"
#define API_ENTRY						\
    Smart_Data *sd;						\
    sd = evas_object_smart_data_get(obj);			\
    if ((!obj) || (!sd) || (evas_object_type_get(obj)		\
	    && strcmp(evas_object_type_get(obj), SMART_NAME)))	\

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
    if (i > 3)
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


void enna_mainmenu_select_next(Evas_Object *obj)
{
    Smart_Item *new, *prev;
    int ns = 0;

    API_ENTRY return;

    ns = sd->selected;
    prev = eina_list_nth(sd->items, ns);
    if (!prev)
        return;
    ns++;
    new = eina_list_nth(sd->items, ns);
    if (!new)
    {
        ns = 0;
        new = eina_list_nth(sd->items, ns);
        if (!new)
            return;
    }
    sd->selected = ns;
    edje_object_signal_emit(new->o_base, "select", "enna");
    edje_object_signal_emit(prev->o_base, "unselect", "enna");

}

void enna_mainmenu_select_prev(Evas_Object *obj)
{
    Smart_Item *new, *prev;
    int ns = 0;

    API_ENTRY return;

    ns = sd->selected;
    prev = eina_list_nth(sd->items, ns);
    if (!prev)
        return;
    ns--;
    new = eina_list_nth(sd->items, ns);
    if (!new)
    {
        ns = eina_list_count(sd->items)-1;
        new = eina_list_nth(sd->items, ns);
        if (!new)
            return;
    }
    sd->selected = ns;
    edje_object_signal_emit(new->o_base, "select", "enna");
    edje_object_signal_emit(prev->o_base, "unselect", "enna");

}

void enna_mainmenu_event_feed(Evas_Object *obj, void *event_info)
{
    API_ENTRY return;

    enna_key_t key;

    key = enna_get_key(event_info);
    switch (key)
    {

    case ENNA_KEY_RIGHT:
	enna_mainmenu_select_next(obj);
	break;
	enna_mainmenu_select_prev(obj);
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

/* local subsystem globals */

static void _home_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    evas_event_feed_key_down(enna->evas, "Super_L", "Super_L", "Super_L", NULL, ecore_time_get(), data);
}

static void _back_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{

    evas_event_feed_key_down(enna->evas, "BackSpace", "BackSpace", "BackSpace", NULL, ecore_time_get(), data);
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
    Evas_Object *o, *ic, *bt;
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
