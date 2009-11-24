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

#ifndef WEATHER_API_H
#define WEATHER_API_H

typedef enum {
    TEMP_CELCIUS,
    TEMP_FAHRENHEIT
} temp_type_t;

typedef struct weather_current_s {
    char *condition;
    char *temp;
    char *humidity;
    char *icon;
    char *wind;
} weather_current_t;

typedef struct weather_forecast_s {
    char *day;
    char *low;
    char *high;
    char *icon;
    char *condition;
} weather_forecast_t;

typedef struct weather_smart_data_s {
    char *city;
    char *lang;
    char *date;
    temp_type_t temp;
    weather_current_t current;
    weather_forecast_t forecast[4];
} weather_t;

void enna_weather_parse_config (weather_t *w);
void enna_weather_update       (weather_t *w);
void enna_weather_set_city     (weather_t *w, const char *city);
void enna_weather_set_lang     (weather_t *w, const char *lang);

weather_t *enna_weather_init (void);
void enna_weather_free (weather_t *w);

#endif /* WEATHER_API_H */
