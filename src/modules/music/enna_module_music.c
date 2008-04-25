/* Interface */

#include "enna.h"
static void           _create_gui();
static void           _class_init(int dummy);
static void           _class_shutdown(int dummy);
static void           _class_show(int dummy);
static void           _class_hide(int dummy);

static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

typedef struct _Enna_Module_Music Enna_Module_Music;

struct _Enna_Module_Music
{
   Evas *e;
   Evas_Object *o_edje;
   Evas_Object *o_scroll;
   Evas_Object *o_list;
   Enna_Module *em;
};

static Enna_Module_Music *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "music"
};

static Enna_Class_Activity class =
{
    "music",
    1,
    "music",
    NULL,
    "icon/music",
    {
        _class_init,
        _class_shutdown,
        _class_show,
        _class_hide
    },
    NULL
};



static void _class_init(int dummy)
{
    _create_gui();
    enna_content_append("music", mod->o_edje);
}

static void _class_shutdown(int dummy)
{
}

static void _class_show(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "show", "enna");
}

static void _class_hide(int dummy)
{
    edje_object_signal_emit(mod->o_edje, "hide", "enna");
}

static void _create_gui()
{
  Evas_Object *o;
  int i;
  Evas_Coord mw, mh;
  Evas_List *l, *categories;

  o = edje_object_add(mod->em->evas);
  edje_object_file_set(o, enna_config_theme_get(), "module/music");
  mod->o_edje = o;

  o =  enna_scrollframe_add(mod->em->evas);
  edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o);
  mod->o_scroll = o;

  o = enna_list_add(mod->em->evas);
  enna_scrollframe_child_set(mod->o_scroll, o);
  enna_list_icon_size_set(o, 64, 64);
  categories = enna_activity_categories_get("music");
  for( l = categories; l; l = l->next)
    {
       Evas_Object *icon;
       Enna_Class_Filesystem *cat;

       cat = l->data;

       icon = edje_object_add(mod->em->evas);
       edje_object_file_set(icon, enna_config_theme_get(), "icon/music");
       enna_list_append(o, icon, cat->label, 0, NULL, NULL);
    }
  enna_list_selected_set(o, 0);
  enna_list_min_size_get(o, &mw, &mh);;
  evas_object_resize(o, 500, mh);
  mod->o_list = o;
  evas_object_show(o);
}



/* Module interface */

static int
em_init(Enna_Module *em)
{
    mod = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;

    enna_activity_add(&class);

    return 1;
}


static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Music *mod;

    mod = em->mod;
    evas_object_del(mod->o_edje);
    evas_object_del(mod->o_scroll);
    evas_object_del(mod->o_list);
    return 1;
}

EAPI void
module_init(Enna_Module *em)
{
    if (!em)
        return;

    if (!em_init(em))
        return;
}

EAPI void
module_shutdown(Enna_Module *em)
{
    em_shutdown(em);
}
