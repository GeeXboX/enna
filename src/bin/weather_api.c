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
#include "xml_utils.h"
#include "url_utils.h"
#include "logs.h"
#include "content.h"
#include "mainmenu.h"
#include "utils.h"
#include "weather_api.h"
#include "view_list2.h"
#include "input.h"

#define ENNA_MODULE_NAME      "weather"

#define WEATHER_QUERY         "http://www.google.com/ig/api?weather=%s&hl=%s"
#define MAX_URL_SIZE          1024

#define WEATHER_DEFAULT_CITY  "Paris"
#define WEATHER_DEFAULT_TEMP  TEMP_CELCIUS

#undef DEBUG

typedef struct weather_cfg_s {
    Eina_List *cities;
    temp_type_t unit;
} weather_cfg_t;

static weather_cfg_t weather_cfg;
static Input_Listener *_input_listener = NULL;
static Evas_Object *_o_cfg_panel;
static Enna_Config_Panel *_cfg_panel = NULL;
static int _weather_init_count = 0;
/****************************************************************************/
/*                        Google Weather API                                */
/****************************************************************************/

static const struct {
    const char *icon;
    const char *data;
} weather_icon_mapping[] = {
    { "weather/cloudy",           "/partly_cloudy"             },
    { "weather/foggy",            "/haze"                      },
    { "weather/foggy",            "/fog"                       },
    { "weather/heavy_rain",       "/rain"                      },
    { "weather/light_snow",       "/chance_of_snow"            },
    { "weather/rain",             "/mist"                      },
    { "weather/rain",             "/chance_of_rain"            },
    { "weather/snow",             "/snow"                      },
    { "weather/clouds",           "/cloudy"                    },
    { "weather/light_clouds",     "/mostly_sunny"              },
    { "weather/mostly_cloudy",    "/mostly_cloudy"             },
    { "weather/sunny",            "/sunny"                     },
    { "weather/windy",            "/flurries"                  },
    { "weather/rain_storm",       "/chance_of_storm"           },
    { "weather/rain_storm",       "/thunderstorm"              },
    { "weather/ice",              "/icy"                       },
#if 0
    /* matches to be found */
    { "weather/showers",          "/"                          },
    { "weather/sun_rain",         "/"                          },
    { "weather/sun_snow",         "/"                          },
    { "weather/sun_storm",        "/"                          },
#endif
    { NULL,                       NULL }
};

static void
weather_display_debug (weather_t *w)
{
#if DEBUG
    int i;

    if (!w)
        return;

    printf("************************************\n");
    printf("** City: %s\n", w->city);
    printf("** Lang: %s\n", w->lang);
    printf("** Temp: %s\n", w->temp ? "F" : "C");
    printf("** Date: %s\n", w->date);
    printf("**   Current:\n");
    printf("**     Condition: %s\n", w->current.condition);
    printf("**     Temp: %s\n", w->current.temp);
    printf("**     Humidity: %s\n", w->current.humidity);
    printf("**     Icon: %s\n", w->current.icon);
    printf("**     Wind: %s\n", w->current.wind);

    for (i = 0; i < 4; i++)
    {
        printf("**   Forecast %d:\n", i + 1);
        printf("**     Day: %s\n", w->forecast[i].day);
        printf("**     Low: %s\n", w->forecast[i].low);
        printf("**     High: %s\n", w->forecast[i].high);
        printf("**     Icon: %s\n", w->forecast[i].icon);
        printf("**     Condition: %s\n", w->forecast[i].condition);
    }
    printf("************************************\n");
#endif
}

static int
weather_c_to_f (int c)
{
    return ((int) ((9 * c / 5) + 32));
}

static int
weather_f_to_c (int f)
{
    return ((int) (5 * (f - 32) / 9));
}

static int
weather_get_degrees (weather_t *w, int value)
{
    if (!w)
        return 0;

    switch (w->temp)
    {
    case TEMP_CELCIUS:
        return (weather_cfg.unit == TEMP_CELCIUS) ?
            value : weather_c_to_f(value);
    case TEMP_FAHRENHEIT:
        return (weather_cfg.unit == TEMP_CELCIUS) ?
            weather_f_to_c(value) : value;
    }

    return 0;
}

