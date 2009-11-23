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

#define DEBUG 1

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
weather_display_debug (weather_smart_data_t *sd)
{
#if DEBUG
    int i;

    if (!sd)
        return;

    printf("************************************\n");
    printf("** City: %s\n", sd->city);
    printf("** Lang: %s\n", sd->lang);
    printf("** Temp: %s\n", sd->temp ? "F" : "C");
    printf("**   Current:\n");
    printf("**     Condition: %s\n", sd->current.condition);
    printf("**     Temp: %s\n", sd->current.temp);
    printf("**     Humidity: %s\n", sd->current.humidity);
    printf("**     Icon: %s\n", sd->current.icon);
    printf("**     Wind: %s\n", sd->current.wind);

    for (i = 0; i < 4; i++)
    {
        printf("**   Forecast %d:\n", i + 1);
        printf("**     Day: %s\n", sd->forecast[i].day);
        printf("**     Low: %s\n", sd->forecast[i].low);
        printf("**     High: %s\n", sd->forecast[i].high);
        printf("**     Icon: %s\n", sd->forecast[i].icon);
        printf("**     Condition: %s\n", sd->forecast[i].condition);
    }
    printf("************************************\n");
#endif
}

static void
weather_get_unit_system (weather_smart_data_t *sd, xmlDocPtr doc)
{
    xmlChar *tmp;
    xmlNode *n;

    if (!sd || !doc)
        return;

    n = get_node_xml_tree(xmlDocGetRootElement(doc), "forecast_information");
    if (!n)
        return;

    tmp = get_attr_value_from_xml_tree(n, "unit_system", "data");
    if (!tmp)
        return;

    sd->temp = (!xmlStrcmp ((unsigned char *) "SI", tmp))
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
weather_get_forecast (weather_smart_data_t *sd, xmlDocPtr doc)
{
    char *temp;
    xmlNode *n;
    int i;

    if (!sd || !doc)
        return;

    temp = (sd->temp == TEMP_CELCIUS) ? "°C" : "°F";

    /* check for forecast information node */
    n = get_node_xml_tree(xmlDocGetRootElement(doc), "forecast_conditions");
    if (!n)
        return;

    for (i = 0; i < 4; i++)
    {
        weather_set_field(n, "day_of_week", NULL, &sd->forecast[i].day);
        weather_set_field(n, "low",         temp, &sd->forecast[i].low);
        weather_set_field(n, "high",        temp, &sd->forecast[i].high);
        weather_set_field(n, "icon",        NULL, &sd->forecast[i].icon);
        weather_set_field(n, "condition",   NULL, &sd->forecast[i].condition);

        n = n->next;
    }
}

static void
weather_get_current (weather_smart_data_t *sd, xmlDocPtr doc)
{
    xmlNode *n;

    if (!sd || !doc)
        return;

    /* check for current conditions node */
    n = get_node_xml_tree(xmlDocGetRootElement(doc), "current_conditions");
    if (!n)
        return;

    weather_set_field(n, "condition",      NULL, &sd->current.condition);

    if (sd->temp == TEMP_CELCIUS)
        weather_set_field(n, "temp_c", "°C", &sd->current.temp);
    else if (sd->temp == TEMP_FAHRENHEIT)
        weather_set_field(n, "temp_f", "°F", &sd->current.temp);

    weather_set_field(n, "humidity",       NULL, &sd->current.humidity);
    weather_set_field(n, "icon",           NULL, &sd->current.icon);
    weather_set_field(n, "wind_condition", NULL, &sd->current.wind);
}

static void
weather_google_search (weather_smart_data_t *sd)
{
    char url[MAX_URL_SIZE];
    url_data_t data;
    url_t handler;
    xmlDocPtr doc = NULL;
    xmlNode *n;

    if (!sd)
        return;

    handler = url_new();
    if (!handler)
        goto error;

    /* proceed with Google Weather API request */
    memset(url, '\0', MAX_URL_SIZE);
    snprintf(url, MAX_URL_SIZE, WEATHER_QUERY,
             url_escape_string(handler, sd->city), sd->lang);
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
                 "The requested city (%s) can't be found.", sd->city);
        goto error;
    }

    weather_get_unit_system (sd, doc);
    weather_get_current (sd, doc);
    weather_get_forecast (sd, doc);

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
enna_weather_parse_config (weather_smart_data_t *sd)
{
    Enna_Config_Data *cfgdata;
    Eina_List *l;

    if (!sd)
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
                ENNA_FREE(sd->city);
                sd->city = strdup(city);
            }
        }
    }
}

void
enna_weather_update (Evas_Object *obj)
{
    weather_smart_data_t *sd;

    if (!obj)
        return;

    sd = evas_object_data_get(obj, "sd");
    weather_google_search(sd);
    weather_display_debug(sd);
}

void
enna_weather_set_city (Evas_Object *obj, const char *city)
{
    weather_smart_data_t *sd;

    if (!obj || !city)
        return;

    sd = evas_object_data_get(obj, "sd");
    ENNA_FREE(sd->city);
    sd->city = strdup(city);
    enna_weather_update(obj);
}

void
enna_weather_set_lang (Evas_Object *obj, const char *lang)
{
    weather_smart_data_t *sd;

    if (!obj || !lang)
        return;

    sd = evas_object_data_get(obj, "sd");
    ENNA_FREE(sd->lang);
    sd->lang = strdup(lang);
    enna_weather_update(obj);
}
