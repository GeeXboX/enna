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

#ifndef ENNA_H
#define ENNA_H

#define _GNU_SOURCE

#include <Ecore_Evas.h>
#include <Ecore.h>

#include "config.h"
#include "gettext.h"

//#define ENNA_DEBUG0
#define ENNA_DEBUG 2
//#define ENNA_DEBUG2

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define ENNA_TOSTRING(s) #s
#define ENNA_STRINGIFY(s) ENNA_TOSTRING(s)
#define ARRAY_NB_ELEMENTS(array) (sizeof (array) / sizeof (array[0]))
#define ENNA_FREE(p) do { free(p); p = NULL; } while (0)
#define ENNA_NEW(s, n) (s *)calloc(n, sizeof(s))
#define ENNA_FREE_LIST(list, free)			\
  do							\
    {							\
       if (list)					\
	 {						\
	    Eina_List *tmp;				\
	    tmp = list;					\
	    list = NULL;				\
	    while (tmp)					\
	      {						\
		 free(tmp->data);			\
		 tmp = eina_list_remove_list(tmp, tmp); \
	      }						\
	 }						\
    }							\
  while (0)

#define ENNA_TIMER_DEL(timer)                           \
    if (timer)                                          \
    {                                                   \
        ecore_timer_del(timer);                         \
        timer = NULL;                                   \
    }                                                   \

#define ENNA_EVENT_HANDLER_DEL(event_handler)           \
    if (event_handler)                                  \
    {                                                   \
        ecore_event_handler_del(event_handler);         \
        event_handler = NULL;                           \
    }                                                   \

#define ENNA_STRINGSHARE_DEL(string)                    \
    if (string)                                         \
    {                                                   \
        eina_stringshare_del(string);                   \
        string = NULL;                                  \
    }                                                   \

#define ENNA_OBJECT_DEL(obj)                            \
    if (obj) {                                          \
        evas_object_del(obj);                           \
        obj = NULL;                                     \
    }                                                   \

#define API_ENTRY                                       \
   Smart_Data *sd;                                      \
   sd = evas_object_smart_data_get(obj);                \
   if ((!obj) || (!sd) ||                               \
     (evas_object_type_get(obj) &&                      \
     strcmp(evas_object_type_get(obj), SMART_NAME)))

#define INTERNAL_ENTRY                                  \
   Smart_Data *sd;                                      \
   sd = evas_object_smart_data_get(obj);                \
   if (!sd) \
      return;

#define INTERNAL_ENTRY_RETURN                           \
   Smart_Data *sd;                                      \
   sd = evas_object_smart_data_get(obj);                \
   if (!sd) \
      return

typedef enum
{
    ENNA_MSG_NONE, /* no error messages */
    ENNA_MSG_EVENT, /* notify each incoming event */
    ENNA_MSG_INFO, /* working operations */
    ENNA_MSG_WARNING, /* harmless failures */
    ENNA_MSG_ERROR, /* may result in hazardous behavior */
    ENNA_MSG_CRITICAL, /* prevents lib from working */
} enna_msg_level_t;

/**
 * @struct Enna
 * @brief Main Enna struct, includes all stuct and vars
 */

typedef struct _Enna Enna;

struct _Enna
{
    Ecore_X_Window ee_winid; /**< Ecore_Evas WindowID */
    Evas *evas; /**< Main enna evas.  */
    Evas_Object *win; /** Main elementary window. */
    Evas_Object *o_background;/**< Background object, it handles key down. */
    Evas_Object *layout; /**< Main elementary layout widget. */
    Evas_Object *o_content; /** Edje Object to swallow content */
    Evas_Object *o_cursor; /** Edje Object for mouse cursor */
    enna_msg_level_t lvl; /**< Error message level */
    int slideshow_delay;
    int cursor_is_shown;
    int run_fullscreen;
    Ecore_Timer *mouse_idle_timer;
    Ecore_Timer *idle_timer;
};

extern Enna *enna;

/* Events */
int ENNA_EVENT_ACTIVITIES_CHANGED; /**< ev = NULL */

void enna_idle_timer_renew(void);

#endif /* ENNA_H */
