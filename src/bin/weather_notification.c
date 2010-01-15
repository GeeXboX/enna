/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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
#include "weather_api.h"
#include "weather_notification.h"

typedef struct weather_notifier_sd_s {
    weather_t *w;
    Evas_Object *edje;
    Evas_Object *icon;
} weather_notifier_smart_data_t;

static void
cb_del (void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    weather_notifier_smart_data_t *sd = data;

    ENNA_OBJECT_DEL(sd->icon);
    enna_weather_free(sd->w);
    ENNA_FREE(sd);
}

void
enna_weather_notification_update (Evas_Object *obj)
{
  weather_notifier_smart_data_t *sd;

  if (!obj)
    return;

  sd = evas_object_data_get(obj, "sd");
  /*enna_weather_free(sd->w);
    sd->w = enna_weather_new();*/
  enna_weather_update(sd->w);

  /* weather icon */
  ENNA_OBJECT_DEL(sd->icon);
  sd->icon = edje_object_add(evas_object_evas_get(sd->edje));
  edje_object_file_set(sd->icon, enna_config_theme_get(), sd->w->current.icon);
  edje_object_part_swallow(sd->edje, "weather.icon.swallow", sd->icon);

  /* weather texts */
  edje_object_part_text_set(sd->edje, "weather.text.city.str",
                            sd->w->city);
  edje_object_part_text_set(sd->edje, "weather.text.condition.str",
                            sd->w->current.condition);
  edje_object_part_text_set(sd->edje, "weather.text.temp.str",
                            sd->w->current.temp);

  /* check whether or not to display the notifier */
  edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                          sd->w->current.condition ?
                          "weather,show" : "weather,hide", "enna");
}

Evas_Object *
enna_weather_notification_smart_add (Evas *evas)
{
  weather_notifier_smart_data_t *sd;
  Eina_List *cities;

  sd = ENNA_NEW(weather_notifier_smart_data_t, 1);

  cities = enna_weather_cities_get();
  sd->w = enna_weather_new(cities->data);

  sd->edje = edje_object_add(evas);
  evas_object_event_callback_add(sd->edje, EVAS_CALLBACK_DEL, cb_del, sd);

  evas_object_data_set(sd->edje, "sd", sd);
  edje_object_file_set(sd->edje, enna_config_theme_get(),
                       "enna/notification/weather");
  enna_weather_notification_update(sd->edje);

  return sd->edje;
}
