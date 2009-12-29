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

#include <Ecore.h>

#include "enna.h"
#include "enna_config.h"
#include "logs.h"
#include "input.h"
#include "module.h"

#define ENNA_MODULE_NAME "input_kbd"


static const struct
{
    const char *keyname;
    Ecore_Event_Modifier modifier;
    enna_input input;
} enna_keymap[] = {
    { "Left",         ECORE_NONE,              ENNA_INPUT_LEFT          },
    { "Right",        ECORE_NONE,              ENNA_INPUT_RIGHT         },
    { "Up",           ECORE_NONE,              ENNA_INPUT_UP            },
    { "KP_Up",        ECORE_NONE,              ENNA_INPUT_UP            },
    { "Down",         ECORE_NONE,              ENNA_INPUT_DOWN          },
    { "KP_Down",      ECORE_NONE,              ENNA_INPUT_DOWN          },
    { "Home",         ECORE_NONE,              ENNA_INPUT_HOME          },
    { "KP_Home",      ECORE_NONE,              ENNA_INPUT_HOME          },
    { "End",          ECORE_NONE,              ENNA_INPUT_END           },
    { "KP_End",       ECORE_NONE,              ENNA_INPUT_END           },
    { "Prior",        ECORE_NONE,              ENNA_INPUT_PREV          },
    { "Next",         ECORE_NONE,              ENNA_INPUT_NEXT          },
    { "Return",       ECORE_NONE,              ENNA_INPUT_OK            },
    { "KP_Enter",     ECORE_NONE,              ENNA_INPUT_OK            },
    { "space",        ECORE_NONE,              ENNA_INPUT_SPACE         },
    //~ { "Stop",         ECORE_NONE,              ENNA_KEY_STOP         }, stop on the keyboard? multimedia keyb?
    { "BackSpace",    ECORE_NONE,              ENNA_INPUT_EXIT          },
    
    { "Escape",       ECORE_NONE,              ENNA_INPUT_QUIT          },
    { "Super_L",      ECORE_NONE,              ENNA_INPUT_MENU          },
    { "Meta_L",       ECORE_NONE,              ENNA_INPUT_MENU          },
    { "Hyper_L",      ECORE_NONE,              ENNA_INPUT_MENU          },
    { "plus",         ECORE_NONE,              ENNA_INPUT_PLUS          },
    { "plus",         ECORE_SHIFT,             ENNA_INPUT_PLUS          },
    { "KP_Add",       ECORE_NONE,              ENNA_INPUT_PLUS          },
    { "minus",        ECORE_NONE,              ENNA_INPUT_MINUS         },
    { "KP_Subtract",  ECORE_NONE,              ENNA_INPUT_MINUS         },
    { "m",            ECORE_CTRL,              ENNA_INPUT_MUTE          },
    { "f",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    },
    { "0",            ECORE_NONE,              ENNA_INPUT_KEY_0         },
    { "KP_0",         ECORE_NONE,              ENNA_INPUT_KEY_0         },
    { "1",            ECORE_NONE,              ENNA_INPUT_KEY_1         },
    { "KP_1",         ECORE_NONE,              ENNA_INPUT_KEY_1         },
    { "2",            ECORE_NONE,              ENNA_INPUT_KEY_2         },
    { "KP_2",         ECORE_NONE,              ENNA_INPUT_KEY_2         },
    { "3",            ECORE_NONE,              ENNA_INPUT_KEY_3         },
    { "KP_3",         ECORE_NONE,              ENNA_INPUT_KEY_3         },
    { "4",            ECORE_NONE,              ENNA_INPUT_KEY_4         },
    { "KP_4",         ECORE_NONE,              ENNA_INPUT_KEY_4         },
    { "5",            ECORE_NONE,              ENNA_INPUT_KEY_5         },
    { "KP_5",         ECORE_NONE,              ENNA_INPUT_KEY_5         },
    { "6",            ECORE_NONE,              ENNA_INPUT_KEY_6         },
    { "KP_6",         ECORE_NONE,              ENNA_INPUT_KEY_6         },
    { "7",            ECORE_NONE,              ENNA_INPUT_KEY_7         },
    { "KP_7",         ECORE_NONE,              ENNA_INPUT_KEY_7         },
    { "8",            ECORE_NONE,              ENNA_INPUT_KEY_8         },
    { "KP_8",         ECORE_NONE,              ENNA_INPUT_KEY_8         },
    { "9",            ECORE_NONE,              ENNA_INPUT_KEY_9         },
    { "KP_9",         ECORE_NONE,              ENNA_INPUT_KEY_9         },
    { "a",            ECORE_NONE,              ENNA_INPUT_KEY_A         },
    { "b",            ECORE_NONE,              ENNA_INPUT_KEY_B         },
    { "c",            ECORE_NONE,              ENNA_INPUT_KEY_C         },
    { "d",            ECORE_NONE,              ENNA_INPUT_KEY_D         },
    { "e",            ECORE_NONE,              ENNA_INPUT_KEY_E         },
    { "f",            ECORE_NONE,              ENNA_INPUT_KEY_F         },
    { "g",            ECORE_NONE,              ENNA_INPUT_KEY_G         },
    { "h",            ECORE_NONE,              ENNA_INPUT_KEY_H         },
    { "i",            ECORE_NONE,              ENNA_INPUT_KEY_I         },
    { "j",            ECORE_NONE,              ENNA_INPUT_KEY_J         },
    { "k",            ECORE_NONE,              ENNA_INPUT_KEY_K         },
    { "l",            ECORE_NONE,              ENNA_INPUT_KEY_L         },
    { "m",            ECORE_NONE,              ENNA_INPUT_KEY_M         },
    { "n",            ECORE_NONE,              ENNA_INPUT_KEY_N         },
    { "o",            ECORE_NONE,              ENNA_INPUT_KEY_O         },
    { "p",            ECORE_NONE,              ENNA_INPUT_KEY_P         },
    { "q",            ECORE_NONE,              ENNA_INPUT_KEY_Q         },
    { "r",            ECORE_NONE,              ENNA_INPUT_KEY_R         },
    { "s",            ECORE_NONE,              ENNA_INPUT_KEY_S         },
    { "t",            ECORE_NONE,              ENNA_INPUT_KEY_T         },
    { "u",            ECORE_NONE,              ENNA_INPUT_KEY_U         },
    { "v",            ECORE_NONE,              ENNA_INPUT_KEY_V         },
    { "w",            ECORE_NONE,              ENNA_INPUT_KEY_W         },
    { "x",            ECORE_NONE,              ENNA_INPUT_KEY_X         },
    { "y",            ECORE_NONE,              ENNA_INPUT_KEY_Y         },
    { "z",            ECORE_NONE,              ENNA_INPUT_KEY_Z         },
    { NULL,           ECORE_NONE,              ENNA_INPUT_UNKNOWN       }
};



