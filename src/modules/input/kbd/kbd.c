/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2010 The Enna Project
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
    { "Super_R",      ECORE_NONE,              ENNA_INPUT_MENU          },
    { "Meta_R",       ECORE_NONE,              ENNA_INPUT_MENU          },
    { "Hyper_R",      ECORE_NONE,              ENNA_INPUT_MENU          },
    { "Escape",       ECORE_NONE,              ENNA_INPUT_QUIT          },
    { "BackSpace",    ECORE_NONE,              ENNA_INPUT_EXIT          },
    { "Return",       ECORE_NONE,              ENNA_INPUT_OK            },
    { "KP_Enter",     ECORE_NONE,              ENNA_INPUT_OK            },
    { "Super_L",      ECORE_NONE,              ENNA_INPUT_HOME          },
    { "Meta_L",       ECORE_NONE,              ENNA_INPUT_HOME          },
    { "Hyper_L",      ECORE_NONE,              ENNA_INPUT_HOME          },

    { "Left",         ECORE_NONE,              ENNA_INPUT_LEFT          },
    { "Right",        ECORE_NONE,              ENNA_INPUT_RIGHT         },
    { "Up",           ECORE_NONE,              ENNA_INPUT_UP            },
    { "KP_Up",        ECORE_NONE,              ENNA_INPUT_UP            },
    { "Down",         ECORE_NONE,              ENNA_INPUT_DOWN          },
    { "KP_Down",      ECORE_NONE,              ENNA_INPUT_DOWN          },

    { "Next",         ECORE_NONE,              ENNA_INPUT_NEXT          },
    { "Prior",        ECORE_NONE,              ENNA_INPUT_PREV          },
    { "Home",         ECORE_NONE,              ENNA_INPUT_FIRST         },
    { "KP_Home",      ECORE_NONE,              ENNA_INPUT_FIRST         },
    { "End",          ECORE_NONE,              ENNA_INPUT_LAST          },
    { "KP_End",       ECORE_NONE,              ENNA_INPUT_LAST          },

    { "f",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    },
    { "F",            ECORE_CTRL,              ENNA_INPUT_FULLSCREEN    },
    { "i",            ECORE_CTRL,              ENNA_INPUT_INFO          },
    { "I",            ECORE_CTRL,              ENNA_INPUT_INFO          },
    { "w",            ECORE_CTRL,              ENNA_INPUT_FRAMEDROP     },
    { "W",            ECORE_CTRL,              ENNA_INPUT_FRAMEDROP     },
    { "r",            ECORE_CTRL,              ENNA_INPUT_ROTATE        },
    { "R",            ECORE_CTRL,              ENNA_INPUT_ROTATE        },

    /* Player controls */
    { "space",        ECORE_CTRL,              ENNA_INPUT_PLAY          },
    { "v",            ECORE_CTRL,              ENNA_INPUT_STOP          },
    { "V",            ECORE_CTRL,              ENNA_INPUT_STOP          },
    { "b",            ECORE_CTRL,              ENNA_INPUT_PAUSE         },
    { "B",            ECORE_CTRL,              ENNA_INPUT_PAUSE         },
    { "h",            ECORE_CTRL,              ENNA_INPUT_FORWARD       },
    { "H",            ECORE_CTRL,              ENNA_INPUT_FORWARD       },
    { "g",            ECORE_CTRL,              ENNA_INPUT_REWIND        },
    { "G",            ECORE_CTRL,              ENNA_INPUT_REWIND        },
    { "n",            ECORE_CTRL,              ENNA_INPUT_RECORD        },
    { "N",            ECORE_CTRL,              ENNA_INPUT_RECORD        },

    /* Audio controls */
    { "plus",         ECORE_NONE,              ENNA_INPUT_VOLPLUS           },
    { "plus",         ECORE_SHIFT,             ENNA_INPUT_VOLPLUS           },
    { "KP_Add",       ECORE_NONE,              ENNA_INPUT_VOLPLUS           },
    { "minus",        ECORE_NONE,              ENNA_INPUT_VOLMINUS          },
    { "KP_Subtract",  ECORE_NONE,              ENNA_INPUT_VOLMINUS          },
    { "m",            ECORE_CTRL,              ENNA_INPUT_MUTE              },
    { "M",            ECORE_CTRL,              ENNA_INPUT_MUTE              },
    { "k",            ECORE_CTRL,              ENNA_INPUT_AUDIO_PREV        },
    { "K",            ECORE_CTRL,              ENNA_INPUT_AUDIO_PREV        },
    { "l",            ECORE_CTRL,              ENNA_INPUT_AUDIO_NEXT        },
    { "L",            ECORE_CTRL,              ENNA_INPUT_AUDIO_NEXT        },
    { "p",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_PLUS  },
    { "P",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_PLUS  },
    { "o",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_MINUS },
    { "O",            ECORE_CTRL,              ENNA_INPUT_AUDIO_DELAY_MINUS },

    /* Subtitles controls */
    { "s",            ECORE_CTRL,              ENNA_INPUT_SUBTITLES        },
    { "S",            ECORE_CTRL,              ENNA_INPUT_SUBTITLES        },
    { "g",            ECORE_CTRL,              ENNA_INPUT_SUBS_PREV        },
    { "G",            ECORE_CTRL,              ENNA_INPUT_SUBS_PREV        },
    { "y",            ECORE_CTRL,              ENNA_INPUT_SUBS_NEXT        },
    { "Y",            ECORE_CTRL,              ENNA_INPUT_SUBS_NEXT        },
    { "a",            ECORE_CTRL,              ENNA_INPUT_SUBS_ALIGN       },
    { "A",            ECORE_CTRL,              ENNA_INPUT_SUBS_ALIGN       },
    { "t",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_PLUS    },
    { "T",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_PLUS    },
    { "r",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_MINUS   },
    { "R",            ECORE_CTRL,              ENNA_INPUT_SUBS_POS_MINUS   },
    { "j",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_PLUS  },
    { "J",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_PLUS  },
    { "i",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_MINUS },
    { "I",            ECORE_CTRL,              ENNA_INPUT_SUBS_SCALE_MINUS },
    { "x",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_PLUS  },
    { "X",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_PLUS  },
    { "z",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_MINUS },
    { "Z",            ECORE_CTRL,              ENNA_INPUT_SUBS_DELAY_MINUS },

    /* TV controls */
    { "F1",           ECORE_NONE,              ENNA_INPUT_RED           },
    { "F2",           ECORE_NONE,              ENNA_INPUT_GREEN         },
    { "F3",           ECORE_NONE,              ENNA_INPUT_YELLOW        },
    { "F4",           ECORE_NONE,              ENNA_INPUT_BLUE          },
    { "c",            ECORE_CTRL,              ENNA_INPUT_CHANPREV      },
    { "C",            ECORE_CTRL,              ENNA_INPUT_CHANPREV      },
    { "F5",           ECORE_NONE,              ENNA_INPUT_SCHEDULE      },
    { "F6",           ECORE_NONE,              ENNA_INPUT_CHANNELS      },
    { "F7",           ECORE_NONE,              ENNA_INPUT_TIMERS        },
    { "F8",           ECORE_NONE,              ENNA_INPUT_RECORDINGS    },

    /* Special characters */
    { "space",        ECORE_NONE,              ENNA_INPUT_KEY_SPACE     },

    /* Number characters */
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

    /* Alphabetical characters */
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

    /* discard some modifiers */
    if (ev->modifiers >= ECORE_EVENT_LOCK_CAPS)
        ev->modifiers -= ECORE_EVENT_LOCK_CAPS;
    if (ev->modifiers >= ECORE_EVENT_LOCK_NUM)
        ev->modifiers -= ECORE_EVENT_LOCK_NUM;
    if (ev->modifiers >= ECORE_EVENT_LOCK_SCROLL)
        ev->modifiers -= ECORE_EVENT_LOCK_SCROLL;

    return (ev->modifiers && ev->modifiers < ECORE_LAST) ?
      _input_event_modifier(ev) : _input_event(ev);
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

static void
module_init(Enna_Module *em)
{
    key_down_event_handler =
        ecore_event_handler_add (ECORE_EVENT_KEY_DOWN, _ecore_event_key_down_cb, NULL);
}

static void
module_shutdown(Enna_Module *em)
{
    ENNA_EVENT_HANDLER_DEL(key_down_event_handler);
}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME,
    N_("Keyboard controls"),
    NULL,
    N_("Module to control enna from the keyboard"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};
