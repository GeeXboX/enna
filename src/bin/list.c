/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "enna.h"
#include "list.h"

#define SMART_NAME "enna_list"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;

typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data 
{
   Evas_Coord x, y, w, h, iw, ih;
   Evas_Object *o_smart;
   Evas_Object *o_box;
   Evas_List *items;
   int selected;
   unsigned char selector : 1;
   unsigned char multi_select : 1;
   unsigned char on_hold : 1;
};

static void _e_smart_init             (void);
static void _e_smart_add              (Evas_Object *obj);
static void _e_smart_del              (Evas_Object *obj);
static void _e_smart_show             (Evas_Object *obj);
static void _e_smart_hide             (Evas_Object *obj);
static void _e_smart_move             (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize           (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_color_set        (Evas_Object *obj, int r, int g, int b, int a);
static void _e_smart_clip_set         (Evas_Object *obj, Evas_Object *clip);
static void _e_smart_clip_unset       (Evas_Object *obj);
static void _e_smart_reconfigure      (E_Smart_Data *sd);
static void _e_smart_event_mouse_down (void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_up   (void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_smart_event_key_down   (void *data, Evas *evas, Evas_Object *obj, void *event_info);

static Evas_Smart *_e_smart = NULL;

EAPI Evas_Object *
enna_list_add(Evas *evas) 
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void 
enna_list_append(Evas_Object *obj, Evas_Object *icon, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2)
{
   Enna_List_Item *si;
   Evas_Coord mw = 0, mh = 0;
   
   API_ENTRY return;
   si = ENNA_NEW(Enna_List_Item, 1);
   si->sd = sd;
   si->o_base = edje_object_add(evas_object_evas_get(sd->o_smart));
   
   if (header) 
     edje_object_file_set(si->o_base, "default/default.edj", 
			     "e/widgets/ilist_header");
   else if (evas_list_count(sd->items) & 0x1)
     edje_object_file_set(si->o_base, "default/default.edj",
			     "e/widgets/ilist");
   else
     edje_object_file_set(si->o_base, "default/default.edj",
			     "e/widgets/ilist");
   if (label)
     edje_object_part_text_set(si->o_base, "e.text.label", label);
   si->o_icon = icon;
   if (si->o_icon) 
     {
	edje_extern_object_min_size_set(si->o_icon, sd->iw, sd->ih);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	evas_object_show(si->o_icon);
     }
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;
   sd->items = evas_list_append(sd->items, si);
   
   edje_object_size_min_calc(si->o_base, &mw, &mh);
   enna_box_freeze(sd->o_box);
   enna_box_pack_end(sd->o_box, si->o_base);
   dbg("%d %d\n",mw, mh);
   
   enna_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5, 
			  mw, 48, 99999, 99999);
   enna_box_thaw(sd->o_box);
   
   evas_object_lower(sd->o_box);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
				  _e_smart_event_mouse_up, si);
   evas_object_show(si->o_base);
}

EAPI int 
enna_list_multi_select_get(Evas_Object *obj) 
{
   API_ENTRY return 0;
   return sd->multi_select;
}

EAPI void 
enna_list_multi_select_set(Evas_Object *obj, int multi) 
{
   API_ENTRY return;
   sd->multi_select = multi;
}

EAPI void 
enna_list_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h) 
{
   API_ENTRY return;
   enna_box_min_size_get(sd->o_box, w, h);
}

EAPI void 
enna_list_unselect(Evas_Object *obj) 
{
   Evas_List *l;
   
   API_ENTRY return;
   if (!sd->items) return;
   if (sd->selected < 0) return;
   for (l = sd->items; l; l = l->next) 
     {
	Enna_List_Item *si;
	
	si = l->data;
	if (!si) continue;
	if (!si->selected) continue;
	edje_object_signal_emit(si->o_base, "e,state,unselected", "e");
	si->selected = 0;
     }
   sd->selected = -1;
}

EAPI void 
enna_list_selected_set(Evas_Object *obj, int n) 
{
   Enna_List_Item *si;
   Evas_List *l;
   int i;
   
   API_ENTRY return;
   if (!sd->items) return;
   
   i = evas_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0) n = 0;

   for (l = sd->items; l; l = l->next) 
     {
	si = l->data;
	if (!si) continue;
	if (!si->selected) continue;
	edje_object_signal_emit(si->o_base, "e,state,unselected", "e");
	si->selected = 0;
     }
   sd->selected = -1;
   si = evas_list_nth(sd->items, n);
   if (!si) return;
   si->selected = 1;
   sd->selected = n;
   evas_object_raise(si->o_base);
   edje_object_signal_emit(si->o_base, "e,state,selected", "e");
   if (si->func_hilight) si->func_hilight(si->data, si->data2);
   if (sd->selector) return;
   if (!sd->on_hold)
     {
	if (si->func) si->func(si->data, si->data2);
     }
}

