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

#include "enna.h"
#include "enna_config.h"
#include "utils.h"
#include "mediaplayer.h"
#include "volume_notification.h"

#define VOLUME_TIMER_VALUE 3

typedef struct volume_notifier_sd_s {
    Evas_Object *edje;
    Ecore_Timer *timer;
} volume_notifier_smart_data_t;

static void
cb_del (void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    volume_notifier_smart_data_t *sd = data;

    ENNA_TIMER_DEL(sd->timer);
    ENNA_FREE(sd);
}

static void
volume_notification_update (Evas_Object *obj)
{
    volume_notifier_smart_data_t *sd;
    char txt[16] = { 0 };

    if (!obj)
        return;

    sd = evas_object_data_get(obj, "sd");

    if (enna_mediaplayer_mute_get())
        snprintf(txt, sizeof(txt), "%s", _("Mute"));
    else
    {
        int vol = enna_mediaplayer_volume_get();
        if (vol <= 0)
            snprintf(txt, sizeof(txt), "%s", _("Mute"));
        else
            snprintf(txt, sizeof(txt), "%d %%", vol);
    }

    edje_object_part_text_set(sd->edje, "volume.text.str", txt);
}

static Eina_Bool
volume_notification_hide_cb (void *data)
{
    volume_notifier_smart_data_t *sd = data;

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "volume,hide", "enna");

    ENNA_TIMER_DEL(sd->timer);
    return ECORE_CALLBACK_CANCEL;
}

void
enna_volume_notification_show (Evas_Object *obj)
{
    volume_notifier_smart_data_t *sd;

    if (!obj)
        return;

    sd = evas_object_data_get(obj, "sd");
    volume_notification_update(sd->edje);

    /* reset timer */
    ENNA_TIMER_DEL(sd->timer);
    sd->timer =
        ecore_timer_add(VOLUME_TIMER_VALUE, volume_notification_hide_cb, sd);

    edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                            "volume,show", "enna");
}

Evas_Object *
enna_volume_notification_smart_add (Evas *evas)
{
  volume_notifier_smart_data_t *sd;

  sd = ENNA_NEW(volume_notifier_smart_data_t, 1);

  sd->edje = edje_object_add(evas);
  evas_object_event_callback_add(sd->edje, EVAS_CALLBACK_DEL, cb_del, sd);

  evas_object_data_set(sd->edje, "sd", sd);
  edje_object_file_set(sd->edje, enna_config_theme_get(),
                       "enna/notification/volume");

  return sd->edje;
}
