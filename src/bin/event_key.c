
#include "enna.h"

static const struct {
  const char *keyname;
  enna_key_t keycode;
} enna_keymap[] = {
  { "m",                          ENNA_KEY_MENU          },
  { "q",                          ENNA_KEY_QUIT          },
  { "Left",                       ENNA_KEY_LEFT          },
  { "Right",                      ENNA_KEY_RIGHT         },
  { "Up",                         ENNA_KEY_UP            },
  { "Down",                       ENNA_KEY_DOWN          },
  { "Return",                     ENNA_KEY_OK            },
  { "KP_Enter",                   ENNA_KEY_OK            },
  { "BackSpace",                  ENNA_KEY_CANCEL        },
  { NULL,                         ENNA_KEY_UNKNOWN       }
};

enna_key_t
enna_get_key (Evas_Event_Key_Down *ev)
{
  int i;
  
  if (!ev)
    return ENNA_KEY_UNKNOWN;

  printf ("Key pressed : %s\n", ev->key);

  for (i = 0; enna_keymap[i].keyname; i++)
    if (!strcmp (enna_keymap[i].keyname, ev->key))
      return enna_keymap[i].keycode;

  return ENNA_KEY_UNKNOWN;
}
