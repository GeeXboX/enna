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
#include "activity.h"
#include "xml_utils.h"
#include "url_utils.h"
#include "logs.h"
#include "content.h"
#include "mainmenu.h"
#include "utils.h"
#include "module.h"

#define ENNA_MODULE_NAME "weather"

#define WEATHER_QUERY         "http://www.google.com/ig/api?weather=%s&hl=%s"
#define MAX_URL_SIZE          1024

#define TIMER_TEMPO           20

typedef enum {
    TEMP_CELCIUS,
    TEMP_FAHRENHEIT
} temp_type_t;

typedef struct _Enna_Module_Weather {
    Evas *e;
    Evas_Object *edje;
    Enna_Module *em;
    Ecore_Timer *timer;
    Evas_Object *bg;
    Evas_Object *icon;
    Evas_Object *icon_d1, *icon_d2, *icon_d3, *icon_d4;
    int counter;
    char *city;
    char *lang;
    temp_type_t temp;
} Enna_Module_Weather;

static Enna_Module_Weather *mod;

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
get_unit_system (xmlDocPtr doc)
{
    xmlChar *tmp;
    xmlNode *n;

    if (!doc)
        return;

    n = get_node_xml_tree (xmlDocGetRootElement (doc),
                           "forecast_information");
    if (!n)
        return;

    tmp = get_attr_value_from_xml_tree (n, "unit_system", "data");
    if (!tmp)
        return;

    if (!xmlStrcmp ((unsigned char *) "SI", tmp))
        mod->temp = TEMP_CELCIUS;
    else
        mod->temp = TEMP_FAHRENHEIT;
}


static void
set_field (xmlNode *n, char *prop, char *extra, char *field)
{
    xmlChar *tmp;
    char val[256];

    if (!n || !prop || !field)
        return;

    tmp = get_attr_value_from_xml_tree (n, prop, "data");
    if (!tmp)
        return;

    memset (val, '\0', sizeof (val));
    if (extra)
        snprintf (val, sizeof (val), "%s%s", tmp, extra);
    else
        snprintf (val, sizeof (val), "%s", tmp);

    edje_object_part_text_set (mod->edje, field, val);
}

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
weather_set_icon (Evas_Object *obj, char *data, char *field)
{
    int i;

    if (!data || !field)
        goto error_set_icon;

    for (i = 0; weather_icon_mapping[i].icon; i++)
        if (strstr (data, weather_icon_mapping[i].data))
        {
            set_picture (&obj, weather_icon_mapping[i].icon, field, NULL);
            return;
        }

 error_set_icon:
    enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
              "Unable to determine an icon match for '%s'", data);
    set_picture (&obj, "weather/unknown", field, NULL);
}

static void
fill_current_frame (xmlDocPtr doc)
{
    xmlNode *n;
    xmlChar *tmp;

    if (!doc)
        return;

    /* check for forecast information node */
    n = get_node_xml_tree (xmlDocGetRootElement (doc),
                           "forecast_information");
    if (!n)
        goto current_conditions_node;

    set_field (n, "city", NULL, "frame.current.city");
    set_field (n, "current_date_time", NULL, "frame.current.date");

 current_conditions_node:
    /* check for current conditions node */
    n = get_node_xml_tree (xmlDocGetRootElement (doc),
                           "current_conditions");
    if (!n)
        return;

    set_field (n, "condition", NULL, "frame.current.condition");

    if (mod->temp == TEMP_CELCIUS)
        set_field (n, "temp_c", "°C", "frame.current.temp");
    else if (mod->temp == TEMP_FAHRENHEIT)
        set_field (n, "temp_f", "°F", "frame.current.temp");

    set_field (n, "humidity", NULL, "frame.current.humidity");

    tmp = get_attr_value_from_xml_tree (n, "icon", "data");
    weather_set_icon (mod->icon, (char * ) tmp, "frame.current.icon");

    set_field (n, "wind_condition", NULL, "frame.current.wind");
}

