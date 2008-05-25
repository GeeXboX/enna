#ifndef _EVENT_KEY_H_
#define _EVENT_KEY_H_

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
} enna_key_t;

enna_key_t enna_get_key (Evas_Event_Key_Down *ev);

#endif /* _EVENT_KEY_H_ */
