/* -*- Mode: C; indent-tabs-mode: nil; tab-width: 2 -*-
 *
 * Copyright (C) 2009 Canonical Ltd.
 * Authors:
 *  Gustavo Sverzut Barbieri <gustavo.barbieri@canonical.com>
 *
 * This file is part of Netbook Launcher EFL.
 *
 * Netbook Launcher EFL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Netbook Launcher EFL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Netbook Launcher EFL.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * DESIGN NOTE:
 *
 * Directional spatial navigation tries to find the closest item,
 * doing so by using center of given object and the closest edge of
 * children. In order to give the movement axis more priority, a bias
 * is added to the other direction, making it account more on that
 * other axis and thus making the object less likely to be used.
 *
 * Examples:
 *
 * - up movement will ignore all objects on the same line or below the
 *   middle of current object. Distance on X will account as DEFAULT_BIAS
 *   more (shift, power of 2).
 *
 * - down movement will ignore all objects on the same line or above
 *   the middle of current object. Distance on X will account as
 *   DEFAULT_BIAS more.
 *
 * - right movement will ignore all objects on the same line or left
 *   of the middle of current object. Distance on Y will account as
 *   DEFAULT_BIAS more.
 *
 * - left movement will ignore all objects on the same line or right
 *   of the middle of current object. Distance on Y will account as
 *   DEFAULT_BIAS more.
 *
 * This is specially important if item layout is pie/circular, for
 * example the following objects should be selected after movements
 * from "C", "U" is up, "D" is down, "L" is left and "R" is right.
 *
 *             [_][U][_]
 *           [_]       [_]
 *         [_]           [_]
 *       [_]               [_]
 *      [L]       [C]       [R]
 *       [_]               [_]
 *         [_]           [_]
 *           [_]       [_]
 *             [_][D][_]
 *
 * If just distance on movement axis were considered, L/R would be
 * selected for vertical movement, as it's in the same line as "C" and
 * so the smallest vertical distance.
 *
 * If regular distance of both edges (distance = sqrt(dx^2 + dy^2)),
 * then all the objects would be at the same distance, as radius is
 * the same.
 *
 * So the algorithm considers the other axis, but at a higher
 * cost. This is done by left-shifting the DEFAULT_BIAS
 * constant. Using so the following case would happen:
 *
 *      [_]
 *
 *
 *
 *
 *           [U]
 *      [C]
 *
 * Even though the topmost object is in the same line as "C", the
 * object slightly more to right is selected, it's not straight up
 * "C", but its distance is much smaller that it pays off.
 *
 */
#include "kbdnav.h"
#include <stdint.h>
#include "logs.h"
/* well be used as shift-left operator, effectively multiplies by power of 2 */
#define DEFAULT_BIAS 2

static inline uint64_t
_kbdnav_distance_calc(Evas_Coord ref_x, Evas_Coord ref_y, Evas_Coord x, Evas_Coord y, unsigned char bias_x, unsigned char bias_y)
{
  int64_t dx, dy;

  dx = (x - ref_x);
  dy = (y - ref_y);

  return ((uint64_t)(dx * dx) << bias_x) + ((uint64_t)(dy * dy) << bias_y);
}

static Eina_List *
_kbdnav_up(const Eina_List *items, const Evas_Object *current_obj, Evas_Object *(*node_object_get)(void *user_data, const Eina_List *node), const void *user_data)
{
  const Eina_List *node, *new_node;
  Evas_Coord cx, cy, cw, ch;
  uint64_t n_distance;

  evas_object_geometry_get(current_obj, &cx, &cy, &cw, &ch);
  cx += cw / 2;
  cy += ch / 2;

  new_node = NULL;
  n_distance = UINT64_MAX;

  for (node = items; node != NULL; node = node->next)
  {
    const Evas_Object *obj;
    Evas_Coord x, y, w, h;
    uint64_t distance;

    obj = node_object_get((void *)user_data, node);
    if (obj == current_obj) continue;
    if (!evas_object_visible_get(obj)) continue;
    if (evas_object_pass_events_get(obj)) continue;
    evas_object_geometry_get(obj, &x, &y, &w, &h);

    y += h;
    if (y >= cy) continue;

    x += w / 2;
    distance = _kbdnav_distance_calc(cx, cy, x, y, DEFAULT_BIAS, 0);

    if (distance < n_distance)
    {
      new_node = node;
      n_distance = distance;
    }
  }

  return (Eina_List *)new_node;
}