EAPI int 
enna_list_selected_get(Evas_Object *obj) 
{
   Evas_List *l;
   int i, j;
   
   API_ENTRY return -1;
   if (!sd->items) return -1;
   if (!sd->multi_select)
     return sd->selected;
   else
     {
	j = -1;
	for (i = 0, l = sd->items; l; l = l->next, i++) 
	  {
	     Enna_List_Item *li;
	     
	     li = l->data;
	     if (!li) continue;
	     if (li->selected) j = i;
	  }
	return j;
     }
}

EAPI const char *
enna_list_selected_label_get(Evas_Object *obj) 
{
   Enna_List_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return edje_object_part_text_get(si->o_base, "e.text.label");
   return NULL;
}

EAPI void *
enna_list_selected_data_get(Evas_Object *obj) 
{
   Enna_List_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->data;
   return NULL;
}

EAPI void *
enna_list_selected_data2_get(Evas_Object *obj) 
{
   Enna_List_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->data2;
   return NULL;
}

EAPI Evas_Object *
enna_list_selected_icon_get(Evas_Object *obj) 
{
   Enna_List_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->o_icon;
   return NULL;
}

EAPI void 
enna_list_selected_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h) 
{
   Enna_List_Item *si;
   
   API_ENTRY return;
   if (!sd->items) return;
   if (sd->selected < 0) return;
   si = evas_list_nth(sd->items, sd->selected);
   if (!si) return;
   evas_object_geometry_get(si->o_base, x, y, w, h);
   *x -= sd->x;
   *y -= sd->y;
}

EAPI int 
enna_list_selected_count_get(Evas_Object *obj) 
{
   Evas_List *l = NULL;
   int count = 0;
   
   API_ENTRY return 0;
   if (!sd->items) return 0;
   for (l = sd->items; l; l = l->next) 
     {
	Enna_List_Item *si;
	
	si = l->data;
	if (!si) continue;
	if (si->selected) count++;
     }
   return count;
}

EAPI void 
enna_list_remove_num(Evas_Object *obj, int n) 
{
   Enna_List_Item *si;
   
   API_ENTRY return;
   if (!sd->items) return;
   si = evas_list_nth(sd->items, n);
   if (!si) return;
   sd->items = evas_list_remove(sd->items, si);
   if (sd->selected == n) sd->selected = -1;
   if (si->o_icon) evas_object_del(si->o_icon);
   evas_object_del(si->o_base);
   ENNA_FREE(si);
}

EAPI const char *
enna_list_nth_label_get(Evas_Object *obj, int n) 
{
   Enna_List_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, n);
   if (si) return edje_object_part_text_get(si->o_base, "e.text.label");
   return NULL;
}





EAPI void 
enna_list_icon_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h) 
{
   Evas_List *l;
   
   API_ENTRY return;
   if ((sd->iw == w) && (sd->ih == h)) return;
   sd->iw = w;
   sd->ih = h;
   for (l = sd->items; l; l = l->next) 
     {
	Enna_List_Item *si;
	Evas_Coord mw = 0, mh = 0;
	
	si = l->data;
	if (!si) continue;
	if (!si->o_icon) continue;
	edje_extern_object_min_size_set(si->o_icon, w, h);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	edje_object_size_min_calc(si->o_base, &mw, &mh);
	enna_box_pack_options_set(si->o_icon, 1, 1, 1, 0, 0.5, 0.5, 
			       mw, mh, 99999, 99999);
     }
}

EAPI void 
enna_list_clear(Evas_Object *obj) 
{
   API_ENTRY return;
   //enna_list_freeze(obj);
   while (sd->items) 
     {
	Enna_List_Item *si;
	
	si = sd->items->data;
	sd->items = evas_list_remove_list(sd->items, sd->items);
	if (si->o_icon) evas_object_del(si->o_icon);
	evas_object_del(si->o_base);
	ENNA_FREE(si);
     }
   //enna_list_thaw(obj);
   sd->selected = -1;
}


/* SMART FUNCTIONS */
static void 
_e_smart_init(void) 
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
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
}