static void
fill_forecast_frame (xmlDocPtr doc)
{
    char *temp = (mod->temp == TEMP_CELCIUS) ? "°C" : "°F";
    xmlNode *n;
    int i;

    if (!doc)
        return;

    /* check for forecast information node */
    n = get_node_xml_tree (xmlDocGetRootElement (doc),
                           "forecast_conditions");
    if (!n)
        return;

    for (i = 0; i < 4; i++)
    {
        char field[128];
        Evas_Object *icon = NULL;
        xmlChar *tmp;

        if (i == 0)
            icon = mod->icon_d1;
        else if (i == 1)
            icon = mod->icon_d2;
        else if (i == 2)
            icon = mod->icon_d3;
        else if (i == 3)
            icon = mod->icon_d4;

        memset (field, '\0', sizeof (field));
        snprintf (field, sizeof (field), "frame.forecast.day%d", i + 1);
        set_field (n, "day_of_week", NULL, field);

        memset (field, '\0', sizeof (field));
        snprintf (field, sizeof (field),
                  "frame.forecast.day%d.low.text", i + 1);
        set_field (n, "low", temp, field);

        memset (field, '\0', sizeof (field));
        snprintf (field, sizeof (field),
                  "frame.forecast.day%d.high.text", i + 1);
        set_field (n, "high", temp, field);


        tmp = get_attr_value_from_xml_tree (n, "icon", "data");
        memset (field, '\0', sizeof (field));
        snprintf (field, sizeof (field),
                  "frame.forecast.day%d.icon", i + 1);
        weather_set_icon (icon, (char * ) tmp, field);

        memset (field, '\0', sizeof (field));
        snprintf (field, sizeof (field),
                  "frame.forecast.day%d.conditions", i + 1);
        set_field (n, "condition", NULL, field);

        n = n->next;
    }
}

static void
google_weather_search (void)
{
    char url[MAX_URL_SIZE];
    url_data_t data;
    url_t handler;
    xmlDocPtr doc = NULL;
    xmlNode *n;

    handler = url_new ();
    if (!handler)
        goto error;

    /* proceed with Google Weather API request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, WEATHER_QUERY,
              url_escape_string (handler, mod->city), mod->lang);
    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Search Request: %s", url);

    data = url_get_data (handler, url);
    if (data.status != 0)
        goto error;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Search Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = get_xml_doc_from_memory (data.buffer);
    free (data.buffer);
    if (!doc)
        goto error;

    /* check for existing city */
    n = get_node_xml_tree (xmlDocGetRootElement (doc), "problem_cause");
    if (n)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "The requested city (%s) can't be found.", mod->city);
        goto error;
    }

    get_unit_system (doc);
    fill_current_frame (doc);
    fill_forecast_frame (doc);

 error:
    if (doc)
    {
        xmlFreeDoc (doc);
        doc = NULL;
    }
    url_free (handler);
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
create_gui (void)
{
    mod->edje = edje_object_add (enna->evas);
    edje_object_file_set (mod->edje,
                          enna_config_theme_get (), "module/weather");
}

static void
parse_config (void)
{
    Enna_Config_Data *cfgdata;
    Eina_List *l;

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
                ENNA_FREE (mod->city);
                mod->city = strdup (city);
            }
        }
    }
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

    /* spring: March - May */
    if (ti->tm_mon >= 0 && ti->tm_mon <= 4)
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

    set_background (season);
}

static void
_class_init (int dummy)
{
    parse_config ();
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
    update_background ();
    google_weather_search ();
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
    if (event != ENNA_INPUT_EXIT)
        return;

    enna_content_hide();
    enna_mainmenu_show();
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("Weather"),
    NULL,
    "icon/weather",
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

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "activity_weather",
    "Weather forecast",
    "icon/weather",
    "Show the weather at your location",
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
module_init (Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_Weather));
    mod->em = em;
    mod->lang = get_lang();
    mod->city = strdup ("New York");
    mod->temp = TEMP_CELCIUS;
    em->mod = mod;

    enna_activity_add (&class);
}

void
module_shutdown (Enna_Module *em)
{
    enna_activity_del(ENNA_MODULE_NAME);

    ENNA_TIMER_DEL (mod->timer);
    ENNA_FREE (mod->city);
    ENNA_FREE (mod->lang);
    ENNA_OBJECT_DEL (mod->edje);
    ENNA_FREE (mod);
}