static Eina_List *
_kbdnav_down(const Eina_List *items, const Evas_Object *current_obj, Evas_Object *(*node_object_get)(void *user_data, const Eina_List *node), const void *user_data)
{
  const Eina_List *node, *new_node;
  Evas_Coord cx, cy, cw, ch;
  uint64_t n_distance;

  evas_object_geometry_get(current_obj, &cx, &cy, &cw, &ch);
  cx += cw / 2;
  cy += ch / 2;

  new_node = NULL;
  n_distance = UINT64_MAX;

  for (node = items; node != NULL; node = node->next)
  {
    const Evas_Object *obj;
    Evas_Coord x, y, w;
    uint64_t distance;

    obj = node_object_get((void *)user_data, node);
    if (obj == current_obj) continue;
    if (!evas_object_visible_get(obj)) continue;
    if (evas_object_pass_events_get(obj)) continue;
    evas_object_geometry_get(obj, &x, &y, &w, NULL);

    if (y < cy) continue;

    x += w / 2;
    distance = _kbdnav_distance_calc(cx, cy, x, y, DEFAULT_BIAS, 0);

    if (distance < n_distance)
    {
      new_node = node;
      n_distance = distance;
    }
  }

  return (Eina_List *)new_node;
}

static Eina_List *
_kbdnav_left(const Eina_List *items, const Evas_Object *current_obj, Evas_Object *(*node_object_get)(void *user_data, const Eina_List *node), const void *user_data)
{
  const Eina_List *node, *new_node;
  Evas_Coord cx, cy, cw, ch;
  uint64_t n_distance;

  evas_object_geometry_get(current_obj, &cx, &cy, &cw, &ch);
  cx += cw / 2;
  cy += ch / 2;

  new_node = NULL;
  n_distance = UINT64_MAX;

  for (node = items; node != NULL; node = node->next)
  {
    const Evas_Object *obj;
    Evas_Coord x, y, w, h;
    uint64_t distance;

    obj = node_object_get((void *)user_data, node);
    if (obj == current_obj) continue;
    if (!evas_object_visible_get(obj)) continue;
    if (evas_object_pass_events_get(obj)) continue;
    evas_object_geometry_get(obj, &x, &y, &w, &h);

    x += w;
    if (x >= cx) continue;

    y += h / 2;
    distance = _kbdnav_distance_calc(cx, cy, x, y, 0, DEFAULT_BIAS);

    if (distance < n_distance)
    {
      new_node = node;
      n_distance = distance;
    }
  }

  return (Eina_List *)new_node;
}

static Eina_List *
_kbdnav_right(const Eina_List *items, const Evas_Object *current_obj, Evas_Object *(*node_object_get)(void *user_data, const Eina_List *node), const void *user_data)
{
  const Eina_List *node, *new_node;
  Evas_Coord cx, cy, cw, ch;
  uint64_t n_distance;

  evas_object_geometry_get(current_obj, &cx, &cy, &cw, &ch);
  cx += cw / 2;
  cy += ch / 2;

  new_node = NULL;
  n_distance = UINT64_MAX;

  for (node = items; node != NULL; node = node->next)
  {
    const Evas_Object *obj;
    Evas_Coord x, y, w, h;
    uint64_t distance;

    obj = node_object_get((void *)user_data, node);
    if (obj == current_obj) continue;
    if (!evas_object_visible_get(obj)) continue;
    if (evas_object_pass_events_get(obj)) continue;
    evas_object_geometry_get(obj, &x, &y, &w, &h);

    if (x < cx) continue;

    y += h / 2;
    distance = _kbdnav_distance_calc(cx, cy, x, y, 0, DEFAULT_BIAS);

    if (distance < n_distance)
    {
      new_node = node;
      n_distance = distance;
    }
  }

  return (Eina_List *)new_node;
}

struct _Kbdnav_Pool
{
  Eina_List *items;
  struct
  {
    const Eina_List *node;
    const char *id;
  } current;
  struct
  {
    Evas_Object *(*object_get)(void *user_data, void *item_data);
    const char *(*id_get)(void *user_data, void *item_data);
    void (*select)(void *user_data, void *item_data);
    void (*unselect)(void *user_data, void *item_data);
    void *user_data;
  } api;
};

