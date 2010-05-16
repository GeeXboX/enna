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

#include <string.h>
#include <time.h>

#include <Ecore.h>
#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "activity.h"
#include "logs.h"
#include "content.h"
#include "mainmenu.h"
#include "utils.h"
#include "module.h"
#include "weather_api.h"

#define ENNA_MODULE_NAME "weather"

#define TIMER_TEMPO           20

typedef struct _Enna_Module_Weather {
    Evas *e;
    Evas_Object *edje;
    Enna_Module *em;
    Ecore_Timer *timer;
    Evas_Object *bg;
    Evas_Object *icon;
    Evas_Object *icon_d1, *icon_d2, *icon_d3, *icon_d4;
    int counter;
    weather_t *w;
} Enna_Module_Weather;

static Enna_Module_Weather *mod;

/****************************************************************************/
/*                        Google Weather API                                */
/****************************************************************************/

static void
set_picture (Evas_Object **old,
             const char *img, const char *field, const char *program)
{
    Evas_Object *obj, *old_obj;

    if (!img || !field)
        return;

    obj = edje_object_add (enna->evas);
    edje_object_file_set (obj, enna_config_theme_get (), img);

    old_obj = edje_object_part_swallow_get(mod->edje, field);
    edje_object_part_unswallow(mod->edje, old_obj);
    ENNA_OBJECT_DEL(old_obj);

    edje_object_part_swallow (mod->edje, field, obj);
    if (program)
        edje_object_signal_emit (mod->edje, program, "enna");

    *old = obj;
}

static void
fill_current_frame (void)
{
    edje_object_part_text_set (mod->edje, "frame.current.city",
                               mod->w->city);
    edje_object_part_text_set (mod->edje, "frame.current.date",
                               mod->w->date);
    edje_object_part_text_set (mod->edje, "frame.current.condition",
                               mod->w->current.condition);
    edje_object_part_text_set (mod->edje, "frame.current.temp",
                               mod->w->current.temp);
    edje_object_part_text_set (mod->edje, "frame.current.humidity",
                               mod->w->current.humidity);
    edje_object_part_text_set (mod->edje, "frame.current.wind",
                               mod->w->current.wind);

    set_picture (&mod->icon, mod->w->current.icon, "frame.current.icon", NULL);
}

static void
forecast_set_text (const char *edje, const char *val, int i)
{
    char field[64] = { 0 };

    snprintf (field, sizeof (field), "frame.forecast.day%d%s", i + 1, edje);
    edje_object_part_text_set (mod->edje, field, val);
}

static void
forecast_set_icon (Evas_Object *obj, int i)
{
    char field[64] = { 0 };

    snprintf (field, sizeof (field), "frame.forecast.day%d.icon", i + 1);
    set_picture (&obj, mod->w->forecast[i].icon, field, NULL);
}

static void
fill_forecast_frame (void)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        Evas_Object *icon;

        if (i == 0)
            icon = mod->icon_d1;
        else if (i == 1)
            icon = mod->icon_d2;
        else if (i == 2)
            icon = mod->icon_d3;
        else if (i == 3)
            icon = mod->icon_d4;

        forecast_set_text ("",            mod->w->forecast[i].day,       i);
        forecast_set_text (".low.text",   mod->w->forecast[i].low,       i);
        forecast_set_text (".high.text",  mod->w->forecast[i].high,      i);
        forecast_set_text (".conditions", mod->w->forecast[i].condition, i);

        forecast_set_icon (icon, i);
    }
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
create_gui (void)
{
    mod->edje = edje_object_add (enna->evas);
    edje_object_file_set (mod->edje,
                          enna_config_theme_get (), "activity/weather");
}

static void
set_background (int id)
{
    char *array[] = {
        "weather/spring",
        "weather/summer",
        "weather/fall",
        "weather/winter"
    };

    if (id < 0 || id > 4)
        return;

    set_picture (&mod->bg, array[id], "weather.bg", "bg,show");
}

static int
background_cb (void *data)
{
    mod->counter++;
    if ((mod->counter % 4) == 0)
        mod->counter = 0;

    set_background (mod->counter);

    /* keep running */
    return 1;
}


static void
update_background (void)
{
    ENNA_TIMER_DEL (mod->timer);
    mod->timer = ecore_timer_add (TIMER_TEMPO, background_cb, NULL);
}

static void
set_default_background (void)
{
    time_t t;
    struct tm *ti;
    int season = 0;

    t = time (NULL);
    ti = localtime (&t);

    if (mod->w->hemisphere == NORTHERN_HEMISPHERE)
    {
        /* spring: March - May */
        if (ti->tm_mon >= 2 && ti->tm_mon <= 4)
            season = 0;
        /* summer: June - August */
        else if (ti->tm_mon >= 5 && ti->tm_mon <= 7)
            season = 1;
        /* fall: September - November */
        else if (ti->tm_mon >= 8 && ti->tm_mon <= 10)
            season = 2;
        /* winter: December - February */
        else if (ti->tm_mon >= 11 || ti->tm_mon <= 1)
            season = 3;
    }
    else
    {
        /* spring: September - November */
        if (ti->tm_mon >= 8 && ti->tm_mon <= 10)
            season = 0;
        /* summer: December - February */
        else if (ti->tm_mon >= 11 || ti->tm_mon <= 1)
            season = 1;
        /* fall: March - May */
        else if (ti->tm_mon >= 2 && ti->tm_mon <= 4)
            season = 2;
        /* winter: June - August */
        else if (ti->tm_mon >= 5 && ti->tm_mon <= 7)
            season = 3;
    }

    set_background (season);
}

static void
_class_init (int dummy)
{
    create_gui ();
    set_default_background ();
    enna_content_append (ENNA_MODULE_NAME, mod->edje);
}

static void
_class_shutdown (int dummy)
{
    ENNA_TIMER_DEL (mod->timer);
    ENNA_OBJECT_DEL (mod->bg);
    ENNA_OBJECT_DEL (mod->icon);
    ENNA_OBJECT_DEL (mod->icon_d1);
    ENNA_OBJECT_DEL (mod->icon_d2);
    ENNA_OBJECT_DEL (mod->icon_d3);
    ENNA_OBJECT_DEL (mod->icon_d4);
    ENNA_OBJECT_DEL (mod->edje);
}

static void
_class_show (int dummy)
{
    enna_content_select(ENNA_MODULE_NAME);
    enna_weather_update(mod->w);
    update_background ();
    fill_current_frame ();
    fill_forecast_frame ();
    edje_object_signal_emit (mod->edje, "weather,show", "enna");
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit (mod->edje, "weather,hide", "enna");
}

static void
_class_event (enna_input event)
{
    if (event != ENNA_INPUT_BACK)
        return;

    enna_content_hide();
    enna_mainmenu_show();
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    9,
    N_("Weather"),
    NULL,
    "icon/weather",
    "background/weather",
    ENNA_CAPS_NONE,
    {
        _class_init,
        NULL,
        _class_shutdown,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_activity_weather
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    Eina_List *cities;

    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_Weather));
    mod->em = em;

    cities = enna_weather_cities_get();
    mod->w = enna_weather_new(cities->data);
    em->mod = mod;

    enna_activity_register(&class);
}

static void
module_shutdown(Enna_Module *em)
{
    enna_activity_unregister(&class);

    enna_weather_free(mod->w);
    ENNA_TIMER_DEL (mod->timer);
    ENNA_OBJECT_DEL (mod->edje);
    ENNA_FREE (mod);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "activity_weather",
    N_("Weather forecasts"),
    "icon/weather",
    N_("Show the weather at your location"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
