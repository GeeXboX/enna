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

/* TODO : remove smart object and use directly elm objects */

#include "Elementary.h"

#include "enna.h"
#include "enna_config.h"
#include "box.h"
#include "buffer.h"
#include "activity.h"
#include "input.h"
#include "exit.h"

#define SMART_NAME "enna_exit"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Object *inwin;
    Evas_Object *layout;
    Evas_Object *list;
    Evas_Object *label;
    Input_Listener *listener;
    Eina_Bool visible: 1;
};

static Smart_Data *sd = NULL;
static int _exit_init_count = -1;

/* local subsystem globals */

static void
_inwin_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;

    ENNA_OBJECT_DEL(sd->label);
    ENNA_OBJECT_DEL(sd->list);
    ENNA_OBJECT_DEL(sd->layout);
}

static void
_update_text(Evas_Object *lb)
{
    buffer_t *label;
    const char *tmp;

    label = buffer_new();
    buffer_append(label, "<h3><c>");
    buffer_append(label, _("Are you sure you want to quit Enna?"));
    buffer_append(label, "</c></h3><br>");
    tmp =  enna_activity_request_quit_all();

    if (tmp) buffer_appendf(label, "<h2>%s<h2>", tmp);

    elm_label_label_set(lb, label->buf);
    buffer_free(label);
}

static void
_inwin_hide()
{
    ENNA_OBJECT_DEL(sd->inwin);
    sd->visible = EINA_FALSE;
    enna_input_listener_demote(sd->listener);
}

static void
_yes_cb(void *data)
{
    ecore_main_loop_quit();
}

static void
_no_cb(void *data)
{
    _inwin_hide();
}

static void
_inwin_add()
{
    ENNA_OBJECT_DEL(sd->inwin);

    sd->inwin = elm_win_inwin_add(enna->win);
    elm_object_style_set(sd->inwin, "enna");

    sd->layout = elm_layout_add(sd->inwin);
    elm_layout_file_set(sd->layout, enna_config_theme_get(), "enna/exit/layout");
    evas_object_size_hint_weight_set(sd->layout, 1.0, 1.0);
    evas_object_show(sd->layout);

    sd->label = elm_label_add(sd->layout);
    elm_object_style_set(sd->label, "enna");
    _update_text(sd->label);

    sd->list =  enna_box_add(enna->win, "exit");

    enna_box_append(sd->list, _("Yes"), _("quit Enna"), "ctrl/shutdown", _yes_cb, sd);
    enna_box_append(sd->list, _("No"), _("continue using Enna"), "ctrl/hibernate", _no_cb, sd);
    enna_box_select_nth(sd->list, 0);

    elm_layout_content_set(sd->layout, "enna.content.swallow",
                           sd->list);
    elm_layout_content_set(sd->layout, "enna.label.swallow",
                           sd->label);

    elm_win_inwin_content_set(sd->inwin, sd->layout);
    elm_win_inwin_activate(sd->inwin);

    /* connect to the input signal */

    evas_object_event_callback_add(sd->inwin, EVAS_CALLBACK_DEL,
                                   _inwin_del_cb, sd);
}

static void
_inwin_show()
{
    _inwin_add();

    if (sd->visible)
        enna_exit_update_text();
    enna_box_select_nth(sd->list, 0);
    sd->visible = EINA_TRUE;
    enna_input_listener_promote(sd->listener);
}

static Eina_Bool
_input_events_cb(void *data, enna_input event)
{
    Smart_Data *sd = data;

    if (event == ENNA_INPUT_QUIT)
    {
        if (sd->visible)
            _inwin_hide();
        else
            _inwin_show();
        return ENNA_EVENT_BLOCK;
    }
    if (sd->visible)
    {
        enna_box_input_feed(sd->list, event);
        return ENNA_EVENT_BLOCK;
    }
    return ENNA_EVENT_CONTINUE;
}



/* externally accessible functions */

int 
enna_exit_visible()
{
  return sd->visible;
}

int
enna_exit_init()
{
     /* Prevent multiple loads */
    if (_exit_init_count > 0)
        return ++_exit_init_count;

    sd = calloc(1, sizeof(Smart_Data));

    /* Add input listener at the lowest level */
    sd->listener = enna_input_listener_add("exit_dialog", _input_events_cb, sd);
    enna_input_listener_demote(sd->listener);

    _exit_init_count = 1;
    return 1;
}

int
enna_exit_shutdown()
{
    /* Shutdown only when all clients have called this function */
    _exit_init_count--;
    if (_exit_init_count == 0)
    {
        ENNA_OBJECT_DEL(sd->inwin);
        enna_input_listener_del(sd->listener);
        ENNA_FREE(sd);
    }
    return _exit_init_count;
}

void
enna_exit_update_text()
{
    _update_text(sd->label);
}
