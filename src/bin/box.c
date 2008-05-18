/*
 * enna_box.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna_box.c is free software copyrighted by Nicolas Aguirre.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Nicolas Aguirre'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * enna_box.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Nicolas Aguirre OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
   This file is stolen from e himself
   and modified for the need of enna
*/


#include "enna.h"
#include "box.h"


typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Box_Item E_Box_Item;

#define SMART_NAME "enna_box"

struct _E_Smart_Data
{
   Evas_Coord          x, y, w, h;
   Evas_Object        *obj;
   Evas_Object        *clip;
   int                 frozen;
   unsigned char       changed:1;
   unsigned char       horizontal:1;
   unsigned char       homogenous:1;
   Evas_List          *items;
   struct
   {
      Evas_Coord          w, h;
   }
   min                , max;
   struct
   {
      double              x, y;
   }
   align;
};

struct _E_Box_Item
{
   E_Smart_Data       *sd;
   unsigned char       fill_w:1;
   unsigned char       fill_h:1;
   unsigned char       expand_w:1;
   unsigned char       expand_h:1;
   struct
   {
      Evas_Coord          w, h;
   }
   min                , max;
   struct
   {
      double              x, y;
   }
   align;
   Evas_Object        *obj;
};

/* local subsystem functions */
static E_Box_Item  *_enna_box_smart_adopt(E_Smart_Data * sd, Evas_Object * obj);
static void         _enna_box_smart_disown(Evas_Object * obj);
static void         _enna_box_smart_item_del_hook(void *data, Evas * e,
						  Evas_Object * obj,
						  void *event_info);
static void         _enna_box_smart_reconfigure(E_Smart_Data * sd);
static void         _enna_box_smart_extents_calcuate(E_Smart_Data * sd);

static void         _e_smart_init(void);
static void         _e_smart_add(Evas_Object * obj);
static void         _e_smart_del(Evas_Object * obj);
static void         _e_smart_move(Evas_Object * obj, Evas_Coord x,
				  Evas_Coord y);
static void         _e_smart_resize(Evas_Object * obj, Evas_Coord w,
				    Evas_Coord h);
static void         _e_smart_show(Evas_Object * obj);
static void         _e_smart_hide(Evas_Object * obj);
static void         _e_smart_color_set(Evas_Object * obj, int r, int g, int b,
				       int a);
static void         _e_smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void         _e_smart_clip_unset(Evas_Object * obj);

/* local subsystem globals */
static Evas_Smart  *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object   *
enna_box_add(Evas * evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI int
enna_box_freeze(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   sd->frozen++;
   return sd->frozen;
}

EAPI int
enna_box_thaw(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   sd->frozen--;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
   return sd->frozen;
}

EAPI void
enna_box_orientation_set(Evas_Object * obj, int horizontal)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if (((sd->horizontal) && (horizontal)) ||
       ((!sd->horizontal) && (!horizontal)))
      return;
   sd->horizontal = horizontal;
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
}

EAPI int
enna_box_orientation_get(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   return sd->horizontal;
}

EAPI void
enna_box_homogenous_set(Evas_Object * obj, int homogenous)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if (sd->homogenous == homogenous)
      return;
   sd->homogenous = homogenous;
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
}

EAPI int
enna_box_pack_start(Evas_Object * obj, Evas_Object * child)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   _enna_box_smart_adopt(sd, child);
   sd->items = evas_list_prepend(sd->items, child);
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
   return 0;
}

EAPI int
enna_box_pack_end(Evas_Object * obj, Evas_Object * child)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   _enna_box_smart_adopt(sd, child);
   sd->items = evas_list_append(sd->items, child);
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
   return evas_list_count(sd->items) - 1;
}

EAPI int
enna_box_pack_before(Evas_Object * obj, Evas_Object * child,
		     Evas_Object * before)
{
   E_Smart_Data       *sd;
   int                 i = 0;
   Evas_List          *l;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   _enna_box_smart_adopt(sd, child);
   sd->items = evas_list_prepend_relative(sd->items, child, before);
   for (i = 0, l = sd->items; l; l = l->next, i++)
     {
	if (l->data == child)
	   break;
     }
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
   return i;
}

EAPI int
enna_box_pack_after(Evas_Object * obj, Evas_Object * child, Evas_Object * after)
{
   E_Smart_Data       *sd;
   int                 i = 0;
   Evas_List          *l;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   _enna_box_smart_adopt(sd, child);
   sd->items = evas_list_append_relative(sd->items, child, after);
   for (i = 0, l = sd->items; l; l = l->next, i++)
     {
	if (l->data == child)
	   break;
     }
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
   return i;
}

EAPI int
enna_box_pack_count_get(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return 0;
   return evas_list_count(sd->items);
}

