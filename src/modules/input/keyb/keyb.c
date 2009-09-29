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

#include <Ecore.h>

#include "enna.h"
#include "enna_config.h"
#include "logs.h"
#include "input.h"
#include "module.h"

#define ENNA_MODULE_NAME "input_keyb"


Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME,
    "Keyboard controls",
    NULL,
    "Module to control enna from the keyboard",
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
    
};

static const struct
{
    const char *keyname;
    const char *modifier;
    enna_input input;
} enna_keymap[] = {
    { "Left",         0,              ENNA_INPUT_LEFT          },
    { "Right",        0,              ENNA_INPUT_RIGHT         },
    { "Up",           0,              ENNA_INPUT_UP            },
    { "KP_Up",        0,              ENNA_INPUT_UP            },
    { "Down",         0,              ENNA_INPUT_DOWN          },
    { "KP_Down",      0,              ENNA_INPUT_DOWN          },
    { "Home",         0,              ENNA_INPUT_HOME          },
    { "KP_Home",      0,              ENNA_INPUT_HOME          },
    { "End",          0,              ENNA_INPUT_END           },
    { "KP_End",       0,              ENNA_INPUT_END           },
    { "Prior",        0,              ENNA_INPUT_PREV          },
    { "Next",         0,              ENNA_INPUT_NEXT          },
    { "Return",       0,              ENNA_INPUT_OK            },
    { "KP_Enter",     0,              ENNA_INPUT_OK            },
    { "space",        0,              ENNA_INPUT_OK            },
    //~ { "Stop",         0,              ENNA_KEY_STOP         }, stop on the keyboard? multimedia keyb?
    { "BackSpace",    0,              ENNA_INPUT_EXIT          },
    
    { "Escape",       0,              ENNA_INPUT_QUIT          },
    { "Super_L",      0,              ENNA_INPUT_MENU          },
    { "Meta_L",       0,              ENNA_INPUT_MENU          },
    { "Hyper_L",      0,              ENNA_INPUT_MENU          },
    { "plus",         0,              ENNA_INPUT_PLUS          },
    { "KP_Add",       0,              ENNA_INPUT_PLUS          },
    { "minus",        0,              ENNA_INPUT_MINUS         },
    { "KP_Subtract",  0,              ENNA_INPUT_MINUS         },
    { "f",            "Control",      ENNA_INPUT_FULLSCREEN    },
    { "0",            0,              ENNA_INPUT_KEY_0         },
    { "KP_0",         0,              ENNA_INPUT_KEY_0         },
    { "1",            0,              ENNA_INPUT_KEY_1         },
    { "KP_1",         0,              ENNA_INPUT_KEY_1         },
    { "2",            0,              ENNA_INPUT_KEY_2         },
    { "KP_2",         0,              ENNA_INPUT_KEY_2         },
    { "3",            0,              ENNA_INPUT_KEY_3         },
    { "KP_3",         0,              ENNA_INPUT_KEY_3         },
    { "4",            0,              ENNA_INPUT_KEY_4         },
    { "KP_4",         0,              ENNA_INPUT_KEY_4         },
    { "5",            0,              ENNA_INPUT_KEY_5         },
    { "KP_5",         0,              ENNA_INPUT_KEY_5         },
    { "6",            0,              ENNA_INPUT_KEY_6         },
    { "KP_6",         0,              ENNA_INPUT_KEY_6         },
    { "7",            0,              ENNA_INPUT_KEY_7         },
    { "KP_7",         0,              ENNA_INPUT_KEY_7         },
    { "8",            0,              ENNA_INPUT_KEY_8         },
    { "KP_8",         0,              ENNA_INPUT_KEY_8         },
    { "9",            0,              ENNA_INPUT_KEY_9         },
    { "KP_9",         0,              ENNA_INPUT_KEY_9         },
    { "a",            0,              ENNA_INPUT_KEY_A         },
    { "b",            0,              ENNA_INPUT_KEY_B         },
    { "c",            0,              ENNA_INPUT_KEY_C         },
    { "d",            0,              ENNA_INPUT_KEY_D         },
    { "e",            0,              ENNA_INPUT_KEY_E         },
    { "f",            0,              ENNA_INPUT_KEY_F         },
    { "g",            0,              ENNA_INPUT_KEY_G         },
    { "h",            0,              ENNA_INPUT_KEY_H         },
    { "i",            0,              ENNA_INPUT_KEY_I         },
    { "j",            0,              ENNA_INPUT_KEY_J         },
    { "k",            0,              ENNA_INPUT_KEY_K         },
    { "l",            0,              ENNA_INPUT_KEY_L         },
    { "m",            0,              ENNA_INPUT_KEY_M         },
    { "n",            0,              ENNA_INPUT_KEY_N         },
    { "o",            0,              ENNA_INPUT_KEY_O         },
    { "p",            0,              ENNA_INPUT_KEY_P         },
    { "q",            0,              ENNA_INPUT_KEY_Q         },
    { "r",            0,              ENNA_INPUT_KEY_R         },
    { "s",            0,              ENNA_INPUT_KEY_S         },
    { "t",            0,              ENNA_INPUT_KEY_T         },
    { "u",            0,              ENNA_INPUT_KEY_U         },
    { "v",            0,              ENNA_INPUT_KEY_V         },
    { "w",            0,              ENNA_INPUT_KEY_W         },
    { "x",            0,              ENNA_INPUT_KEY_X         },
    { "y",            0,              ENNA_INPUT_KEY_Y         },
    { "z",            0,              ENNA_INPUT_KEY_Z         },
    { NULL,           0,              ENNA_INPUT_UNKNOWN       }
};

static enna_input
_get_input_from_event(void *event)
{
    int i;
    Evas_Event_Key_Down *ev;

    ev = event;

    if (!ev) return ENNA_INPUT_UNKNOWN;

    for (i = 0; enna_keymap[i].keyname; i++)
    {
        /* Test First if modifer is set and is different than "None"*/
        if (enna_keymap[i].modifier
            && evas_key_modifier_is_set (ev->modifiers, enna_keymap[i].modifier)
            && !strcmp (enna_keymap[i].keyname, ev->key) )
        {
            enna_log (ENNA_MSG_EVENT, NULL, "Key pressed : [%s] + %s",
                enna_keymap[i].modifier, enna_keymap[i] );
            return enna_keymap[i].input;
        }
        /* Else just test if keyname match */
        else if (!enna_keymap[i].modifier && !strcmp (enna_keymap[i].keyname, ev->key))
        {
            enna_log (ENNA_MSG_EVENT, NULL, "Key pressed : %s",
                enna_keymap[i].keyname );
            return enna_keymap[i].input;
        }
    }

    return ENNA_INPUT_UNKNOWN;
}

static void _event_key_down_cb(void *data, Evas *e,
                                  Evas_Object *obj, void *event)
{
    enna_input in;

    enna_idle_timer_renew();
    in = _get_input_from_event(event);
    if (in != ENNA_INPUT_UNKNOWN)
        enna_input_event_emit(in);
}

/* Module interface */

void module_init(Enna_Module *em)
{
    evas_object_event_callback_add(enna->win, EVAS_CALLBACK_KEY_DOWN,
                                   _event_key_down_cb, enna);
    evas_object_focus_set(enna->win, 1);
}

void module_shutdown(Enna_Module *em)
{
    printf(" @@ KEYB SHUTDOWN\n");
    // TODO remove event callback
}