Kbdnav_Pool *
kbdnav_pool_new(Kbdnav_Pool_Class *class, const void *user_data)
{
  Kbdnav_Pool *pool;

  if (!class->object_get)
  {
    ERR("object_get == NULL\n");
    return NULL;
  }

  if (! class->id_get)
  {
    ERR("id_get == NULL\n");
    return NULL;
  }

  if (!class->select)
  {
    ERR("select == NULL\n");
    return NULL;
  }

  if (!class->unselect)
  {
    ERR("unselect == NULL\n");
    return NULL;
  }

  pool = calloc(1, sizeof(*pool));
  if (!pool)
  {
    ERR("could not allocate kbdnav_pool.\n");
    return NULL;
  }

  pool->api.object_get =  class->object_get;
  pool->api.id_get = class->id_get;
  pool->api.select = class->select;
  pool->api.unselect = class->unselect;
  pool->api.user_data = (void *)user_data;
  return pool;
}

void
kbdnav_pool_free(Kbdnav_Pool *pool)
{
  if (!pool)
  {
    ERR("pool == NULL\n");
    return;
  }

  if (pool->items)
    WRN("%d items still alive!\n", eina_list_count(pool->items));
  eina_list_free(pool->items);
  eina_stringshare_del(pool->current.id);
  free(pool);
}

Eina_Bool
kbdnav_pool_item_add(Kbdnav_Pool *pool, const void *data)
{
  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }
  pool->items = eina_list_append(pool->items, data);
  return !eina_error_get();
}

Eina_Bool
kbdnav_pool_item_del(Kbdnav_Pool *pool, const void *data)
{
  Eina_List *n;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  n = eina_list_data_find_list(pool->items, data);
  if (!n)
  {
    ERR("data %p not in pool.\n", data);
    return 0;
  }

  if (pool->current.node == n)
  {
    eina_stringshare_del(pool->current.id);
    pool->current.id = NULL;
    pool->current.node = NULL;
  }

  pool->items = eina_list_remove_list(pool->items, n);
  return 1;
}

Eina_Bool
kbdnav_pool_current_id_set(Kbdnav_Pool *pool, const char *id)
{
  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  if (id == pool->current.id)
    return 1;

  eina_stringshare_ref(id);
  eina_stringshare_del(pool->current.id);
  pool->current.id = id;
  pool->current.node = NULL;

  return 1;
}

static const Eina_List *
_kbdnav_pool_current_node_get(Kbdnav_Pool *pool)
{
  const char *(*id_get)(void *user_data, void *item_data);
  const char *current_id;
  Eina_List *n;
  void *user_data;

  if (pool->current.node)
    return pool->current.node;

  id_get = pool->api.id_get;
  user_data = pool->api.user_data;

  if (!pool->current.id)
    return NULL;

  current_id = pool->current.id;
  for (n = pool->items; n != NULL; n = n->next)
  {
    const char *id = id_get(user_data, n->data);
    if (current_id == id)
    {
      pool->current.node = n;
      return n;
    }
  }
  WRN("id '%s' not found in pool!\n", current_id);

  return NULL;
}

static Eina_Bool
_kbdnav_pool_current_node_set(Kbdnav_Pool *pool, const Eina_List *node)
{
  void *user_data = pool->api.user_data;

  if (pool->current.node)
    pool->api.unselect(user_data, pool->current.node->data);

  if (node)
  {
    const char *id = pool->api.id_get(user_data, node->data);
    eina_stringshare_replace(&pool->current.id, id);
    pool->current.node = node;
    pool->api.select(user_data, node->data);
  }
  else
  {
    eina_stringshare_del(pool->current.id);
    pool->current.id = NULL;
    pool->current.node = NULL;
  }

  return 1;
}

void *
kbdnav_pool_current_item_get(Kbdnav_Pool *pool)
{
  const Eina_List *n;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return NULL;
  }

  n = _kbdnav_pool_current_node_get(pool);
  if (!n)
    return NULL;

  return n->data;
}