static void
weather_update_degrees (weather_t *w, char **temp)
{
    int t1, t2;
    char val[8];

    if (!temp)
        return;

    t1 = atoi(*temp);
    ENNA_FREE(*temp);

    t2 = weather_get_degrees(w, t1);
    snprintf(val, sizeof(val), "%d%s", t2,
             (weather_cfg.unit == TEMP_CELCIUS) ? "°C" : "°F");

    *temp = strdup(val);
}

static void
weather_get_unit_system (weather_t *w, xmlDocPtr doc)
{
    xmlChar *tmp;
    xmlNode *n;

    if (!w || !doc)
        return;

    n = get_node_xml_tree(xmlDocGetRootElement(doc), "forecast_information");
    if (!n)
        return;

    tmp = get_attr_value_from_xml_tree(n, "unit_system", "data");
    if (!tmp)
        return;

    w->temp = (!xmlStrcmp ((unsigned char *) "SI", tmp))
        ? TEMP_CELCIUS : TEMP_FAHRENHEIT;

    xmlFree(tmp);
}

static void
weather_set_field (xmlNode *n, char *prop, char *extra, char **field)
{
    xmlChar *tmp;
    char val[256] = { 0 };

    if (!n || !prop || !field)
        return;

    tmp = get_attr_value_from_xml_tree(n, prop, "data");
    if (!tmp)
        return;

    if (extra)
        snprintf(val, sizeof(val), "%s%s", tmp, extra);
    else
        snprintf(val, sizeof(val), "%s", tmp);

    ENNA_FREE(*field);

    if (!strcmp (prop, "icon"))
    {
        int i;

        for (i = 0; weather_icon_mapping[i].icon; i++)
            if (strstr((char *) tmp, weather_icon_mapping[i].data))
            {
                *field = strdup(weather_icon_mapping[i].icon);
                goto out;
            }

        enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                 "Unable to determine an icon match for '%s'", tmp);
        *field = strdup("weather/unknown");
    }
    else
        *field = strdup(val);

 out:
    xmlFree(tmp);
}

static void
weather_get_forecast (weather_t *w, xmlDocPtr doc)
{
    char *temp;
    xmlNode *n;
    int i;

    if (!w || !doc)
        return;

    temp = (weather_cfg.unit == TEMP_CELCIUS) ? "°C" : "°F";

    /* check for forecast information node */
    n = get_node_xml_tree(xmlDocGetRootElement(doc), "forecast_conditions");
    if (!n)
        return;

    for (i = 0; i < 4; i++)
    {
        weather_set_field(n, "day_of_week", NULL, &w->forecast[i].day);
        weather_set_field(n, "low",         temp, &w->forecast[i].low);
        weather_set_field(n, "high",        temp, &w->forecast[i].high);
        weather_set_field(n, "icon",        NULL, &w->forecast[i].icon);
        weather_set_field(n, "condition",   NULL, &w->forecast[i].condition);

        weather_update_degrees(w, &w->forecast[i].low);
        weather_update_degrees(w, &w->forecast[i].high);

        n = n->next;
    }
}

static void
weather_get_current (weather_t *w, xmlDocPtr doc)
{
    xmlNode *n;

    if (!w || !doc)
        return;

    /* check for current conditions node */
    n = get_node_xml_tree(xmlDocGetRootElement(doc), "current_conditions");
    if (!n)
        return;

    weather_set_field(n, "condition",      NULL, &w->current.condition);

    if (weather_cfg.unit == TEMP_CELCIUS)
        weather_set_field(n, "temp_c", "°C", &w->current.temp);
    else if (weather_cfg.unit == TEMP_FAHRENHEIT)
        weather_set_field(n, "temp_f", "°F", &w->current.temp);

    weather_set_field(n, "humidity",       NULL, &w->current.humidity);
    weather_set_field(n, "icon",           NULL, &w->current.icon);
    weather_set_field(n, "wind_condition", NULL, &w->current.wind);
}

