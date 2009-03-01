/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "enna.h"
#include "list.h"

#define thumbscroll_friction 1.0
#define thumbscroll_momentum_threshhold 100
#define thumbscroll_threshhold 16

#define SMART_NAME "enna_list"

typedef struct _Smart_Data Smart_Data;
typedef struct _List_Item List_Item;

struct _List_Item
{
    void *data;
    void (*func) (void *data);
    Elm_Genlist_Item *item;
};

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *o_smart;
    Evas_Object *o_edje;
    Evas_Object *o_list;
    Eina_List *items;
    unsigned char on_hold : 1;
    unsigned int letter_mode;
    Ecore_Timer *letter_timer;
    unsigned int letter_event_nbr;
    char letter_key;

};

static void _smart_init(void);
static void _smart_add(Evas_Object *obj);
static void _smart_del(Evas_Object *obj);
static void _smart_show(Evas_Object *obj);
static void _smart_hide(Evas_Object *obj);
static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _smart_clip_unset(Evas_Object *obj);
static void _smart_reconfigure(Smart_Data *sd);
static void _smart_event_key_down(Smart_Data *sd, void *event_info);
static int _letter_timer_cb(void *data);
static void list_item_select(Smart_Data *sd, int n);

static Evas_Smart *_e_smart = NULL;

Evas_Object *
enna_list_add(Evas *evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _e_smart);
}

void _item_activated(void *data, Evas_Object *obj, void *event_info)
{
    Smart_Data *sd = data;
    Elm_Genlist_Item *item = event_info;
    List_Item *it = NULL;
    Eina_List *l;

    if (!sd) return;

    EINA_LIST_FOREACH(sd->items, l, it)
    {
	if (it->item == item)
	{
	    printf("Activate item\n");
	    it->func(it->data);
	    return;
	}
    }
}

void enna_list_append(Evas_Object *obj, Elm_Genlist_Item_Class *class, void * class_data, void (*func) (void *data),  void *data)
{

    List_Item *it;
    API_ENTRY return;

    it = calloc(1, sizeof(List_Item));
    it->item = elm_genlist_item_append (sd->o_list, class, class_data,
	NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL );

    it->func = func;
    it->data = data;
    evas_object_smart_callback_add(sd->o_list, "clicked", _item_activated, sd);
    sd->items = eina_list_append(sd->items, it);
}

void enna_list_selected_set(Evas_Object *obj, int n)
{

    API_ENTRY return;
    list_item_select(sd, n);
}

int enna_list_jump_label(Evas_Object *obj, const char *label)
{

    return -1;
}

void enna_list_jump_nth(Evas_Object *obj, int n)
{

}

int enna_list_selected_get(Evas_Object *obj)
{
    Eina_List *l;
    List_Item *it;
    int i = 0;

    API_ENTRY return -1;

    printf("selected get\n");

    if (!sd->items) return -1;
    EINA_LIST_FOREACH(sd->items,l, it)
    {
	if ( elm_genlist_item_selected_get (it->item))
	{
	    printf("Selected item : %d\n", i);
	    return i;
	}
	i++;
    }
    return -1;
}

void *enna_list_selected_data_get(Evas_Object *obj)
{
    Eina_List *l;
    List_Item *it;

    API_ENTRY return NULL;

    if (!sd->items) return NULL;

    EINA_LIST_FOREACH(sd->items,l, it)
    {
	if ( elm_genlist_item_selected_get (it->item))
	{
	    return it->data;
	}
    }
    return NULL;
}



void enna_list_event_key_down(Evas_Object *obj, void *event_info)
{
    API_ENTRY
    return;
    _smart_event_key_down(sd, event_info);
}

/* SMART FUNCTIONS */
static void _smart_init(void)
{
    if (_e_smart)
        return;
    {
        static const Evas_Smart_Class sc =
        {
	    SMART_NAME,
	    EVAS_SMART_CLASS_VERSION,
	    _smart_add,
	    _smart_del,
	    _smart_move,
	    _smart_resize,
	    _smart_show,
	    _smart_hide,
	    _smart_color_set,
	    _smart_clip_set,
	    _smart_clip_unset,
	    NULL,
	    NULL
	};
        _e_smart = evas_smart_class_new(&sc);
    }
}


static void _smart_add(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    evas_object_smart_data_set(obj, sd);

    sd->o_smart = obj;
    sd->x = sd->y = sd->w = sd->h = 0;

    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->o_edje, enna_config_theme_get(), "enna/list");

    sd->o_list = elm_genlist_add(obj);
    evas_object_size_hint_weight_set(sd->o_list, 1.0, 1.0);
    evas_object_show(sd->o_list);

    edje_object_part_swallow(sd->o_edje, "enna.swallow.content", sd->o_list);


    evas_object_smart_member_add(sd->o_edje, obj);

    evas_object_propagate_events_set(obj, 0);
}