EAPI Evas_Object   *
enna_box_pack_object_nth(Evas_Object * obj, int n)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return NULL;
   return evas_list_nth(sd->items, n);
}

EAPI Evas_Object   *
enna_box_pack_object_first(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return NULL;
   return evas_list_data(sd->items);
}

EAPI Evas_Object   *
enna_box_pack_object_last(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return NULL;
   return evas_list_data(evas_list_last(sd->items));
}

EAPI void
enna_box_pack_options_set(Evas_Object * obj, int fill_w, int fill_h,
			  int expand_w, int expand_h, double align_x,
			  double align_y, Evas_Coord min_w, Evas_Coord min_h,
			  Evas_Coord max_w, Evas_Coord max_h)
{
   E_Box_Item         *bi;

   bi = evas_object_data_get(obj, "enna_box_data");
   if (!bi)
      return;
   bi->fill_w = fill_w;
   bi->fill_h = fill_h;
   bi->expand_w = expand_w;
   bi->expand_h = expand_h;
   bi->align.x = align_x;
   bi->align.y = align_y;
   bi->min.w = min_w;
   bi->min.h = min_h;
   bi->max.w = max_w;
   bi->max.h = max_h;
   bi->sd->changed = 1;
   if (bi->sd->frozen <= 0)
      _enna_box_smart_reconfigure(bi->sd);
}

EAPI void
enna_box_unpack(Evas_Object * obj)
{
   E_Box_Item         *bi;
   E_Smart_Data       *sd;

   bi = evas_object_data_get(obj, "enna_box_data");
   if (!bi)
      return;
   sd = bi->sd;
   if (!sd)
      return;
   sd->items = evas_list_remove(sd->items, obj);
   _enna_box_smart_disown(obj);
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
}

EAPI void
enna_box_min_size_get(Evas_Object * obj, Evas_Coord * minw, Evas_Coord * minh)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if (sd->changed)
      _enna_box_smart_extents_calcuate(sd);

   if (minw)
      *minw = sd->min.w;
   if (minh)
      *minh = sd->min.h;
}

EAPI void
enna_box_max_size_get(Evas_Object * obj, Evas_Coord * maxw, Evas_Coord * maxh)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if (sd->changed)
      _enna_box_smart_extents_calcuate(sd);
   if (maxw)
      *maxw = sd->max.w;
   if (maxh)
      *maxh = sd->max.h;
}

EAPI void
enna_box_align_get(Evas_Object * obj, double *ax, double *ay)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if (ax)
      *ax = sd->align.x;
   if (ay)
      *ay = sd->align.y;
}

EAPI void
enna_box_align_set(Evas_Object * obj, double ax, double ay)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if ((sd->align.x == ax) && (sd->align.y == ay))
      return;
   sd->align.x = ax;
   sd->align.y = ay;
   sd->changed = 1;
   if (sd->frozen <= 0)
      _enna_box_smart_reconfigure(sd);
}

/* local subsystem functions */
static E_Box_Item  *
_enna_box_smart_adopt(E_Smart_Data * sd, Evas_Object * obj)
{
   E_Box_Item         *bi;

   bi = calloc(1, sizeof(E_Box_Item));
   if (!bi)
      return NULL;
   bi->sd = sd;
   bi->obj = obj;
   /* defaults */
   bi->fill_w = 0;
   bi->fill_h = 0;
   bi->expand_w = 0;
   bi->expand_h = 0;
   bi->align.x = 0.5;
   bi->align.y = 0.5;
   bi->min.w = 0;
   bi->min.h = 0;
   bi->max.w = 0;
   bi->max.h = 0;
   evas_object_clip_set(obj, sd->clip);
   evas_object_smart_member_add(obj, bi->sd->obj);
   evas_object_data_set(obj, "enna_box_data", bi);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
				  _enna_box_smart_item_del_hook, NULL);
   if ((!evas_object_visible_get(sd->clip)) &&
       (evas_object_visible_get(sd->obj)))
      evas_object_show(sd->clip);
   return bi;
}

