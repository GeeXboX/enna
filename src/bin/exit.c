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

/* TODO : remove smart object and use directly elm objects */

#include "Elementary.h"

#include "enna.h"
#include "enna_config.h"
#include "view_list.h"
#include "buffer.h"
#include "activity.h"
#include "input.h"
#include "exit.h"

#define SMART_NAME "enna_exit"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *layout;
    Evas_Object *list;
    Evas_Object *label;
    Input_Listener *listener;
    Eina_Bool visible: 1;
};

/* local subsystem globals */

static void
_yes_cb(void *data)
{
    ecore_main_loop_quit();
}

static void
_no_cb(void *data)
{
    Evas_Object *obj = data;

    enna_exit_hide(obj);
}

static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    Evas_Object *obj = data;
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (event == ENNA_INPUT_QUIT)
    {
        if (sd->visible)
            enna_exit_hide(obj);
        else
            enna_exit_show(obj);
        return ENNA_EVENT_BLOCK;
    }
    if (sd->visible)
    {
        enna_list_input_feed(sd->list, event);
        return ENNA_EVENT_BLOCK;
    }
    return ENNA_EVENT_CONTINUE;
}

static void
_update_text(Evas_Object *lb)
{
    buffer_t *label;
    const char *tmp;

    label = buffer_new();
    buffer_append(label, "<h3><c>");
    buffer_append(label, _("Are you sure you want to quit enna ?"));
    buffer_append(label, "</c></h3><br>");
    tmp =  enna_activity_request_quit_all();

    if (tmp) buffer_appendf(label, "<h2>%s<h2>", tmp);

    elm_label_label_set(lb, label->buf);
    buffer_free(label);
}

static Enna_Vfs_File *
_create_list_item (char *label, char *icon)
{
    Enna_Vfs_File *it;

    it = calloc (1, sizeof (Enna_Vfs_File));
    it->label = (char*)eina_stringshare_add (label);
    it->icon = (char*)eina_stringshare_add (icon);

    return it;
}

static void
_sd_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    ENNA_OBJECT_DEL(sd->label);
    ENNA_OBJECT_DEL(sd->list);
    ENNA_OBJECT_DEL(sd->layout);
    ENNA_FREE(sd);
}

/* externally accessible functions */
Evas_Object *
enna_exit_add(Evas * evas)
{
    Evas_Object *obj;
    Smart_Data *sd;
    Enna_Vfs_File *it1, *it2;

    sd = calloc(1, sizeof(Smart_Data));

    obj = elm_win_inwin_add(enna->win);
    elm_object_style_set(obj, "enna");

    sd->layout = elm_layout_add(obj);
    elm_layout_file_set(sd->layout, enna_config_theme_get(), "exit_layout");
    evas_object_size_hint_weight_set(sd->layout, 1.0, 1.0);
    evas_object_show(sd->layout);

    sd->label = elm_label_add(sd->layout);
    _update_text(sd->label);

    sd->list = enna_list_add(enna->evas);
    it1 = _create_list_item (_("Yes, Quit Enna"), "ctrl/shutdown");
    enna_list_file_append(sd->list, it1, _yes_cb, obj);

    it2 = _create_list_item (_("No, Continue using enna"), "ctrl/hibernate");
    enna_list_file_append(sd->list, it2, _no_cb, obj);
    enna_list_select_nth(sd->list, 0);
/*
    enna_list2_append(list, _("Yes, Quit Enna"), NULL,  "ctrl/shutdown",
                      _yes_cb, obj);

    enna_list2_append(list, _("No, Continue using enna"), NULL,  "ctrl/hibernate",
                      _no_cb, obj);
*/
    elm_layout_content_set(sd->layout, "enna.content.swallow",
                           sd->list);
    elm_layout_content_set(sd->layout, "enna.label.swallow",
                           sd->label);

    elm_win_inwin_content_set(obj, sd->layout);
    elm_win_inwin_activate(obj);

    evas_object_data_set(obj, "sd", sd);
    /* connect to the input signal */

    sd->listener = enna_input_listener_add("exit_dialog", _input_events_cb, obj);
    enna_input_listener_demote(sd->listener);

    evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL,
                                   _sd_del, sd);
    //  sd->label = lb;
    return obj;
}

void
enna_exit_show(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    if (sd->visible)
        enna_exit_update_text(obj);
    enna_list_select_nth(sd->list, 0);
    sd->visible = EINA_TRUE;
    edje_object_signal_emit(elm_layout_edje_get(enna->layout), "exit,show", "enna");
    enna_input_listener_promote(sd->listener);
}

void
enna_exit_hide(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    sd->visible = EINA_FALSE;
    enna_input_listener_demote(sd->listener);
    edje_object_signal_emit(elm_layout_edje_get(enna->layout), "exit,hide", "enna");
}

void
enna_exit_update_text(Evas_Object *obj)
{
    Smart_Data *sd = evas_object_data_get(obj, "sd");

    _update_text(sd->label);
}

