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

#ifndef ENNA_INPUT_H
#define ENNA_INPUT_H

#include "enna.h"

#define ENNA_EVENT_BLOCK ECORE_CALLBACK_CANCEL
#define ENNA_EVENT_CONTINUE ECORE_CALLBACK_RENEW

typedef struct _Input_Listener Input_Listener;

typedef enum
{
    ENNA_INPUT_UNKNOWN,

    ENNA_INPUT_MENU,
    ENNA_INPUT_QUIT,
    ENNA_INPUT_EXIT,
    //~ ENNA_KEY_CANCEL, isn't the same as 'exit' ??
    ENNA_INPUT_OK,
    ENNA_INPUT_SPACE,

    ENNA_INPUT_LEFT,
    ENNA_INPUT_RIGHT,
    ENNA_INPUT_UP,
    ENNA_INPUT_DOWN,

    ENNA_INPUT_HOME,
    ENNA_INPUT_END,

    ENNA_INPUT_NEXT,
    ENNA_INPUT_PREV,

    ENNA_INPUT_PLAY,
    ENNA_INPUT_STOP,
    ENNA_INPUT_PAUSE,
    ENNA_INPUT_FORWARD,
    ENNA_INPUT_REWIND,

    ENNA_INPUT_FULLSCREEN,
    ENNA_INPUT_PLUS,
    ENNA_INPUT_MINUS,
    ENNA_INPUT_MUTE,

    ENNA_INPUT_KEY_0,
    ENNA_INPUT_KEY_1,
    ENNA_INPUT_KEY_2,
    ENNA_INPUT_KEY_3,
    ENNA_INPUT_KEY_4,
    ENNA_INPUT_KEY_5,
    ENNA_INPUT_KEY_6,
    ENNA_INPUT_KEY_7,
    ENNA_INPUT_KEY_8,
    ENNA_INPUT_KEY_9,

    /* Alphabetical characters */
    ENNA_INPUT_KEY_A,
    ENNA_INPUT_KEY_B,
    ENNA_INPUT_KEY_C,
    ENNA_INPUT_KEY_D,
    ENNA_INPUT_KEY_E,
    ENNA_INPUT_KEY_F,
    ENNA_INPUT_KEY_G,
    ENNA_INPUT_KEY_H,
    ENNA_INPUT_KEY_I,
    ENNA_INPUT_KEY_J,
    ENNA_INPUT_KEY_K,
    ENNA_INPUT_KEY_L,
    ENNA_INPUT_KEY_M,
    ENNA_INPUT_KEY_N,
    ENNA_INPUT_KEY_O,
    ENNA_INPUT_KEY_P,
    ENNA_INPUT_KEY_Q,
    ENNA_INPUT_KEY_R,
    ENNA_INPUT_KEY_S,
    ENNA_INPUT_KEY_T,
    ENNA_INPUT_KEY_U,
    ENNA_INPUT_KEY_V,
    ENNA_INPUT_KEY_W,
    ENNA_INPUT_KEY_X,
    ENNA_INPUT_KEY_Y,
    ENNA_INPUT_KEY_Z,
} enna_input;


/* Enna Event API functions */
Eina_Bool enna_input_event_emit(enna_input in);

Input_Listener *enna_input_listener_add(const char *name, Eina_Bool (*func)(void *data, enna_input event), void *data);
void enna_input_listener_promote(Input_Listener *il);
void enna_input_listener_demote(Input_Listener *il);
void enna_input_listener_del(Input_Listener *il);

#endif /* ENNA_INPUT_H */
