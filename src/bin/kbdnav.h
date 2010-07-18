#ifndef _KBD_NAV_
#define _KBD_NAV_

#include <Eina.h>
#include <Evas.h>

typedef struct _Kbdnav_Pool Kbdnav_Pool;

typedef struct _Kbdnav_Pool_Class
{
    Evas_Object *(*object_get)(void *user_data, void *item_data);
    const char *(*id_get)(void *user_data, void *item_data);
    void (*select)(void *user_data, void *item_data);
    void (*unselect)(void *user_data, void *item_data);
    void *user_data;
}Kbdnav_Pool_Class;

Kbdnav_Pool        *kbdnav_pool_new(Kbdnav_Pool_Class *class, const void *data);
void                kbdnav_pool_free(Kbdnav_Pool *pool);

Eina_Bool           kbdnav_pool_item_add(Kbdnav_Pool *pool, const void *data);
Eina_Bool           kbdnav_pool_item_del(Kbdnav_Pool *pool, const void *data);
Eina_Bool           kbdnav_pool_current_id_set(Kbdnav_Pool *pool, const char *id);
void               *kbdnav_pool_current_item_get(Kbdnav_Pool *pool);
Evas_Object        *kbdnav_pool_current_object_get(Kbdnav_Pool *pool);
Eina_Bool           kbdnav_pool_current_select(Kbdnav_Pool *pool);
Eina_Bool           kbdnav_pool_current_unselect(Kbdnav_Pool *pool);

Eina_Bool           kbdnav_pool_navigate_up(Kbdnav_Pool *pool);
Eina_Bool           kbdnav_pool_navigate_down(Kbdnav_Pool *pool);
Eina_Bool           kbdnav_pool_navigate_left(Kbdnav_Pool *pool);
Eina_Bool           kbdnav_pool_navigate_right(Kbdnav_Pool *pool);

#endif /* _KBD_NAV_ */