static void 
_e_smart_add(Evas_Object *obj) 
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);
   
   sd->o_smart = obj;
   sd->x = sd->y = sd->w = sd->h = 0;
   sd->iw = sd->ih = 64;
   sd->selected = -1;
   sd->multi_select = 0;
   
   sd->o_box = enna_box_add(evas_object_evas_get(obj));
   enna_box_align_set(sd->o_box, 0.0, 0.0);
   enna_box_homogenous_set(sd->o_box, 0);
   evas_object_smart_member_add(sd->o_box, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, 
				  _e_smart_event_key_down, sd);
   evas_object_propagate_events_set(obj, 0);
}

static void 
_e_smart_del(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   enna_list_clear(obj);
   evas_object_del(sd->o_box);
   free(sd);
}

static void 
_e_smart_show(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   evas_object_show(sd->o_box);
}

static void 
_e_smart_hide(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->o_box);
}

static void 
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y) 
{
   INTERNAL_ENTRY;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(sd);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h) 
{
   INTERNAL_ENTRY;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(sd);
}

static void 
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a) 
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->o_box, r, g, b, a);
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip) 
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->o_box, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->o_box);
}

static void 
_e_smart_reconfigure(E_Smart_Data *sd) 
{
   evas_object_move(sd->o_box, sd->x, sd->y);
   evas_object_resize(sd->o_box, sd->w, sd->h);
}

static void 
_e_smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   E_Smart_Data *sd;
   Evas_Event_Mouse_Down *ev;
   Enna_List_Item *si;
   Evas_List *l;
   int i;

   ev = event_info;
   si = data;
   sd = si->sd;
   
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;
   
   if (si->header) return;
   if (!sd->items) return;
   for (i = 0, l = sd->items; l; l = l->next, i++) 
     {
	if (l->data == si) 
	  {
	     if (!sd->multi_select)
	       enna_list_selected_set(sd->o_smart, i);
	     else 
	       {
		  enna_list_selected_set(sd->o_smart, i);
	       }
	     break;
	  }
     }
   
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(sd->o_smart, "selected", NULL);
}

static void 
_e_smart_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   E_Smart_Data *sd;
   Evas_Event_Mouse_Up *ev;
   Enna_List_Item *si;

   ev = event_info;
   si = data;
   sd = si->sd;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;
   
   if (si->header) return;
   if (!sd->items) return;
   if (!sd->selector) return;
   si = evas_list_nth(sd->items, sd->selected);
   if (!si) return;
   if (sd->on_hold)
     {
	sd->on_hold = 0;
	return;
     }
   if (si->func) si->func(si->data, si->data2);
}

static void 
_e_smart_event_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   Evas_Event_Key_Down *ev;
   E_Smart_Data *sd;
   Enna_List_Item *si;
   int n, ns;
   
   sd = data;
   ev = event_info;
   ns = sd->selected;
   
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;
   
   if ((!strcmp(ev->keyname, "Up")) || (!strcmp(ev->keyname, "KP_Up")))
     {
	n = ns;
	do
	  {
	     if (n == 0)
	       {
		  n = ns;
		  break;
	       }
	     -- n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) 
	  {
	       enna_list_selected_set(sd->o_smart, n);
	  }
     }
   else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
     {
	n = ns;
	do
	  {
	     if (n == (evas_list_count(sd->items) - 1))
	       {
		  n = ns;
		  break;
	       }
	     ++ n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) 
	  {
	     
	     enna_list_selected_set(sd->o_smart, n);
	  }
     }
   else if ((!strcmp(ev->keyname, "Home")) || (!strcmp(ev->keyname, "KP_Home")))
     {
	n = -1;
	do
	  {
	     if (n == (evas_list_count(sd->items) - 1))
	       {
		  n = ns;
		  break;
	       }
	     ++ n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) 
	  {
	     enna_list_selected_set(sd->o_smart, n);
	  }
     }
   else if ((!strcmp(ev->keyname, "End")) || (!strcmp(ev->keyname, "KP_End")))
     {
	n = evas_list_count(sd->items);
	do
	  {
	     if (n == 0)
	       {
		  n = ns;
		  break;
	       }
	     -- n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) 
	  {
	     enna_list_selected_set(sd->o_smart, n);
	  }
     }
   else if ((!strcmp(ev->keyname, "Return")) ||
	    (!strcmp(ev->keyname, "KP_Enter")) ||
	    (!strcmp(ev->keyname, "space")))
     {
	if (!sd->on_hold)
	  {
	     si = evas_list_nth(sd->items, sd->selected);
	     if (si)
	       {
		  if (si->func) si->func(si->data, si->data2);
	       }
	  }
     }   
   sd->on_hold = 0;
}