Evas_Object *
kbdnav_pool_current_object_get(Kbdnav_Pool *pool)
{
  void *item_data;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return NULL;
  }

  item_data = kbdnav_pool_current_item_get(pool);
  if (!item_data)
    return NULL;
  return pool->api.object_get(pool->api.user_data, item_data);
}

Eina_Bool
kbdnav_pool_current_select(Kbdnav_Pool *pool)
{
  const Eina_List *node;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  node = _kbdnav_pool_current_node_get(pool);
  if (!node)
  {
    DBG("no current.\n");
    return 0;
  }
  pool->api.select(pool->api.user_data, node->data);
  return 1;
}

Eina_Bool
kbdnav_pool_current_unselect(Kbdnav_Pool *pool)
{
  const Eina_List *node;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  node = _kbdnav_pool_current_node_get(pool);
  if (!node)
  {
    DBG("no current.\n");
    return 0;
  }
  pool->api.unselect(pool->api.user_data, node->data);
  return 1;
}

static Evas_Object *
_kbdnav_pool_node_object_get(void *user_data, const Eina_List *node)
{
  Kbdnav_Pool *pool = user_data;
  return pool->api.object_get(pool->api.user_data, node->data);
}

Eina_Bool
kbdnav_pool_navigate_up(Kbdnav_Pool *pool)
{
  const Eina_List *node, *result;
  Evas_Object *o;
  void *user_data;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  node = _kbdnav_pool_current_node_get(pool);
  if (!node)
  {
    if (pool->items)
      return _kbdnav_pool_current_node_set(pool, pool->items);
    WRN("node == NULL\n");
    return 0;
  }
  user_data = pool->api.user_data;
  o = pool->api.object_get(user_data, node->data);
  result = _kbdnav_up(pool->items, o, _kbdnav_pool_node_object_get, pool);
  if (!result)
  {
    DBG("cannot move up\n");
    return 0;
  }
  return _kbdnav_pool_current_node_set(pool, result);
}

Eina_Bool
kbdnav_pool_navigate_down(Kbdnav_Pool *pool)
{
  const Eina_List *node, *result;
  Evas_Object *o;
  void *user_data;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  node = _kbdnav_pool_current_node_get(pool);
  if (!node)
  {
    if (pool->items)
      return _kbdnav_pool_current_node_set(pool, pool->items);
    WRN("node == NULL\n");
    return 0;
  }
  user_data = pool->api.user_data;
  o = pool->api.object_get(user_data, node->data);
  result = _kbdnav_down(pool->items, o, _kbdnav_pool_node_object_get, pool);
  if (!result)
  {
    DBG("cannot move down\n");
    return 0;
  }
  return _kbdnav_pool_current_node_set(pool, result);
}

Eina_Bool
kbdnav_pool_navigate_left(Kbdnav_Pool *pool)
{
  const Eina_List *node, *result;
  Evas_Object *o;
  void *user_data;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  node = _kbdnav_pool_current_node_get(pool);
  if (!node)
  {
    if (pool->items)
      return _kbdnav_pool_current_node_set(pool, pool->items);
    WRN("node == NULL\n");
    return 0;
  }
  user_data = pool->api.user_data;
  o = pool->api.object_get(user_data, node->data);
  result = _kbdnav_left(pool->items, o, _kbdnav_pool_node_object_get, pool);
  if (!result)
  {
    DBG("cannot move left\n");
    return 0;
  }
  return _kbdnav_pool_current_node_set(pool, result);
}

Eina_Bool
kbdnav_pool_navigate_right(Kbdnav_Pool *pool)
{
  const Eina_List *node, *result;
  Evas_Object *o;
  void *user_data;

  if (!pool)
  {
    ERR("pool == NULL\n");
    return 0;
  }

  node = _kbdnav_pool_current_node_get(pool);
  if (!node)
  {
    if (pool->items)
      return _kbdnav_pool_current_node_set(pool, pool->items);
    WRN("node == NULL\n");
    return 0;
  }
  user_data = pool->api.user_data;
  o = pool->api.object_get(user_data, node->data);
  result = _kbdnav_right(pool->items, o, _kbdnav_pool_node_object_get, pool);
  if (!result)
  {
    DBG("cannot move right\n");
    return 0;
  }
  return _kbdnav_pool_current_node_set(pool, result);
}
