/* Interface */

#include "enna.h"
#include "smart_player.h"

static void           _create_gui();
static void           _create_mediaplayer_gui();
static void           _class_init(int dummy);
static void           _class_shutdown(int dummy);
static void           _class_show(int dummy);
static void           _class_hide(int dummy);
static void           _class_event(void *event_info);
static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);
static void           _list_transition_core(Evas_List *files, unsigned char direction);
static void           _list_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src);
static void           _list_transition_right_end_cb(void *data, Evas_Object *o, const char *sig, const char *src);
static void           _browse(void *data, void *data2);
static void           _browse_down();
static void           _activate();

typedef struct _Enna_Module_Music Enna_Module_Music;

typedef enum _MUSIC_STATE MUSIC_STATE;

enum _MUSIC_STATE
{
  LIST_VIEW,
  MEDIAPLAYER_VIEW,
  DEFAULT_VIEW
};

  struct _Enna_Module_Music
  {
     Evas *e;
     Evas_Object *o_edje;
     Evas_Object *o_list;
     Evas_Object *o_location;
     Enna_Class_Vfs *vfs;
     Enna_Module *em;
     MUSIC_STATE state;
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
   edje_object_signal_emit(mod->o_edje, "module,show", "enna");
   switch  (mod->state)
     {
      case LIST_VIEW:
	 edje_object_signal_emit(mod->o_edje, "list,show", "enna");
	 edje_object_signal_emit(mod->o_edje, "mediaplayer,hide", "enna");
	 break;
      case MEDIAPLAYER_VIEW:
	 edje_object_signal_emit(mod->o_edje, "mediaplayer,show", "enna");
	 edje_object_signal_emit(mod->o_edje, "list,hide", "enna");
	 break;
      default:
	 printf("Error State Unknown in music module\n");
     }


}

static void _class_hide(int dummy)
{
   edje_object_signal_emit(mod->o_edje, "module,hide", "enna");
}

static void _class_event(void *event_info)
{
   Evas_Event_Key_Down *ev;

   ev = (Evas_Event_Key_Down *) event_info;

   if (!strcmp(ev->key, "BackSpace") || !strcmp(ev->key, "Left"))
     _browse_down();
   else if (!strcmp(ev->key, "Return") ||!strcmp(ev->key, "KP_Enter") || !strcmp(ev->key, "Right"))
     _activate();
   else
     enna_list_event_key_down(mod->o_list, event_info);

}

static void
_activate()
{
   Enna_Vfs_File *f;
   Enna_Class_Vfs *vfs;

   vfs = (Enna_Class_Vfs*)enna_list_selected_data_get(mod->o_list);
   f = (Enna_Vfs_File*)enna_list_selected_data2_get(mod->o_list);
   _browse(vfs, f);

}

static void
_list_transition_core(Evas_List *files, unsigned char direction)
{
   Evas_Object *o_list, *oe;
   Evas_List *l;

   o_list = mod->o_list;
   oe = enna_list_edje_object_get(o_list);
   if (!direction)
     edje_object_signal_callback_del(oe, "list,transition,end", "edje", _list_transition_left_end_cb);
   else
     edje_object_signal_callback_del(oe, "list,transition,end", "edje", _list_transition_right_end_cb);

   enna_list_freeze(o_list);
   evas_object_del(o_list);

   o_list = enna_list_add(mod->em->evas);
   oe = enna_list_edje_object_get(o_list);
   evas_object_show(o_list);
   edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o_list);

   if (direction == 0)
     edje_object_signal_emit(oe, "list,right,now", "enna");
   else
     edje_object_signal_emit(oe, "list,left,now", "enna");

   enna_list_freeze(o_list);
   enna_list_icon_size_set(o_list, 64, 64);
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
	     enna_list_append(o_list, icon, f->label, 0, _browse, NULL,mod->vfs, f);

	  }

     }
   else
     {
	/* No files returned : create no media item */
	Evas_Object *icon;

	icon = edje_object_add(mod->em->evas);
	edje_object_file_set(icon, enna_config_theme_get(), "icon_nofile");
	enna_list_append(o_list, icon, "No media found !", 0, NULL, NULL, NULL, NULL);
     }

   enna_list_thaw(o_list);
   enna_list_selected_set(o_list, 0);
   mod->o_list = o_list;
   edje_object_signal_emit(oe, "list,default", "enna");
}

static void
_list_transition_left_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{

   _list_transition_core(data, 0);


}

static void
_list_transition_right_end_cb(void *data, Evas_Object *o, const char *sig, const char *src)
{
   _list_transition_core(data, 1);
}