static int _letter_timer_cb(void *data)
{

    return 0;
}
static void _smart_del(Evas_Object *obj)
{
    Eina_List *list = NULL;
    Eina_List *l;
    Eina_List *l_prev;
    List_Item *it;

    INTERNAL_ENTRY;

    evas_object_del(sd->o_list);
    evas_object_del(sd->o_edje);
    EINA_LIST_REVERSE_FOREACH_SAFE(sd->items, l, l_prev, it)
    {
	free(it);
	list = eina_list_remove_list(list, l);
    }
    free(sd);
}

static void _smart_show(Evas_Object *obj)
{
    INTERNAL_ENTRY ;
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object *obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY
    ;
    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY
    ;
    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;

    _smart_reconfigure(sd);
}

static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY
    ;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
    INTERNAL_ENTRY
    ;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object *obj)
{
    INTERNAL_ENTRY
    ;
    evas_object_clip_unset(sd->o_edje);
}

static void _smart_reconfigure(Smart_Data *sd)
{

    evas_object_move(sd->o_edje, sd->x, sd->y);
    evas_object_resize(sd->o_edje, sd->w, sd->h);
}

static void list_item_select(Smart_Data *sd, int n)
{
    List_Item *it;

    it = eina_list_nth(sd->items, n);
    if (!it) return;

    printf("n : %d\n", n);
    elm_genlist_item_show(it->item);
    elm_genlist_item_selected_set(it->item, 1);

}

static void list_set_item(Smart_Data *sd, int start, int up, int step)
{
    int n, ns;

    ns = start;
    n = ns;
    do
    {
        int i = up ? eina_list_count(sd->items) - 1 : 0;

        if (n == i)
        {
            n = ns;
            break;
        }
        n = up ? n + step : n - step;
    } while (0);

    if (n != ns)
        list_item_select(sd, n);
}

static void list_jump_to_ascii(Smart_Data *sd, char k)
{

}

static char list_get_letter_from_key(char key)
{
    switch (key)
    {
        case '7':
            return 'P';
        case '8':
            return 'T';
        case '9':
            return 'W';
        default:
            return ((key - 50) * 3 + 65);
    }
}

static void list_get_alpha_from_digit(Smart_Data *sd, char key)
{
    char letter[2];

    letter[0] = list_get_letter_from_key(key);
    letter[1] = '\0';

    sd->letter_mode = 1;

    if (!sd->letter_mode)
    {
        sd->letter_event_nbr = 0;
        sd->letter_key = key;
    }
    else
    {
        int mod;

        ecore_timer_del(sd->letter_timer);
        mod = (key == '7' || key == '9') ? 4 : 3;

        if (sd->letter_key == key)
            sd->letter_event_nbr = (sd->letter_event_nbr + 1) % mod;
        else
        {
            sd->letter_event_nbr = 0;
            sd->letter_key = key;
        }

        letter[0] += sd->letter_event_nbr;
    }

    edje_object_signal_emit(sd->o_edje, "letter,show", "enna");
    enna_log(ENNA_MSG_EVENT, NULL, "letter : %s", letter);
    edje_object_part_text_set(sd->o_edje, "enna.text.letter", letter);
    sd->letter_timer = ecore_timer_add(1.5, _letter_timer_cb, sd);
    list_jump_to_ascii(sd, letter[0]);
}

static void _smart_event_key_down(Smart_Data *sd, void *event_info)
{
    Evas_Event_Key_Down *ev;
    enna_key_t keycode;
    int ns;

    ev = event_info;
    ns = enna_list_selected_get(sd->o_smart);
    keycode = enna_get_key(ev);

    switch (keycode)
    {
        case ENNA_KEY_UP:
            list_set_item(sd, ns, 0, 1);
            break;
        case ENNA_KEY_PAGE_UP:
            list_set_item(sd, ns, 0, 5);
            break;
        case ENNA_KEY_DOWN:
            list_set_item(sd, ns, 1, 1);
            break;
        case ENNA_KEY_PAGE_DOWN:
            list_set_item(sd, ns, 1, 5);
            break;
        case ENNA_KEY_HOME:
            list_set_item(sd, -1, 1, 1);
            break;
        case ENNA_KEY_END:
            list_set_item(sd, eina_list_count(sd->items), 0, 1);
            break;
        case ENNA_KEY_OK:
        case ENNA_KEY_SPACE:
        {

	    List_Item *it = eina_list_nth(sd->items, 0);
	    if (it)
	    {
		if (it->func)
		    it->func(it->data);
	    }

        }
            break;
        case ENNA_KEY_2:
        case ENNA_KEY_3:
        case ENNA_KEY_4:
        case ENNA_KEY_5:
        case ENNA_KEY_6:
        case ENNA_KEY_7:
        case ENNA_KEY_8:
        case ENNA_KEY_9:
        {
            char key = ev->key[strlen(ev->key) - 1];
            list_get_alpha_from_digit(sd, key);
        }
            break;
        default:
            if (enna_key_is_alpha(keycode))
            {
                char k = enna_key_get_alpha(keycode);
                list_jump_to_ascii(sd, k);
            }
            break;
    }

    sd->on_hold = 0;
}
