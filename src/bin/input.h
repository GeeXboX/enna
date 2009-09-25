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

/* Input ecore event */
int ENNA_EVENT_INPUT;

typedef enum
{
    ENNA_INPUT_UNKNOWN,

    ENNA_INPUT_MENU,
    ENNA_INPUT_QUIT,
    ENNA_INPUT_EXIT,
    ENNA_INPUT_OK,

    ENNA_INPUT_LEFT,
    ENNA_INPUT_RIGHT,
    ENNA_INPUT_UP,
    ENNA_INPUT_DOWN,

    ENNA_INPUT_HOME,
    ENNA_INPUT_END,
    ENNA_INPUT_PAGE_UP,
    ENNA_INPUT_PAGE_DOWN,
    //~ ENNA_KEY_OK,
    //~ ENNA_KEY_STOP,
    //~ ENNA_KEY_CANCEL,
    //~ ENNA_KEY_SPACE,
    ENNA_INPUT_FULLSCREEN,
    //~ ENNA_KEY_PLUS,
    //~ ENNA_KEY_MINUS,
//~ 
    //~ ENNA_KEY_0,
    //~ ENNA_KEY_1,
    //~ ENNA_KEY_2,
    //~ ENNA_KEY_3,
    //~ ENNA_KEY_4,
    //~ ENNA_KEY_5,
    //~ ENNA_KEY_6,
    //~ ENNA_KEY_7,
    //~ ENNA_KEY_8,
    //~ ENNA_KEY_9,
//~ 
    //~ /* Alphabetical characters */
    //~ ENNA_KEY_A,
    //~ ENNA_KEY_B,
    //~ ENNA_KEY_C,
    //~ ENNA_KEY_D,
    //~ ENNA_KEY_E,
    //~ ENNA_KEY_F,
    //~ ENNA_KEY_G,
    //~ ENNA_KEY_H,
    //~ ENNA_KEY_I,
    //~ ENNA_KEY_J,
    //~ ENNA_KEY_K,
    //~ ENNA_KEY_L,
    //~ ENNA_KEY_M,
    //~ ENNA_KEY_N,
    //~ ENNA_KEY_O,
    //~ ENNA_KEY_P,
    //~ ENNA_KEY_Q,
    //~ ENNA_KEY_R,
    //~ ENNA_KEY_S,
    //~ ENNA_KEY_T,
    //~ ENNA_KEY_U,
    //~ ENNA_KEY_V,
    //~ ENNA_KEY_W,
    //~ ENNA_KEY_X,
    //~ ENNA_KEY_Y,
    //~ ENNA_KEY_Z,
} enna_input;

/* Enna Event API functions */
void enna_input_init(void);
void enna_input_shutdown(void);
Eina_Bool enna_input_event_emit(enna_input in);

#endif /* ENNA_INPUT_H */
