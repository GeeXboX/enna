
#include "enna.h"
#include "event_key.h"

typedef struct _Input_Module_Item Input_Module_Item;
struct _Input_Module_Item
{
   Enna_Module *module;
   Enna_Class_Input *class;
};

static Evas_List *_input_modules;



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
  { "KP_0",                       ENNA_KEY_0             },
  { "KP_1",                       ENNA_KEY_1             },
  { "KP_2",                       ENNA_KEY_2             },
  { "KP_3",                       ENNA_KEY_3             },
  { "KP_4",                       ENNA_KEY_4             },
  { "KP_5",                       ENNA_KEY_5             },
  { "KP_6",                       ENNA_KEY_6             },
  { "KP_7",                       ENNA_KEY_7             },
  { "KP_8",                       ENNA_KEY_8             },
  { "KP_9",                       ENNA_KEY_9             },
  { NULL,                         ENNA_KEY_UNKNOWN       }
};

/* Static functions */

static void
_event_cb(void *data, char *event)
{
   Ecore_X_Event_Key_Down *ev;

   if (!event) return;

   ev = calloc(1, sizeof(Ecore_X_Event_Key_Down));

   ev->keyname = event;
   ev->keysymbol = event;
   ev->key_compose = event;
   ev->modifiers = 0;

   printf("LIRC event : %s\n", event);

   ecore_event_add(ECORE_X_EVENT_KEY_DOWN, ev, NULL, NULL);

}

/* Public Functions */

EAPI enna_key_t
enna_get_key (void *event)
{
  int i;
  Ecore_X_Event_Key_Down *ev;

  ev = event;

  if (!ev)
    return ENNA_KEY_UNKNOWN;

  printf ("Key pressed : %s\n", ev->keysymbol);

  for (i = 0; enna_keymap[i].keyname; i++)
    if (!strcmp (enna_keymap[i].keyname, ev->keysymbol))
      return enna_keymap[i].keycode;

  return ENNA_KEY_UNKNOWN;
}

EAPI void
enna_input_init()
{
#ifdef BUILD_LIRC_MODULE
   Enna_Module *em;
   Input_Module_Item *item;
#endif

   /* Create Input event "Key Down" */
   ENNA_EVENT_INPUT_KEY_DOWN = ecore_event_type_new();

   _input_modules = NULL;

#ifdef BUILD_LIRC_MODULE
   em = enna_module_open("lirc", enna->evas);
   item = calloc(1, sizeof(Input_Module_Item));
   item->module = em;
   _input_modules = evas_list_append(_input_modules, item);
   enna_module_enable(em);
#endif

}

EAPI void
enna_input_shutdown()
{
   Evas_List *l = NULL;

   for (l = _input_modules; l; l = l->next)
     {
	Input_Module_Item *item = l->data;
	item->class->func.class_shutdown(0);
	enna_module_disable(item->module);
     }
   evas_list_free(_input_modules);
}

EAPI int
enna_input_class_register(Enna_Module *module, Enna_Class_Input *class)
{
   Evas_List *l = NULL;

   for (l = _input_modules; l; l = l->next)
     {
	Input_Module_Item *item = l->data;
	if (module == item->module)
	  {
	     item->class = class;
	     class->func.class_init(0);
	     class->func.class_event_cb_set(_event_cb, item);

	     return 0;
	  }
     }
   return -1;
}