static Ecore_Event_Handler *key_down_event_handler;

static enna_input
_input_event_modifier (Ecore_Event_Key *ev)
{
    int i;

    for (i = 0; enna_keymap[i].keyname; i++)
    {
        if (ev->modifiers == enna_keymap[i].modifier &&
            !strcmp(enna_keymap[i].keyname, ev->key))
        {
            enna_log(ENNA_MSG_EVENT, NULL, "Key pressed : [%d] + %s",
                     enna_keymap[i].modifier, enna_keymap[i] );
            return enna_keymap[i].input;
        }
    }

    return ENNA_INPUT_UNKNOWN;
}

static enna_input
_input_event (Ecore_Event_Key *ev)
{
    int i;

    for (i = 0; enna_keymap[i].keyname; i++)
    {
        if ((enna_keymap[i].modifier == ECORE_NONE) &&
            !strcmp(enna_keymap[i].keyname, ev->key))
        {
            enna_log(ENNA_MSG_EVENT, NULL, "Key pressed : %s",
                     enna_keymap[i].keyname );
            return enna_keymap[i].input;
        }
    }

    return ENNA_INPUT_UNKNOWN;
}

static enna_input
_get_input_from_event(Ecore_Event_Key *ev)
{
    if (!ev)
        return ENNA_INPUT_UNKNOWN;

    return (ev->modifiers) ? _input_event_modifier(ev) : _input_event(ev);
}

static int
_ecore_event_key_down_cb(void *data, int type, void *event)
{
    Ecore_Event_Key *e = event;
    enna_input in;

    enna_idle_timer_renew();

    in = _get_input_from_event(e);
    if (in != ENNA_INPUT_UNKNOWN)
        enna_input_event_emit(in);

    return ECORE_CALLBACK_CANCEL;
}


/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_input_kbd
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME,
    N_("Keyboard controls"),
    NULL,
    N_("Module to control enna from the keyboard"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    key_down_event_handler =
        ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _ecore_event_key_down_cb, NULL);
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    ENNA_EVENT_HANDLER_DEL(key_down_event_handler);
}
