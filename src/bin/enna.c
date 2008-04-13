/*
 * enna.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna.c is free software copyrighted by Nicolas Aguirre.
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
 * enna.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
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

#include "enna.h"
#include "enna_inc.h"

/* Global Variable Enna *enna*/
Enna *enna;

/* Callbacks */
static void         _resize_viewport_cb(Ecore_Evas * ee);

/* Functions */
static int          enna_init(int run_gl);
static void         _create_gui(void);
static int          _event_quit(void *data, int ev_type, void *ev);

/* Calbacks */

static int
_event_quit(void *data, int ev_type, void *ev)
{
   Ecore_Event_Signal_Exit *e;

   e = (Ecore_Event_Signal_Exit *) ev;

   if (e)
     {
	if (e->interrupt)
	  dbg("Exit: interrupt\n");
	if (e->quit)
	  dbg("Exit: quit\n");
	if (e->terminate)
	  dbg("Exit: terminate\n");
     }
   ecore_main_loop_quit();
   return 1;
}

static void
_event_bg_key_down_cb(void *data, Evas * e, Evas_Object * obj,
                      void *event_info)
{
   Enna               *enna;
   Evas_Event_Key_Down *ev;

   enna = (Enna *) data;
   ev = (Evas_Event_Key_Down *) event_info;

   if (!ev || !enna)
     return;


 
   if (!strcmp(ev->key, "q"))
     {
	ecore_main_loop_quit();
     }

}

/* Functions */

static int
_enna_init(int run_gl)
{
   Evas_Coord          w, h;
   char                tmp[PATH_MAX];
   int                 i;

   enna->home = enna_util_user_home_get();

   ecore_init();
   ecore_file_init();
   ecore_evas_init();
   edje_init();
   enna_module_init();

   sprintf(tmp, "%s/.enna", enna->home);

   if (!ecore_file_exists(tmp))
     ecore_file_mkdir(tmp);

   sprintf(tmp, "%s/.enna/covers", enna->home);
   if (!ecore_file_exists(tmp))
     ecore_file_mkdir(tmp);

   enna_scanner_init();

   if (run_gl)
     enna->ee = ecore_evas_gl_x11_new(NULL, 0, 0, 0, 64, 64);
   else
     enna->ee = ecore_evas_software_x11_new(NULL, 0, 0, 0, 64, 64);

   if (!enna->ee)
     {
	dbg("Can not Initialize Ecore Evas !\n");
	return 0;
     }

   ecore_evas_title_set(enna->ee, "enna HTPC");
   ecore_evas_name_class_set(enna->ee, "enna", "enna");
   ecore_evas_borderless_set(enna->ee, 0);
   ecore_evas_shaped_set(enna->ee, 1);
   enna->evas = ecore_evas_get(enna->ee);

   evas_data_attach_set(enna->evas, enna);

   ecore_evas_show(enna->ee);
   _create_gui();
   return 1;
}




static void
_create_gui()
{
   Evas_Object *o;
   Evas_Coord w, h;
   Enna_Module *em;

   o = edje_object_add(enna->evas);
   edje_object_file_set(o, enna_config_theme_get(), "enna");
   edje_object_size_min_get(o, &w, &h);
   evas_object_resize(o, w, h);
   evas_object_move(o, 0, 0);
   evas_object_show(o);
   ecore_evas_resize(enna->ee, w, h);
   enna->o_edje = o;

   o = enna_background_add(enna->evas);
   edje_object_part_swallow(enna->o_edje, "enna.swallow.background", o);
   enna->o_background = o;

   o = enna_mainmenu_add(enna->evas);
   edje_object_part_swallow(enna->o_edje, "enna.swallow.mainmenu", o);
   enna->o_mainmenu = o;

   edje_object_signal_emit(enna->o_edje, "mainmenu,show", "enna");
   evas_object_focus_set(enna->o_edje, 1);
   evas_object_event_callback_add(enna->o_edje,
				  EVAS_CALLBACK_KEY_DOWN,
				  _event_bg_key_down_cb, enna);
   /* Create Modules */
   em = enna_module_open("music");
   enna_module_enable(em);


   ecore_evas_show(enna->ee);
}

static void
_enna_shutdown()
{
   evas_object_del(enna->o_background);
   evas_object_del(enna->o_edje);
   evas_object_del(enna->o_mainmenu);

   ENNA_FREE(enna->home);
   edje_shutdown();
   ecore_evas_shutdown();
   ecore_file_shutdown();
   ecore_shutdown();
   evas_shutdown();
   enna_scanner_shutdown();
   enna_config_shutdown();
   enna_module_shutdown();
   ENNA_FREE(enna);
}

static void
usage(char *binname)
{
   dbg("Usage: %s [-c filename] [-fs] [-th theme_name]\n", binname);
   exit(0);
}

int
main(int arc, char **arv)
{

   char               *binname = arv[0];
   char               *conffile = NULL;
   char               *theme_name = NULL;
   int                 run_fullscreen = 0;
   int                 run_gl = 0;

   arv++;
   arc--;

   while (arc)
     {
	if (!strcmp("-fs", *arv))
	  {
	     run_fullscreen = 1;
	     arv++;
	     arc--;
	  }
	else if (!strcmp("-gl", *arv))
	  {
	     run_gl = 1;
	     arv++;
	     arc--;
	  }
	else if (!strcmp("-c", *arv))
	  {
	     arv++;
	     if (!--arc)
	       usage(binname);
	     conffile = strdup(*arv);
	     arc--;
	     arv++;
	  }
	else if (!strcmp("-th", *arv))
	  {
	     arv++;
	     if (!--arc)
	       usage(binname);
	     theme_name = strdup(*arv);
	     dbg("theme in use : %s\n", theme_name);
	     arc--;
	     arv++;
	  }
	else
	  usage(binname);
     }

   /* Must be called first */
   enna_config_init(conffile, theme_name);

   enna = (Enna *) malloc(sizeof(Enna));

   if (!_enna_init(run_gl))
     return 0;

   ecore_main_loop_begin();

   _enna_shutdown();
   dbg("Bye Bye !\n");

   return 0;
}
