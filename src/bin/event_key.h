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

#ifndef EVENT_KEY_H
#define EVENT_KEY_H

#include "enna.h"

typedef enum
{
    ENNA_KEY_UNKNOWN,

    ENNA_KEY_MENU,
    ENNA_KEY_QUIT,

    ENNA_KEY_LEFT,
    ENNA_KEY_RIGHT,
    ENNA_KEY_UP,
    ENNA_KEY_DOWN,

    ENNA_KEY_HOME,
    ENNA_KEY_END,
    ENNA_KEY_PAGE_UP,
    ENNA_KEY_PAGE_DOWN,
    ENNA_KEY_OK,
    ENNA_KEY_CANCEL,
    ENNA_KEY_SPACE,
    ENNA_KEY_FULLSCREEN,

    ENNA_KEY_0,
    ENNA_KEY_1,
    ENNA_KEY_2,
    ENNA_KEY_3,
    ENNA_KEY_4,
    ENNA_KEY_5,
    ENNA_KEY_6,
    ENNA_KEY_7,
    ENNA_KEY_8,
    ENNA_KEY_9,

    /* Alphabetical characters */
    ENNA_KEY_A,
    ENNA_KEY_B,
    ENNA_KEY_C,
    ENNA_KEY_D,
    ENNA_KEY_E,
    ENNA_KEY_F,
    ENNA_KEY_G,
    ENNA_KEY_H,
    ENNA_KEY_I,
    ENNA_KEY_J,
    ENNA_KEY_K,
    ENNA_KEY_L,
    ENNA_KEY_M,
    ENNA_KEY_N,
    ENNA_KEY_O,
    ENNA_KEY_P,
    ENNA_KEY_Q,
    ENNA_KEY_R,
    ENNA_KEY_S,
    ENNA_KEY_T,
    ENNA_KEY_U,
    ENNA_KEY_V,
    ENNA_KEY_W,
    ENNA_KEY_X,
    ENNA_KEY_Y,
    ENNA_KEY_Z,
} enna_key_t;

typedef struct _Enna_Class_Input Enna_Class_Input;

struct _Enna_Class_Input
{
    const char *name;
    struct
    {
        void (*class_init)(int dummy);
        void (*class_shutdown)(int dummy);
        void (*class_event_cb_set)(void (*event_cb)(void*data, char *event), void *data);
    } func;

};

/* Input ecore event */
int ENNA_EVENT_INPUT_KEY_DOWN;

/* Enna Event API functions */
enna_key_t enna_get_key (void *event);
int enna_key_is_alpha(enna_key_t key);
char enna_key_get_alpha(enna_key_t key);
void enna_input_init();
void enna_input_shutdown();
int
        enna_input_class_register(Enna_Module *module, Enna_Class_Input *class);

#endif /* EVENT_KEY_H */
