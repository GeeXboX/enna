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

#define ENNA_MODULE_NAME      "weather"

#define WEATHER_QUERY         "http://www.google.com/ig/api?weather=%s&hl=%s"
#define MAX_URL_SIZE          1024

#define WEATHER_DEFAULT_CITY  "Paris"
#define WEATHER_DEFAULT_TEMP  TEMP_CELCIUS

#undef DEBUG

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
#if 0
    /* matches to be found */
    { "weather/ice",              "/"                          },
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
                return;
            }

        enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                 "Unable to determine an icon match for '%s'", tmp);
        *field = strdup("weather/unknown");
    }
    else
        *field = strdup(val);
}

static void
weather_get_forecast (weather_t *w, xmlDocPtr doc)
{
    char *temp;
    xmlNode *n;
    int i;

    if (!w || !doc)
        return;

    temp = (w->temp == TEMP_CELCIUS) ? "°C" : "°F";

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

    if (w->temp == TEMP_CELCIUS)
        weather_set_field(n, "temp_c", "°C", &w->current.temp);
    else if (w->temp == TEMP_FAHRENHEIT)
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
    url_data_t data;
    url_t handler;
    xmlDocPtr doc = NULL;
    xmlNode *n;

    if (!w)
        return;

    handler = url_new();
    if (!handler)
        goto error;

    /* proceed with Google Weather API request */
    memset(url, '\0', MAX_URL_SIZE);
    snprintf(url, MAX_URL_SIZE, WEATHER_QUERY,
             url_escape_string(handler, w->city), w->lang);
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
/*                         Public Module API                                */
/****************************************************************************/

void
enna_weather_parse_config (weather_t *w)
{
    Enna_Config_Data *cfgdata;
    Eina_List *l;

    if (!w)
        return;

    cfgdata = enna_config_module_pair_get (ENNA_MODULE_NAME);
    if (!cfgdata)
        return;

    for (l = cfgdata->pair; l; l = l->next)
    {
        Config_Pair *pair = l->data;

        if (!strcmp (pair->key, "city"))
        {
            char *city = NULL;
            enna_config_value_store (&city, pair->key,
                                     ENNA_CONFIG_STRING, pair);

            if (city)
            {
                ENNA_FREE(w->city);
                w->city = strdup(city);
            }
        }
    }
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

weather_t *
enna_weather_init (void)
{
    weather_t *w;

    w = ENNA_NEW(weather_t, 1);
    w->lang = get_lang();
    w->city = strdup(WEATHER_DEFAULT_CITY);
    w->temp = WEATHER_DEFAULT_TEMP;
    enna_weather_parse_config (w);

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