static void
weather_get_infos (weather_t *w, xmlDocPtr doc)
{
    xmlNode *n;

    if (!w || !doc)
        return;

    n = get_node_xml_tree(xmlDocGetRootElement(doc), "forecast_information");
    if (!n)
        return;

    weather_set_field(n, "city",              NULL, &w->city);
    weather_set_field(n, "current_date_time", NULL, &w->date);
}

static void
weather_google_search (weather_t *w)
{
    char url[MAX_URL_SIZE];
    char *escaped_city;
    url_data_t data;
    url_t handler;
    xmlDocPtr doc = NULL;
    xmlNode *n;

    if (!w)
        return;

    handler = url_new();
    if (!handler)
        goto error;

    escaped_city = url_escape_string(handler, w->city);
    if (!escaped_city)
        goto error;

    /* proceed with Google Weather API request */
    memset(url, '\0', MAX_URL_SIZE);
    snprintf(url, MAX_URL_SIZE, WEATHER_QUERY, escaped_city, w->lang);
    free(escaped_city);
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Search Request: %s", url);

    data = url_get_data(handler, url);
    if (data.status != 0)
        goto error;

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Search Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = get_xml_doc_from_memory(data.buffer);
    free(data.buffer);
    if (!doc)
        goto error;

    /* check for existing city */
    n = get_node_xml_tree(xmlDocGetRootElement(doc), "problem_cause");
    if (n)
    {
        enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                 "The requested city (%s) can't be found.", w->city);
        goto error;
    }

    weather_get_unit_system (w, doc);
    weather_get_infos (w, doc);
    weather_get_current (w, doc);
    weather_get_forecast (w, doc);

 error:
    if (doc)
    {
        xmlFreeDoc(doc);
        doc = NULL;
    }
    url_free(handler);
}

/****************************************************************************/
/*                      Configuration Panel section                         */
/****************************************************************************/

static Eina_Bool
_weather_config_panel_input_events_cb(void *data, enna_input event)
{
    return enna_list2_input_feed(_o_cfg_panel, event);
}

static Evas_Object *
_weather_config_panel_show(void *data)
{
    Eina_List *l;
    char *city;
    Elm_Genlist_Item *item;

    _o_cfg_panel= enna_list2_add(enna->evas);
    evas_object_size_hint_align_set(_o_cfg_panel, -1.0, -1.0);
    evas_object_size_hint_weight_set(_o_cfg_panel, 1.0, 1.0);
    evas_object_show(_o_cfg_panel);

    EINA_LIST_FOREACH(weather_cfg.cities, l, city)
    {
        item = enna_list2_append(_o_cfg_panel,
                                 _("City"),
                                 _("Choose your city"),
                                 NULL,
                                 NULL, NULL);
        enna_list2_item_entry_add(item,
                                  NULL, city,
                                  NULL, NULL);
        enna_list2_item_button_add(item,
                                   NULL, _("Remove"),
                                   NULL, NULL);
    }

    item = enna_list2_append(_o_cfg_panel,
                             _("Add new city"),
                             _("Add more cities to the list"),
                             NULL,
                             NULL, NULL);

     enna_list2_item_button_add(item,
                                NULL, _("Add"),
                                NULL, NULL);

    if (!_input_listener)
        _input_listener = enna_input_listener_add("configuration/weather",
                                            _weather_config_panel_input_events_cb, NULL);
    return _o_cfg_panel;
}

static void
_weather_config_panel_hide(void *data)
{
    ENNA_OBJECT_DEL(_o_cfg_panel);
    if (!_input_listener)
        enna_input_listener_del(_input_listener);
    _input_listener = NULL;
}

/****************************************************************************/
/*                      Configuration Section Parser                        */
/****************************************************************************/

static void
cfg_weather_section_load (const char *section)
{
    Eina_List *cities;
    const char *unit = NULL;

    cities = enna_config_string_list_get(section, "city");
    if (cities)
    {
        Eina_List *l;
        char *city;

        EINA_LIST_FREE(weather_cfg.cities, city)
            free(city);

        EINA_LIST_FOREACH(cities, l, city)
            weather_cfg.cities = eina_list_append(weather_cfg.cities,
                                                  strdup(city));
    }

    unit = enna_config_string_get(section, "unit");
    if (unit)
    {
        if (!strcmp(unit, "C"))
            weather_cfg.unit = TEMP_CELCIUS;
        else if (!strcmp(unit, "F"))
            weather_cfg.unit = TEMP_FAHRENHEIT;
    }
}

