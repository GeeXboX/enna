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

#ifndef ENNA_H
#define ENNA_H

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

#define ARRAY_NB_ELEMENTS(array) (sizeof (array) / sizeof (array[0]))
#define ENNA_FREE(p) { if (p) {free(p); p = NULL;} }
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
    char *home; /**< Home directory ie $HOME/.enna. */
    Ecore_Evas *ee; /**< Ecore_Evas. */
    Ecore_X_Window ee_winid; /**< Ecore_Evas WindowID */
    Evas *evas; /**< Main enna evas.  */
    Evas_Object *o_background;/**< Background object, it handles key down. */
    Evas_Object *o_edje; /**< Main edje. */
    Evas_Object *o_mainmenu; /**< Top menu. */
    Evas_Object *o_content; /** Edje Object to swallow content */
    Evas_Object *o_cursor; /** Edje Object for mouse cursor */
    enna_msg_level_t lvl; /**< Error message level */
    int use_network;
    int use_covers;
    int use_snapshots;
    int metadata_cache;
    int slideshow_delay;
    int cursor_is_shown;
    Ecore_Timer *mouse_idle_timer;
    Ecore_Timer *idle_timer;
    Ecore_Pipe *pipe_grabber;
};

extern Enna *enna;

void enna_idle_timer_renew(void);

#endif /* ENNA_H */