static void
_enna_box_smart_disown(Evas_Object * obj)
{
   E_Box_Item         *bi;

   bi = evas_object_data_get(obj, "enna_box_data");
   if (!bi)
      return;
   if (!bi->sd->items)
     {
	if (evas_object_visible_get(bi->sd->clip))
	   evas_object_hide(bi->sd->clip);
     }
   evas_object_event_callback_del(obj,
				  EVAS_CALLBACK_FREE,
				  _enna_box_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_clip_unset(obj);
   evas_object_data_del(obj, "enna_box_data");
   free(bi);
}

static void
_enna_box_smart_item_del_hook(void *data, Evas * e, Evas_Object * obj,
			      void *event_info)
{
   enna_box_unpack(obj);
}

static void
_enna_box_smart_reconfigure(E_Smart_Data * sd)
{
   Evas_Coord          x, y, w, h, xx, yy;
   Evas_List          *l;
   int                 minw, minh, wdif, hdif;
   int                 count, expand;

   if (!sd->changed)
      return;

   x = sd->x;
   y = sd->y;
   w = sd->w;
   h = sd->h;

   _enna_box_smart_extents_calcuate(sd);
   minw = sd->min.w;
   minh = sd->min.h;
   count = evas_list_count(sd->items);
   expand = 0;
   if (w < minw)
     {
	x = x + ((w - minw) * (1.0 - sd->align.x));
	w = minw;
     }
   if (h < minh)
     {
	y = y + ((h - minh) * (1.0 - sd->align.y));
	h = minh;
     }
   for (l = sd->items; l; l = l->next)
     {
	E_Box_Item         *bi;
	Evas_Object        *obj;

	obj = l->data;
	bi = evas_object_data_get(obj, "enna_box_data");
	if (bi)
	  {
	     if (sd->horizontal)
	       {
		  if (bi->expand_w)
		     expand++;
	       }
	     else
	       {
		  if (bi->expand_h)
		     expand++;
	       }
	  }
     }
   if (expand == 0)
     {
	if (sd->horizontal)
	  {
	     x += (double)(w - minw) * sd->align.x;
	     w = minw;
	  }
	else
	  {
	     y += (double)(h - minh) * sd->align.y;
	     h = minh;
	  }
     }
   wdif = w - minw;
   hdif = h - minh;
   xx = x;
   yy = y;
   for (l = sd->items; l; l = l->next)
     {
	E_Box_Item         *bi;
	Evas_Object        *obj;

	obj = l->data;
	bi = evas_object_data_get(obj, "enna_box_data");
	if (bi)
	  {
	     if (sd->horizontal)
	       {
		  if (sd->homogenous)
		    {
		       Evas_Coord          ww, hh, ow, oh;

		       ww = (w / (Evas_Coord) count);
		       hh = h;
		       ow = bi->min.w;
		       if (bi->fill_w)
			  ow = ww;
		       if ((bi->max.w >= 0) && (bi->max.w < ow))
			  ow = bi->max.w;
		       oh = bi->min.h;
		       if (bi->fill_h)
			  oh = hh;
		       if ((bi->max.h >= 0) && (bi->max.h < oh))
			  oh = bi->max.h;
		       evas_object_move(obj,
					xx +
					(Evas_Coord) (((double)(ww - ow)) *
						      bi->align.x),
					yy +
					(Evas_Coord) (((double)(hh - oh)) *
						      bi->align.y));
		       evas_object_resize(obj, ow, oh);
		       xx += ww;
		    }
		  else
		    {
		       Evas_Coord          ww, hh, ow, oh;

		       ww = bi->min.w;
		       if ((expand > 0) && (bi->expand_w))
			 {
			    if (expand == 1)
			       ow = wdif;
			    else
			       ow = (w - minw) / expand;
			    wdif -= ow;
			    ww += ow;
			 }
		       hh = h;
		       ow = bi->min.w;
		       if (bi->fill_w)
			  ow = ww;
		       if ((bi->max.w >= 0) && (bi->max.w < ow))
			  ow = bi->max.w;
		       oh = bi->min.h;
		       if (bi->fill_h)
			  oh = hh;
		       if ((bi->max.h >= 0) && (bi->max.h < oh))
			  oh = bi->max.h;
		       evas_object_move(obj,
					xx +
					(Evas_Coord) (((double)(ww - ow)) *
						      bi->align.x),
					yy +
					(Evas_Coord) (((double)(hh - oh)) *
						      bi->align.y));
		       evas_object_resize(obj, ow, oh);
		       xx += ww;
		    }
	       }
	     else
	       {
		  if (sd->homogenous)
		    {
		       Evas_Coord          ww, hh, ow, oh;

		       ww = w;
		       hh = (h / (Evas_Coord) count);
		       ow = bi->min.w;
		       if (bi->expand_w)
			  ow = ww;
		       if ((bi->max.w >= 0) && (bi->max.w < ow))
			  ow = bi->max.w;
		       oh = bi->min.h;
		       if (bi->expand_h)
			  oh = hh;
		       if ((bi->max.h >= 0) && (bi->max.h < oh))
			  oh = bi->max.h;
		       evas_object_move(obj,
					xx +
					(Evas_Coord) (((double)(ww - ow)) *
						      bi->align.x),
					yy +
					(Evas_Coord) (((double)(hh - oh)) *
						      bi->align.y));
		       evas_object_resize(obj, ow, oh);
		       yy += hh;
		    }
		  else
		    {
		       Evas_Coord          ww, hh, ow, oh;

		       ww = w;
		       hh = bi->min.h;
		       if ((expand > 0) && (bi->expand_h))
			 {
			    if (expand == 1)
			       oh = hdif;
			    else
			       oh = (h - minh) / expand;
			    hdif -= oh;
			    hh += oh;
			 }
		       ow = bi->min.w;
		       if (bi->expand_w)
			  ow = ww;
		       if ((bi->max.w >= 0) && (bi->max.w < ow))
			  ow = bi->max.w;
		       oh = bi->min.h;
		       if (bi->expand_h)
			  oh = hh;
		       if ((bi->max.h >= 0) && (bi->max.h < oh))
			  oh = bi->max.h;
		       evas_object_move(obj,
					xx +
					(Evas_Coord) (((double)(ww - ow)) *
						      bi->align.x),
					yy +
					(Evas_Coord) (((double)(hh - oh)) *
						      bi->align.y));
		       evas_object_resize(obj, ow, oh);
		       yy += hh;
		    }
	       }
	  }
     }
   sd->changed = 0;
}

static void
_enna_box_smart_extents_calcuate(E_Smart_Data * sd)
{
   Evas_List          *l;
   int                 minw, minh;

   /* FIXME: need to calc max */
   sd->max.w = -1;		/* max < 0 == unlimited */
   sd->max.h = -1;

   minw = 0;
   minh = 0;
   if (sd->homogenous)
     {
	for (l = sd->items; l; l = l->next)
	  {
	     E_Box_Item         *bi;
	     Evas_Object        *obj;

	     obj = l->data;
	     bi = evas_object_data_get(obj, "enna_box_data");
	     if (bi)
	       {
		  if (sd->horizontal)
		    {
		       if (minh < bi->min.h)
			  minh = bi->min.h;
		       if (minw < bi->min.w)
			  minw = bi->min.w;
		    }
		  else
		    {
		       if (minw < bi->min.w)
			  minw = bi->min.w;
		       if (minh < bi->min.h)
			  minh = bi->min.h;
		    }
	       }
	  }
	if (sd->horizontal)
	  {
	     minw *= evas_list_count(sd->items);
	  }
	else
	  {
	     minh *= evas_list_count(sd->items);
	  }
     }
   else
     {
	for (l = sd->items; l; l = l->next)
	  {
	     E_Box_Item         *bi;
	     Evas_Object        *obj;

	     obj = l->data;
	     bi = evas_object_data_get(obj, "enna_box_data");
	     if (bi)
	       {
		  if (sd->horizontal)
		    {
		       if (minh < bi->min.h)
			  minh = bi->min.h;
		       minw += bi->min.w;
		    }
		  else
		    {
		       if (minw < bi->min.w)
			  minw = bi->min.w;
		       minh += bi->min.h;
		    }
	       }
	  }
     }
   sd->min.w = minw;
   sd->min.h = minh;
}

static void
_e_smart_init(void)
{
   if (_e_smart)
      return;
   static const Evas_Smart_Class sc = {
      SMART_NAME,
      EVAS_SMART_CLASS_VERSION,
      _e_smart_add,
      _e_smart_del,
      _e_smart_move,
      _e_smart_resize,
      _e_smart_show,
      _e_smart_hide,
      _e_smart_color_set,
      _e_smart_clip_set,
      _e_smart_clip_unset,
      NULL
   };
   _e_smart = evas_smart_class_new(&sc);
}

static void
_e_smart_add(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd)
      return;
   sd->obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->align.x = 0.5;
   sd->align.y = 0.5;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, -100004, -100004);
   evas_object_resize(sd->clip, 200008, 200008);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   /* FIXME: this gets into an infinite loop when changin basic->advanced on
    * ibar config dialog
    */
   while (sd->items)
     {
	Evas_Object        *child;

	child = sd->items->data;
	enna_box_unpack(child);
     }
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if ((x == sd->x) && (y == sd->y))
      return;
   {
      Evas_List          *l;
      Evas_Coord          dx, dy;

      dx = x - sd->x;
      dy = y - sd->y;
      for (l = sd->items; l; l = l->next)
	{
	   Evas_Coord          ox, oy;

	   evas_object_geometry_get(l->data, &ox, &oy, NULL, NULL);
	   evas_object_move(l->data, ox + dx, oy + dy);
	}
   }
   sd->x = x;
   sd->y = y;
}

static void
_e_smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if ((w == sd->w) && (h == sd->h))
      return;
   sd->w = w;
   sd->h = h;
   sd->changed = 1;
   _enna_box_smart_reconfigure(sd);
}

static void
_e_smart_show(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   if (sd->items)
      evas_object_show(sd->clip);
}

static void
_e_smart_hide(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   evas_object_hide(sd->clip);
}

static void
_e_smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_smart_clip_unset(Evas_Object * obj)
{
   E_Smart_Data       *sd;

   sd = evas_object_smart_data_get(obj);
   if (!sd)
      return;
   evas_object_clip_unset(sd->clip);
}