static void
cfg_weather_section_save (const char *section)
{
    Eina_List *l;
    char *city;

    EINA_LIST_FOREACH(weather_cfg.cities, l, city)
        enna_config_string_set(section, "city", city);
    enna_config_string_set(section, "unit",
                           (weather_cfg.unit == TEMP_CELCIUS) ? "C" : "F");
}

static void
cfg_weather_free (void)
{
    char *city;

    EINA_LIST_FREE(weather_cfg.cities, city)
        free(city);
}

static void
cfg_weather_section_set_default (void)
{
    cfg_weather_free();

    weather_cfg.cities = eina_list_append(weather_cfg.cities,
                                          strdup(enna->geo_loc ?
                                                 enna->geo_loc :
                                                 WEATHER_DEFAULT_CITY));

    weather_cfg.unit = WEATHER_DEFAULT_TEMP;

}

static Enna_Config_Section_Parser cfg_weather = {
    "weather",
    cfg_weather_section_load,
    cfg_weather_section_save,
    cfg_weather_section_set_default,
    cfg_weather_free,
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

void
enna_weather_parse_config ()
{
    cfg_weather_section_set_default();
    cfg_weather_section_load(cfg_weather.section);
}

void
enna_weather_update (weather_t *w)
{
    if (!w)
        return;

    weather_google_search(w);
    weather_display_debug(w);
}

void
enna_weather_set_city (weather_t *w, const char *city)
{
    if (!w || !city)
        return;

    ENNA_FREE(w->city);
    w->city = strdup(city);
    enna_weather_update(w);
}

void
enna_weather_set_lang (weather_t *w, const char *lang)
{
    if (!w || !lang)
        return;

    ENNA_FREE(w->lang);
    w->lang = strdup(lang);
    enna_weather_update(w);
}

Eina_List *
enna_weather_cities_get(void)
{
    return weather_cfg.cities;
}

weather_t *
enna_weather_new (char *city)
{
    weather_t *w;

    w = ENNA_NEW(weather_t, 1);
    w->lang = get_lang();
    w->city = strdup(WEATHER_DEFAULT_CITY);
    w->temp = WEATHER_DEFAULT_TEMP;
    w->city = strdup(city);

    return w;
}

void
enna_weather_free (weather_t *w)
{
    int i;

    if (!w)
        return;

    ENNA_FREE(w->city);
    ENNA_FREE(w->lang);
    ENNA_FREE(w->date);

    ENNA_FREE(w->current.condition);
    ENNA_FREE(w->current.temp);
    ENNA_FREE(w->current.humidity);
    ENNA_FREE(w->current.icon);
    ENNA_FREE(w->current.wind);

    for (i = 0; i < 4; i++)
    {
        ENNA_FREE(w->forecast[i].day);
        ENNA_FREE(w->forecast[i].low);
        ENNA_FREE(w->forecast[i].high);
        ENNA_FREE(w->forecast[i].icon);
        ENNA_FREE(w->forecast[i].condition);
    }

    ENNA_FREE(w);
}

void
enna_weather_cfg_register (void)
{
    enna_config_section_parser_register(&cfg_weather);
}

int
enna_weather_init(void)
{
    /* Prevent multiple loads */
    if (_weather_init_count > 0)
        return ++_weather_init_count;

    enna_weather_parse_config();

    _cfg_panel =
        enna_config_panel_register(_("Weather"), "icon/weather",
                                   _weather_config_panel_show,
                                   _weather_config_panel_hide, NULL);
    _weather_init_count = 1;
    return 1;
}

int
enna_weather_shutdown(void)
{
    /* Shutdown only when all clients have called this function */
    _weather_init_count--;
    if (_weather_init_count == 0)
        enna_config_panel_unregister(_cfg_panel);
    return _weather_init_count;
}
