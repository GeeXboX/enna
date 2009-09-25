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
#include <ctype.h>

#include <Ecore.h>
#include <Ecore_Data.h>

#include "enna.h"
#include "input.h"


/* Private Functions */
static void
_input_event_free_event_cb(void *data, void *ev)
{
    eina_stringshare_del(ev);
}

/* Public Functions */
void
enna_input_init(void)
{
    /* Create Enna Input Event*/
    ENNA_EVENT_INPUT = ecore_event_type_new();
}

void
enna_input_shutdown(void)
{
    
}

Eina_Bool
enna_input_event_emit(const char *event, void *data)
{
    printf("EMIT EVENT %s\n", event);
    
    // Old way (remove when all use the new way)
    evas_event_feed_key_down(enna->evas, event, event, event, NULL, ecore_time_get(), data);

    // new way
    ecore_event_add(ENNA_EVENT_INPUT, (void*)eina_stringshare_add(event),
                    _input_event_free_event_cb, NULL);
        
    enna_idle_timer_renew();
    return EINA_TRUE;
}
