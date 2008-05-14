/* Interface */

#include "enna.h"
static void           _create_gui();
static void           _class_init(int dummy);
static void           _class_shutdown(int dummy);
static void           _class_show(int dummy);
static void           _class_hide(int dummy);
static void           _class_event(void *event_info);
static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);
static void           _browse(void *data, void *data2);
static void           _browse_down();
static void           _select_down();
static void           _select_up();
static void           _activate();

typedef struct _Enna_Module_Music Enna_Module_Music;

struct _Enna_Module_Music
{
   Evas *e;
   Evas_Object *o_edje;
   Evas_Object *o_scroll;
   Evas_Object *o_list;
   Evas_Object *o_location;
   Enna_Class_Vfs *vfs;
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
        _class_hide,
	_class_event
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

static void _class_event(void *event_info)
{
   Evas_Event_Key_Down *ev;

   ev = (Evas_Event_Key_Down *) event_info;

   printf("Music Key pressed : %s\n", ev->key);

   if (!strcmp(ev->key, "BackSpace"))
     _browse_down();
   else if (!strcmp(ev->key, "Down"))
     _select_down();
   else if (!strcmp(ev->key, "Up"))
     _select_up();
   else if (!strcmp(ev->key, "Return"))
     _activate();

}

static void
_list_clear(void *data, Evas_Object *o, const char *sig, const char *src)

{

   printf("transition end\n");
   evas_object_del(data);
}

static void
_select_down()
{
   enna_list_selected_set(mod->o_list, enna_list_selected_get(mod->o_list) + 1);


}

static void
_select_up()
{
   enna_list_selected_set(mod->o_list, enna_list_selected_get(mod->o_list) - 1);
}

static void
_activate()
{
   Enna_Vfs_File *f;
   Enna_Class_Vfs *vfs;

   vfs = enna_list_selected_data_get(mod->o_list);
   f = enna_list_selected_data2_get(mod->o_list);
   _browse(vfs, f);

}

static void
_browse_down()
{

   if (!mod->vfs) printf("VFS == NULL\n");
   if (mod->vfs && mod->vfs->func.class_browse_down)
     {
	Evas_List *files, *l;
	Evas_Object *o;
	files = mod->vfs->func.class_browse_down();

	o = mod->o_list;

	enna_list_clear(o);
	enna_list_freeze(o);
	if (evas_list_count(files))
	  {
	     int i = 0;
	     /* Files returned : create list with items */
	     for (l = files, i = 0; l; l = l->next, i++)
	       {
		  Enna_Vfs_File *f;
		  Evas_Object *icon;

		  f = l->data;
		  icon = edje_object_add(mod->em->evas);
		  edje_object_file_set(icon, enna_config_theme_get(), f->icon);
		  enna_list_append(o, icon, f->label, 0, _browse, NULL, mod->vfs, f);
	       }


	  }
	else
	  {
	     /* No files returned: Create No media file item */
	     Evas_Object *icon;

	     icon = edje_object_add(mod->em->evas);
	     edje_object_file_set(icon, enna_config_theme_get(), "icon_nofile");
	     enna_list_append(o, icon, "No media found !", 0, NULL, NULL, NULL, NULL);
	     enna_list_selected_set(o, 0);
	  }
	enna_list_selected_set(o, 0);
	enna_list_thaw(o);

     }
}

static void _test(void *data, void *data2)
{

   printf("Clicked on location\n");

}

static void _browse(void *data, void *data2)
{

   Enna_Class_Vfs *vfs = data;
   Enna_Vfs_File *file = data2;
   Evas_Coord mw, mh;

   if (!vfs) return;

   if (vfs->func.class_browse_up)
     {
	Evas_List *files, *l;
	Evas_Object *o;

	if (!file)
	  {
	     /* file param is NULL => create Root menu */
	     files = vfs->func.class_browse_up(NULL);
	     enna_location_append(mod->o_location, "Root", _browse, vfs, NULL);
	  }
	else if (file->is_directory)
	  {
	     /* File selected is a directory */
	     enna_location_append(mod->o_location, file->label, _browse, vfs, file);
	     files = vfs->func.class_browse_up(file->uri);
	  }
	else if (!file->is_directory)
	  {
	     /* File selected is a regular file */
	     printf("Select File\n");
	     return;
	  }

	mod->vfs = vfs;
	o = mod->o_list;
	/* Clear list and add new items */
	enna_list_clear(o);
	enna_list_freeze(o);
	if (evas_list_count(files))
	  {
	     int i = 0;
	     /* Create list of files */
	     for (l = files, i = 0; l; l = l->next, i++)
	       {
		  Enna_Vfs_File *f;
		  Evas_Object *icon;

		  f = l->data;
		  icon = edje_object_add(mod->em->evas);
		  edje_object_file_set(icon, enna_config_theme_get(), f->icon);
		  enna_list_append(o, icon, f->label, 0, _browse, NULL,vfs, f);

	       }

	}
	else
	  {
	     /* No files returned : create no media item */
	     Evas_Object *icon;

	     icon = edje_object_add(mod->em->evas);
	     edje_object_file_set(icon, enna_config_theme_get(), "icon_nofile");
	     enna_list_append(o, icon, "No media found !", 0, NULL, NULL, NULL, NULL);
	  }
	enna_list_thaw(mod->o_list);
	enna_list_selected_set(o, 0);
     }

}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord mw, mh, vw, vh, w, h;

   enna_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   enna_list_min_size_get(data, &mw, &mh);
   evas_object_geometry_get(data, NULL, NULL, &w, &h);

   printf("scrollframe : %dx%d list : %dx%d list min :%dx%d\n", vw, vh, w, h, mw, mh);

   if (vw >= mw)
     {
   	if (w != vw && mw && mh)
	  {
	     printf("reisze list\n");
	     evas_object_resize(data, vw, vh);
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

  o = enna_list_add(mod->em->evas);
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

  mod->vfs = NULL;

  evas_object_show(o);
  enna_list_min_size_get(o, &mw, &mh);
  evas_object_resize(o, 640, 3000);
  mod->o_list = o;

  o = enna_scrollframe_add(mod->em->evas);
  enna_scrollframe_policy_set(o, ENNA_SCROLLFRAME_POLICY_AUTO,
			      ENNA_SCROLLFRAME_POLICY_AUTO);

  enna_scrollframe_child_set(o, mod->o_list);
  edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o);
  evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _e_wid_cb_scrollframe_resize, mod->o_list);
  //evas_object_resize(o, 500, 500);
  mod->o_scroll = o;



  o = enna_location_add(mod->em->evas);
  edje_object_part_swallow(mod->o_edje, "enna.swallow.location", o);
  enna_location_append(o, "Music", NULL, NULL, NULL);
  mod->o_location = o;



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
