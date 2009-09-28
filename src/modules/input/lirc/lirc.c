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

#include <unistd.h>
#include <fcntl.h>

#include <lirc/lirc_client.h>

#include <Ecore.h>
#include <Elementary.h>

#include "enna.h"
#include "enna_config.h"
#include "logs.h"
#include "input.h"
#include "module.h"

#define ENNA_MODULE_NAME "input_lirc"


typedef struct _Enna_Module_Lirc Enna_Module_Lirc;

struct _Enna_Module_Lirc
{
    Enna_Module *em;
    Ecore_Fd_Handler *fd_handler;
    struct lirc_config *lirc_config;
};

static Enna_Module_Lirc *mod;
static Enna_Config_Panel *_config_panel = NULL;
static Evas_Object *_o_main = NULL;

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_NAME
};


static const struct
{
    const char *keyname;
    enna_input input;
} enna_lircmap[] = {
    { "Left",         ENNA_INPUT_LEFT        },
    { "Right",        ENNA_INPUT_RIGHT       },
    { "Up",           ENNA_INPUT_UP          },
    { "Down",         ENNA_INPUT_DOWN        },
    { "Home",         ENNA_INPUT_HOME        },
    { "Ok",           ENNA_INPUT_OK          },
    { "Prev",         ENNA_INPUT_PREV        },
    { "Next",         ENNA_INPUT_NEXT        },

    { "Stop",         ENNA_INPUT_STOP        },
    { "Exit",         ENNA_INPUT_EXIT        },

    { "Quit",         ENNA_INPUT_QUIT        },
    { "Menu",         ENNA_INPUT_MENU        },

    { "Plus",         ENNA_INPUT_PLUS        },
    { "Minus",        ENNA_INPUT_MINUS       },

    { "0",            ENNA_INPUT_KEY_0       },
    { "1",            ENNA_INPUT_KEY_1       },
    { "2",            ENNA_INPUT_KEY_2       },
    { "3",            ENNA_INPUT_KEY_3       },
    { "4",            ENNA_INPUT_KEY_4       },
    { "5",            ENNA_INPUT_KEY_5       },
    { "6",            ENNA_INPUT_KEY_6       },
    { "7",            ENNA_INPUT_KEY_7       },
    { "8",            ENNA_INPUT_KEY_8       },
    { "9",            ENNA_INPUT_KEY_9       },

    { "a",            ENNA_INPUT_KEY_A       },
    { "b",            ENNA_INPUT_KEY_B       },
    { "c",            ENNA_INPUT_KEY_C       },
    { "d",            ENNA_INPUT_KEY_D       },
    { "e",            ENNA_INPUT_KEY_E       },
    { "f",            ENNA_INPUT_KEY_F       },
    { "g",            ENNA_INPUT_KEY_G       },
    { "h",            ENNA_INPUT_KEY_H       },
    { "i",            ENNA_INPUT_KEY_I       },
    { "j",            ENNA_INPUT_KEY_J       },
    { "k",            ENNA_INPUT_KEY_K       },
    { "l",            ENNA_INPUT_KEY_L       },
    { "m",            ENNA_INPUT_KEY_M       },
    { "n",            ENNA_INPUT_KEY_N       },
    { "o",            ENNA_INPUT_KEY_O       },
    { "p",            ENNA_INPUT_KEY_P       },
    { "q",            ENNA_INPUT_KEY_Q       },
    { "r",            ENNA_INPUT_KEY_R       },
    { "s",            ENNA_INPUT_KEY_S       },
    { "t",            ENNA_INPUT_KEY_T       },
    { "u",            ENNA_INPUT_KEY_U       },
    { "v",            ENNA_INPUT_KEY_V       },
    { "w",            ENNA_INPUT_KEY_W       },
    { "x",            ENNA_INPUT_KEY_X       },
    { "y",            ENNA_INPUT_KEY_Y       },
    { "z",            ENNA_INPUT_KEY_Z       },

    { NULL,           ENNA_INPUT_UNKNOWN     }
};


static enna_input
_get_input_from_event(const char *ev)
{
    int i;

    for (i = 0; enna_lircmap[i].keyname; i++)
    {
        if (!strcmp(enna_lircmap[i].keyname, ev))
          return enna_lircmap[i].input;
    }

    enna_log(ENNA_MSG_WARNING, NULL, "Unrecognized lirc key: '%s'", ev);
    // TODO here we could print a list of recognized keys
    return ENNA_INPUT_UNKNOWN;
}

static int _lirc_code_received(void *data, Ecore_Fd_Handler * fd_handler)
{
    char *code, *event;
    int ret = -1;

    while (lirc_nextcode(&code) == 0 && code != NULL)
    {
        while ((ret = lirc_code2char(mod->lirc_config, code, &event)) == 0
                && event != NULL)
        {
            enna_input in;

            in = _get_input_from_event(event);
            if (in != ENNA_INPUT_UNKNOWN)
                enna_input_event_emit(in);
        }
    }
    return 1;
}

/* Config Panel */

static Evas_Object *
lirc_panel_show(void *data)
{
    _o_main = elm_label_add(enna->layout);
    elm_label_label_set(_o_main, "Remote config panel !! TODO :P");
    elm_object_scale_set(_o_main, 6.0);
    evas_object_size_hint_align_set(_o_main, -1.0, -1.0);
    evas_object_size_hint_weight_set(_o_main, 1.0, 1.0);
    evas_object_show(_o_main);

    return _o_main;
}

static void
lirc_panel_hide(void *data)
{
    ENNA_OBJECT_DEL(_o_main);
}

/* Module interface */

void module_init(Enna_Module *em)
{
    struct lirc_config *config;
    int fd;

    if (!em) return;

    mod = calloc(1, sizeof(Enna_Module_Lirc));
    if (!mod) return;
    mod->em = em;
    em->mod = mod;

    // initialize lirc
    if ((fd = lirc_init("enna", 1)) == -1) // TODO need to close this? or ecore_fd_hand_del is enoght?
    {
        enna_log(ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                "could not initialize LIRC support");
        return;
    }
    if (lirc_readconfig(NULL, &config, NULL) != 0)
    {
        lirc_deinit();
        enna_log(ENNA_MSG_ERROR, ENNA_MODULE_NAME,
                "could not find Lirc config file");
        return;
    }
    mod->lirc_config = config;

    // connect to the lirc socket
    fcntl(fd, F_SETFL, O_NONBLOCK);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    mod->fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ,
            _lirc_code_received, NULL, NULL, NULL);

    // register the configuration panel
    _config_panel = enna_config_panel_register(_("Remote"), "icon/music",
                                    lirc_panel_show, lirc_panel_hide, NULL);
}

void module_shutdown(Enna_Module *em)
{
    enna_config_panel_unregister(_config_panel);
    if (mod->fd_handler)
    {
        lirc_freeconfig(mod->lirc_config);
        ecore_main_fd_handler_del(mod->fd_handler);
        lirc_deinit();
    }
}
