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

#include <Edje.h>

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
}

Evas_Object *
enna_weather_notification_smart_add (Evas *evas)
{
  weather_notifier_smart_data_t *sd;

  sd = ENNA_NEW(weather_notifier_smart_data_t, 1);
  sd->w = enna_weather_init();

  sd->edje = edje_object_add(evas);
  evas_object_event_callback_add(sd->edje, EVAS_CALLBACK_DEL, cb_del, sd);

  evas_object_data_set(sd->edje, "sd", sd);
  edje_object_file_set(sd->edje, enna_config_theme_get(),
                       "enna/notification/weather");
  enna_weather_notification_update(sd->edje);

  return sd->edje;
}
