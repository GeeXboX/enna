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
#include "../modules/activity/weather/weather_api.h"
#include "gadgets.h"
#include "module.h"

#define ENNA_MODULE_NAME "gadget_weather"


typedef struct weather_notifier_sd_s {
    weather_t *w;
    Evas_Object *edje;
    Evas_Object *icon;
} weather_notifier_smart_data_t;

static weather_notifier_smart_data_t *sd = NULL;

static void
cb_del (void *data, Evas *a, Evas_Object *obj, void *event_info)
{
    weather_notifier_smart_data_t *sd = data;

    ENNA_OBJECT_DEL(sd->icon);
}

static void
enna_weather_notification_update (Evas_Object *obj)
{
  weather_notifier_smart_data_t *sd;

  if (!obj)
    return;

  sd = evas_object_data_get(obj, "sd");
  /*enna_weather_free(sd->w);
    sd->w = enna_weather_new();*/
  if (!sd->w->current.temp)
      enna_weather_update(sd->w);

  /* weather icon */
  ENNA_OBJECT_DEL(sd->icon);
  sd->icon = elm_icon_add(sd->edje);
  elm_icon_file_set(sd->icon, enna_config_theme_get(), sd->w->current.icon);
  evas_object_show(sd->icon);
  elm_layout_content_set(sd->edje, "weather.icon.swallow", sd->icon);

  /* weather texts */
  elm_layout_text_set(sd->edje, "weather.text.city.str",
                      sd->w->city);
  elm_layout_text_set(sd->edje, "weather.text.condition.str",
                      sd->w->current.condition);
  elm_layout_text_set(sd->edje, "weather.text.temp.str",
                      sd->w->current.temp);

  printf("current temp : %s\n", sd->w->current.temp);

  /* check whether or not to display the notifier */
/*  edje_object_signal_emit(elm_layout_edje_get(enna->layout),
                          sd->w->current.condition ?
                          "weather,show" : "weather,hide", "enna");*/
}

static Evas_Object *
enna_weather_notification_smart_add(Evas_Object *parent)
{
//    Evas_Coord w, h;
//    Evas_Object *o_edje;

    ENNA_OBJECT_DEL(sd->edje);

    sd->edje = elm_layout_add(parent);
    evas_object_event_callback_add(sd->edje, EVAS_CALLBACK_DEL, cb_del, sd);

    evas_object_data_set(sd->edje, "sd", sd);
    elm_layout_file_set(sd->edje, enna_config_theme_get(),
                        "enna/notification/weather");
    /* o_edje = elm_layout_edje_get(sd->edje); */
    /* edje_object_size_min_get(o_edje, &w, &h); */
    /* evas_object_size_hint_min_set(sd->edje, w, h); */
    enna_weather_notification_update(sd->edje);

    printf("Smart add\n");

    evas_object_show(sd->edje);
    elm_layout_content_set(parent, "enna.swallow.weather", sd->edje);
    return NULL;
}

static void
enna_weather_notification_smart_del()
{
    if (sd)
        ENNA_OBJECT_DEL(sd->edje);
}

static Enna_Gadget gadget =
{
    enna_weather_notification_smart_add,
    enna_weather_notification_smart_del,

};


/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_gadget_weather
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    Eina_List *cities;

    sd = ENNA_NEW(weather_notifier_smart_data_t, 1);
    cities = enna_weather_cities_get();
    sd->w = enna_weather_new(cities->data);

    enna_gadgets_register(&gadget);
}

static void
module_shutdown(Enna_Module *em)
{
    ENNA_OBJECT_DEL(sd->edje);
    enna_weather_free(sd->w);
    ENNA_FREE(sd);
    sd = NULL;
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME,
    N_("Weather Gadget"),
    NULL,
    N_("Module to show weather on the desktop"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
