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
