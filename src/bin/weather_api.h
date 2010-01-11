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

void enna_weather_cfg_register (void);
void enna_weather_parse_config (weather_t *w);
void enna_weather_update       (weather_t *w);
void enna_weather_set_city     (weather_t *w, const char *city);
void enna_weather_set_lang     (weather_t *w, const char *lang);

weather_t *enna_weather_new (void);
void enna_weather_free (weather_t *w);

void enna_weather_init(void);
void enna_weather_shutdown(void);

#endif /* WEATHER_API_H */
