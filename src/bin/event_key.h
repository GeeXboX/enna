#ifndef _EVENT_KEY_H_
#define _EVENT_KEY_H_

#include "enna.h"

typedef enum {
  ENNA_KEY_UNKNOWN,
  ENNA_KEY_MENU,
  ENNA_KEY_QUIT,
  ENNA_KEY_LEFT,
  ENNA_KEY_RIGHT,
  ENNA_KEY_UP,
  ENNA_KEY_DOWN,
  ENNA_KEY_OK,
  ENNA_KEY_CANCEL,
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
   }func;

};

/* Input ecore event */
EAPI int ENNA_EVENT_INPUT_KEY_DOWN;

/* Enna Event API functions */
EAPI enna_key_t enna_get_key (void *event);
EAPI void       enna_input_init();
EAPI void       enna_input_shutdown();
EAPI int enna_input_class_register(Enna_Module *module, Enna_Class_Input *class);

#endif /* _EVENT_KEY_H_ */
