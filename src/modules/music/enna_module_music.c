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
   Evas_Object *o_location;
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

static void _browse(void *data, void *data2)
{

   Enna_Class_Vfs *vfs = data;
   Enna_Vfs_File *file = data2;
   Evas_Coord mw, mh;

   if (!vfs) return;

   if (vfs->func.class_browse)
     {
	Evas_List *files, *l;

	if (!file)
	  files = vfs->func.class_browse(NULL);
	else if (file->is_directory)
	  files = vfs->func.class_browse(file->uri);
	else if (!file->is_directory)
	  {
	     printf("Select File\n");
	     return;
	  }


	if (evas_list_count(files))
	  {
	     enna_list_clear(mod->o_list);
	     evas_object_del(mod->o_list);

	     mod->o_list = enna_list_add(mod->em->evas);
	     enna_scrollframe_child_set(mod->o_scroll, mod->o_list);
	     enna_list_icon_size_set(mod->o_list, 64, 64);

	     enna_list_freeze(mod->o_list);
	     for (l = files; l; l = l->next)
	       {
		  Enna_Vfs_File *f;
		  Evas_Object *icon;

		  f = l->data;
		  icon = edje_object_add(mod->em->evas);
		  edje_object_file_set(icon, enna_config_theme_get(), "icon/music");
		  if (f->is_directory)
		    enna_list_append(mod->o_list, icon, f->label, 0, _browse, NULL,vfs, f);
		  else
		     enna_list_append(mod->o_list, NULL, f->label, 0, _browse, NULL,vfs, f);
		  }

	     enna_list_icon_size_set(mod->o_list, 64, 64);
	     enna_list_min_size_get(mod->o_list, &mw, &mh);;
	     evas_object_resize(mod->o_list, 600, mh);
	     enna_list_thaw(mod->o_list);
	     //evas_list_free(files);
	}
     }

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
  categories = enna_vfs_get(ENNA_CAPS_MUSIC);

  for( l = categories; l; l = l->next)
    {
       Evas_Object *icon;
       Enna_Class_Vfs *cat;

       cat = l->data;
       printf("cat->label : %s cat->icon : %s\n", cat->label, cat->icon);
       icon = edje_object_add(mod->em->evas);
       edje_object_file_set(icon, enna_config_theme_get(), "icon/music");
       enna_list_append(o, icon, cat->label, 0, _browse, NULL, cat, NULL);
    }
  //enna_list_selected_set(o, 0);
  enna_list_min_size_get(o, &mw, &mh);;
  evas_object_resize(o, 600, mh);
  mod->o_list = o;
  evas_object_show(o);

  Evas_Object *icon;
  icon = edje_object_add(mod->em->evas);
  edje_object_file_set(icon, enna_config_theme_get(), "icon/music");
  o = enna_location_add(mod->em->evas);
  edje_object_part_swallow(mod->o_edje, "enna.swallow.location", o);
  enna_location_append(o, icon, "Bénabar", NULL, NULL, NULL);
  enna_location_append(o, icon, "Les Risques du Métier", NULL, NULL, NULL);
  enna_location_append(o, icon, "Live au grand rex", NULL, NULL, NULL);
  /* evas_object_resize(o, 1000, 1000);*/

  /*
  Evas_Object *rect[10];
  int i = 0;
  Evas_Object *o;
  Evas_Coord mw, mh;

  o = edje_object_add(mod->em->evas);
  edje_object_file_set(o, enna_config_theme_get(), "module/music");
  mod->o_edje = o;

  for (i = 0; i < 10; i++)
    {
       rect[i] = evas_object_rectangle_add(mod->em->evas);
       evas_object_color_set(rect[i], i*10, i*10, i*10, 255);
       evas_object_resize(rect[i], 4*i, 4*i);
       evas_object_show(o);
    }

  o = enna_box_add(mod->em->evas);
  enna_box_align_set(o, 0.5, 0.5);
  enna_box_homogenous_set(o, 1);
  enna_box_orientation_set(o, 1);
  edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o);

  for (i = 0; i < 10; i++)
    {
       evas_object_geometry_get(rect[i], NULL, NULL, &mw, &mh);
       enna_box_freeze(o);
       enna_box_pack_end(o, rect[i]);
       enna_box_pack_options_set(rect[i], 0, 0, 0, 0, 0.0, 0.0,
				 mw, mh, 99999, 99999);
       enna_box_thaw(o);
       evas_object_show(rect[i]);
       }*/

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