static void
_browse_down()
{
   if (mod->vfs && mod->vfs->func.class_browse_down)
     {
	Evas_Object *o, *oe;
	Evas_List *files;

	files = mod->vfs->func.class_browse_down();
	o = mod->o_list;
	/* Clear list and add new items */
	oe = enna_list_edje_object_get(o);
	edje_object_signal_callback_add(oe, "list,transition,end", "edje", _list_transition_right_end_cb, files);
	edje_object_signal_emit(oe, "list,right", "enna");
	enna_location_remove_nth(mod->o_location, enna_location_count(mod->o_location) - 1);
     }
}



static void _browse(void *data, void *data2)
{

   Enna_Class_Vfs *vfs = data;
   Enna_Vfs_File *file = data2;
   Evas_List *files, *l;

   if (!vfs) return;

   if (vfs->func.class_browse_up)
     {

	Evas_Object *o, *oe;

	if (!file)
	  {
	     Evas_Object *icon;
	     /* file param is NULL => create Root menu */
	     files = vfs->func.class_browse_up(NULL);
	     icon = edje_object_add(mod->em->evas);
	     edje_object_file_set(icon, enna_config_theme_get(), "icon/home_mini");
	     enna_location_append(mod->o_location, "Root", icon, _browse, vfs, NULL);
	  }
	else if (file->is_directory)
	  {
	     /* File selected is a directory */
	     enna_location_append(mod->o_location, file->label, NULL, _browse, vfs, file);
	     files = vfs->func.class_browse_up(file->uri);
	  }
	else if (!file->is_directory)
	  {
	     /* File selected is a regular file */
	     char *tmp;
	     char *p;
	     int i = 0;

	     tmp = strdup(file->uri);

	     p = strrchr(tmp, '/');
	     if (p && *(p - 1) == '/')
	       *(p) = 0;
	     else if (p)
	       *(p) = 0;

	     files = vfs->func.class_browse_up(tmp);
	     free(tmp);
	     enna_mediaplayer_playlist_clear();
	     enna_mediaplayer_stop();
	     for (l = files; l; l = l->next)
	       {
		  Enna_Vfs_File *f;
		  f = l->data;

		  if (!f->is_directory)
		    {
		       printf("add %s\n", f->uri);
		       enna_mediaplayer_uri_append(f->uri);
		       if (!strcmp(f->uri, file->uri))
			 {
			    enna_mediaplayer_select_nth(i);
			    enna_mediaplayer_play();
			 }
		       i++;
		    }

	       }

	     _create_mediaplayer_gui();

	     return;
	  }

	mod->vfs = vfs;
	o = mod->o_list;
	/* Clear list and add new items */
	oe = enna_list_edje_object_get(o);
	edje_object_signal_callback_add(oe, "list,transition,end", "edje", _list_transition_left_end_cb, files);
	edje_object_signal_emit(oe, "list,left", "enna");

     }

}

static void _create_mediaplayer_gui()
{
   Evas_Object *o;

   mod->state = MEDIAPLAYER_VIEW;
   edje_object_signal_emit(mod->o_edje, "mediaplayer,show", "enna");
   edje_object_signal_emit(mod->o_edje, "list,hide", "enna");

   o = enna_smart_player_add(mod->em->evas);
   edje_object_part_swallow(mod->o_edje, "enna.swallow.mediaplayer", o);
   evas_object_show(o);

}

static void _create_gui()
{

   Evas_Object *o, *oe;
   Evas_Object *icon;
   Evas_List *l, *categories;

   mod->state = LIST_VIEW;

   o = edje_object_add(mod->em->evas);
   edje_object_file_set(o, enna_config_theme_get(), "module/music");
   mod->o_edje = o;

   /* Create List */
   o = enna_list_add(mod->em->evas);
   oe = enna_list_edje_object_get(o);
   enna_list_freeze(o);
   edje_object_signal_emit(oe, "list,right,now", "enna");

   categories = enna_vfs_get(ENNA_CAPS_MUSIC);
   enna_list_icon_size_set(o, 64, 64);
   for( l = categories; l; l = l->next)
     {

	Enna_Class_Vfs *cat;

	cat = l->data;
	icon = edje_object_add(mod->em->evas);
	edje_object_file_set(icon, enna_config_theme_get(), "icon/music");
	enna_list_append(o, icon, cat->label, 0, _browse, NULL, cat, NULL);
     }
   enna_list_thaw(o);
   mod->vfs = NULL;
   evas_object_show(o);
   mod->o_list = o;
   edje_object_part_swallow(mod->o_edje, "enna.swallow.list", o);
   edje_object_signal_emit(oe, "list,default", "enna");

   /* Create Location bar */
   o = enna_location_add(mod->em->evas);
   edje_object_part_swallow(mod->o_edje, "enna.swallow.location", o);

   icon = edje_object_add(mod->em->evas);
   edje_object_file_set(icon, enna_config_theme_get(), "icon/music_mini");
   enna_location_append(o, "Music", icon, NULL, NULL, NULL);
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
   evas_object_del(mod->o_edje);
   evas_object_del(mod->o_list);
   evas_object_del(mod->o_location);
   free(mod);
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
