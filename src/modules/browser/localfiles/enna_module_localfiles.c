/* Interface */

#include "enna.h"

static void           _class_init();
static void           _class_shutdown();
//static Evas_List     *_class_browse_up(const char *path, int type);
//static Evas_List     *_class_browse_down(int type);
static Enna_Vfs_File *_class_vfs_get(int type);

static Evas_List     *_class_browse_up_music(const char *path);
static Evas_List     *_class_browse_down_music(void);
static Enna_Vfs_File *_class_vfs_get_music(void);

static Evas_List     *_class_browse_up_video(const char *path);
static Evas_List     *_class_browse_down_video(void);
static Enna_Vfs_File *_class_vfs_get_video(void);

static unsigned char _uri_has_extension(const char *uri, int type);
static unsigned char _uri_is_root(const char *uri, int type);

static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

typedef struct _Root_Directories Root_Directories;
typedef struct _Enna_Module_Music Enna_Module_Music;
typedef struct _Module_Config Module_Config;
typedef struct _Class_Private_Data Class_Private_Data;

struct _Root_Directories
{
   char *uri;
   char *label;
   char *icon;
};

struct _Module_Config
{
   Evas_List *root_directories;
};




struct _Class_Private_Data
{
   const char *uri;
   const char *prev_uri;
   Module_Config *config;
};

struct _Enna_Module_Music
{
   Evas *e;
   Enna_Module *em;
   Class_Private_Data *music;
   Class_Private_Data *video;
};

static Enna_Module_Music *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "localfiles"
};

static Enna_Class_Vfs class_music =
{
    "localfiles_music",
    1,
    "Browse Local Files",
    NULL,
    "icon/hd",
    {
        _class_init,
        _class_shutdown,
	_class_browse_up_music,
	_class_browse_down_music,
	_class_vfs_get_music,
    },
};

static Enna_Class_Vfs class_video =
{
    "localfiles_video",
    1,
    "Browse Local Files",
    NULL,
    "icon/hd",
    {
        _class_init,
        _class_shutdown,
	_class_browse_up_video,
	_class_browse_down_video,
	_class_vfs_get_video,
    },
};


static Evas_List *_class_browse_up_music(const char *path)
{

   /* Browse Root */
   if (!path)
     {
	Evas_List *files = NULL;
	Evas_List *l;
	/* FIXME: this list should come from config ! */
	for (l = mod->music->config->root_directories; l; l = l->next)
	  {
	     Enna_Vfs_File *file;
	     Root_Directories *root;

	     root = l->data;
	     file = calloc(1, sizeof(Enna_Vfs_File));
	     file->uri = root->uri;
	     file->label = root->label;
	     file->icon_file = NULL;
	     file->is_directory = 1;
	     file->icon = "icon/hd";
	     files = evas_list_append(files, file);
	  }
	//evas_stringshare_del(mod->music->prev_uri);
	//evas_stringshare_del(mod->music->uri);
	mod->music->prev_uri = NULL;
	mod->music->uri = NULL;
	return files;
     }
   else if (strstr(path, "file://"))
     {
	Ecore_List *files = NULL;
	char *filename = NULL;
	Evas_List *files_list = NULL;
	Evas_List *dirs_list = NULL;
	Evas_List *l;
	char dir[PATH_MAX];

	files = ecore_file_ls(path+7);
	ecore_list_sort(files, ECORE_COMPARE_CB(strcasecmp), ECORE_SORT_MIN);
	filename = ecore_list_first_goto(files);

	while ((filename = (char *)ecore_list_next(files)) != NULL)
	  {
	     sprintf(dir, "%s/%s", path, filename);
	     if (filename[0] == '.')
	       continue;
	     else if (ecore_file_is_dir(dir+7))
	       {
		  Enna_Vfs_File  *f;

		  f = calloc(1, sizeof(Enna_Vfs_File));
		  f->uri = strdup(dir);
		  f->label = strdup(filename);
		  f->is_directory = 1;
		  f->icon = "icon/directory";
		  dirs_list = evas_list_append(dirs_list, f);
		  if (mod->music->prev_uri)
		    {
		       if (!strcmp(dir, mod->music->prev_uri))
			 f->is_selected = 1;
		       else
			 f->is_selected = 0;
		    }
	       }
	     else if (_uri_has_extension(dir, ENNA_CAPS_MUSIC))
	       {
		  Enna_Vfs_File  *f;
		  f = calloc(1, sizeof(Enna_Vfs_File));
		  f->uri  = strdup(dir);
		  f->label = strdup(filename);
		  f->icon =  "icon/music";
		  f->is_directory = 0;
		  files_list = evas_list_append(files_list, f);
	       }
	  }
	/* File after dir */
	for (l = files_list; l; l = l->next)
	  {
	     dirs_list = evas_list_append(dirs_list, l->data);
	  }
	//evas_stringshare_del(mod->music->prev_uri);
	mod->music->prev_uri = mod->music->uri;
	mod->music->uri = evas_stringshare_add(path);
	return dirs_list;
     }

   return NULL;

}

static Evas_List *_class_browse_up_video(const char *path)
{

   /* Browse Root */
   if (!path)
     {
	Evas_List *files = NULL;
	Evas_List *l;
	for (l = mod->video->config->root_directories; l; l = l->next)
	  {
	     Enna_Vfs_File *file;
	     Root_Directories *root;

	     root = l->data;
	     file = calloc(1, sizeof(Enna_Vfs_File));
	     file->uri = root->uri;
	     file->label = root->label;
	     file->icon_file = NULL;
	     file->is_directory = 1;
	     file->icon = "icon/hd";
	     files = evas_list_append(files, file);
	  }
	//evas_stringshare_del(mod->video->prev_uri);
	//evas_stringshare_del(mod->video->uri);
	mod->video->prev_uri = NULL;
	mod->video->uri = NULL;
	return files;
     }
   else if (strstr(path, "file://"))
     {
	Ecore_List *files = NULL;
	char *filename = NULL;
	Evas_List *files_list = NULL;
	Evas_List *dirs_list = NULL;
	Evas_List *l;
	char dir[PATH_MAX];

	files = ecore_file_ls(path+7);
	ecore_list_sort(files, ECORE_COMPARE_CB(strcasecmp), ECORE_SORT_MIN);
	filename = ecore_list_first_goto(files);

	while ((filename = (char *)ecore_list_next(files)) != NULL)
	  {
	     sprintf(dir, "%s/%s", path, filename);
	     if (filename[0] == '.')
	       continue;
	     else if (ecore_file_is_dir(dir+7))
	       {
		  Enna_Vfs_File  *f;

		  f = calloc(1, sizeof(Enna_Vfs_File));
		  f->uri = strdup(dir);
		  f->label = strdup(filename);
		  f->is_directory = 1;
		  f->icon = "icon/directory";
		  dirs_list = evas_list_append(dirs_list, f);
		  if (mod->video->prev_uri)
		    {
		       if (!strcmp(dir, mod->video->prev_uri))
			 f->is_selected = 1;
		       else
			 f->is_selected = 0;
		    }
	       }
	     else if (_uri_has_extension(dir, ENNA_CAPS_VIDEO))
	       {
		  Enna_Vfs_File  *f;
		  f = calloc(1, sizeof(Enna_Vfs_File));
		  f->uri  = strdup(dir);
		  f->label = strdup(filename);
		  f->icon = "icon/video";
		  f->is_directory = 0;
		  files_list = evas_list_append(files_list, f);
	       }
	  }
	/* File after dir */
	for (l = files_list; l; l = l->next)
	  {
	     dirs_list = evas_list_append(dirs_list, l->data);
	  }
	//evas_stringshare_del(mod->video->prev_uri);
	mod->video->prev_uri = mod->video->uri;
	mod->video->uri = evas_stringshare_add(path);
	return dirs_list;
     }

   return NULL;

}

static unsigned char _uri_has_extension(const char *uri, int type)
{

   Evas_List *l;

   if (type == ENNA_CAPS_MUSIC)
     {

	for (l = enna_config->music_filters; l; l = l->next)
	  {
	     const char *ext = l->data;
	     if(ecore_str_has_extension(uri, ext))
	       return 1;
	  }
     }

   else if (type == ENNA_CAPS_VIDEO)
     {

	for (l = enna_config->video_filters; l; l = l->next)
	  {
	     const char *ext = l->data;
	     if(ecore_str_has_extension(uri, ext))
	       return 1;
	  }
     }
   return 0;

}

static unsigned char _uri_is_root(const char *uri, int type)
{
   Evas_List *l;
   if (type == ENNA_CAPS_MUSIC)
     {
	for (l = mod->music->config->root_directories; l; l = l->next)
	  {
	     Root_Directories *root = l->data;
	     if (!strcmp(root->uri, uri))
	       return 1;
	  }
     }
   else
   if (type == ENNA_CAPS_VIDEO)
     {
	for (l = mod->video->config->root_directories; l; l = l->next)
	  {
	     Root_Directories *root = l->data;
	     if (!strcmp(root->uri, uri))
	       return 1;
	  }
     }
   return 0;
}

static Evas_List *
_class_browse_down_music(void)
{
   /* Browse Root */
   if (mod->music->uri && strstr(mod->music->uri, "file://"))
     {
	char *path_tmp = NULL;
	char *p;
	Evas_List *files = NULL;

	if (_uri_is_root(mod->music->uri, ENNA_CAPS_MUSIC))
	  {
	     Evas_List *files = NULL;
	     Evas_List *l;
	     for (l = mod->music->config->root_directories; l; l = l->next)
	       {
		  Enna_Vfs_File *file;
		  Root_Directories *root;

		  root = l->data;
		  file = calloc(1, sizeof(Enna_Vfs_File));
		  file->uri = root->uri;
		  file->label = root->label;
		  file->icon_file = NULL;
		  file->is_directory = 1;
		  file->icon = "icon/hd";
		  files = evas_list_append(files, file);
	       }
	     mod->music->prev_uri = NULL;
	     mod->music->uri = NULL;
	     return files;
	  }

	path_tmp = strdup(mod->music->uri);
	if (path_tmp[strlen(mod->music->uri) - 1] == '/')
	  path_tmp[strlen(mod->music->uri) - 1] = 0;
	p = strrchr(path_tmp, '/');
	if (p && *(p - 1) == '/')
	  *(p) = 0;
	else if (p)
	  *(p) = 0;

	files = _class_browse_up_music(path_tmp);
	mod->music->uri = evas_stringshare_add(path_tmp);
	return files;
     }

   return NULL;
}

static Evas_List *
_class_browse_down_video(void)
{
   /* Browse Root */
   if (mod->video->uri && strstr(mod->video->uri, "file://"))
     {
	char *path_tmp = NULL;
	char *p;
	Evas_List *files = NULL;

	if (_uri_is_root(mod->video->uri, ENNA_CAPS_VIDEO))
	  {
	     Evas_List *files = NULL;
	     Evas_List *l;
	     for (l = mod->video->config->root_directories; l; l = l->next)
	       {
		  Enna_Vfs_File *file;
		  Root_Directories *root;

		  root = l->data;
		  file = calloc(1, sizeof(Enna_Vfs_File));
		  file->uri = root->uri;
		  file->label = root->label;
		  file->icon_file = NULL;
		  file->is_directory = 1;
		  file->icon = "icon/hd";
		  files = evas_list_append(files, file);
		  }
	     mod->video->prev_uri = NULL;
	     mod->video->uri = NULL;
	     return files;
	  }

	path_tmp = strdup(mod->video->uri);
	if (path_tmp[strlen(mod->video->uri) - 1] == '/')
	  path_tmp[strlen(mod->video->uri) - 1] = 0;
	p = strrchr(path_tmp, '/');
	if (p && *(p - 1) == '/')
	  *(p) = 0;
	else if (p)
	  *(p) = 0;

	files = _class_browse_up_video(path_tmp);
	mod->video->uri = evas_stringshare_add(path_tmp);
	return files;
     }

   return NULL;
}

static Enna_Vfs_File *
_class_vfs_get(int type)
{
   Enna_Vfs_File  *f;
   f = calloc(1, sizeof(Enna_Vfs_File));

   switch(type)
   {
    case ENNA_CAPS_MUSIC:
       f->uri  = mod->music->uri;
       f->label = ecore_file_file_get(mod->music->uri);;
       f->icon = evas_stringshare_add("icon/music");
       break;
    case ENNA_CAPS_VIDEO:
       f->uri  = mod->video->uri;
       f->label = ecore_file_file_get(mod->video->uri);
       f->icon = evas_stringshare_add("icon/video");
       break;
   }
   f->is_directory = 1;
   return f;
}

static Enna_Vfs_File *
_class_vfs_get_music(void)
{
   return _class_vfs_get(ENNA_CAPS_MUSIC);
}

static Enna_Vfs_File *
_class_vfs_get_video(void)
{
   return _class_vfs_get(ENNA_CAPS_VIDEO);
}



static void _class_init(int dummy)
{
}


static void _class_shutdown(int dummy)
{
}

static void _class_init_music(int dummy)
{
   Class_Private_Data *data;
   Enna_Config_Data *cfgdata;
   Evas_List *l;

   data = calloc(1, sizeof(Class_Private_Data));
   mod->music = data;

   enna_vfs_append("localfiles_music", ENNA_CAPS_MUSIC, &class_music);
   mod->music->prev_uri = NULL;

   mod->music->config = calloc(1, sizeof(Module_Config));
   mod->music->config->root_directories = NULL;
   cfgdata = enna_config_module_pair_get("localfiles");
   if (!cfgdata) return;
   for (l = cfgdata->pair; l; l = l->next)
     {
	Config_Pair *pair = l->data;
	if (!strcmp(pair->key, "path_music"))
	  {
	     Evas_List *dir_data;
	     enna_config_value_store(&dir_data, "path_music", ENNA_CONFIG_STRING_LIST, pair);
	     if(dir_data)
	       {
		  if(evas_list_count(dir_data) != 3)
		    continue;
		  else
		    {
		       Root_Directories *root;

		       root = calloc(1, sizeof(Root_Directories));
		       root->uri = evas_list_nth(dir_data,0);
		       root->label = evas_list_nth(dir_data, 1);
		       printf ("Root Data Music : %s\n", root->uri);
		       root->icon = evas_list_nth(dir_data,2);
		       mod->music->config->root_directories = evas_list_append(mod->music->config->root_directories, root);
		    }
	       }
	  }
     }
}

static void _class_init_video(int dummy)
{
   Class_Private_Data *data;
   Enna_Config_Data *cfgdata;
   Evas_List *l;

   data = calloc(1, sizeof(Class_Private_Data));
   mod->video = data;

   enna_vfs_append("localfiles_video", ENNA_CAPS_VIDEO, &class_video);
   mod->video->prev_uri = NULL;

   mod->video->config = calloc(1, sizeof(Module_Config));
   mod->video->config->root_directories = NULL;
   cfgdata = enna_config_module_pair_get("localfiles");
   if (!cfgdata) return;
   for (l = cfgdata->pair; l; l = l->next)
     {
	Config_Pair *pair = l->data;
	if (!strcmp(pair->key, "path_video"))
	  {
	     Evas_List *dir_data;
	     enna_config_value_store(&dir_data, "path_video", ENNA_CONFIG_STRING_LIST, pair);
	     if(dir_data)
	       {
		  if(evas_list_count(dir_data) != 3)
		    continue;
		  else
		    {
		       Root_Directories *root;
		       root = calloc(1, sizeof(Root_Directories));
		       root->uri = evas_list_nth(dir_data,0);
		       root->label = evas_list_nth(dir_data, 1);
		       printf ("Root Data Video : %s\n", root->uri);
		       root->icon = evas_list_nth(dir_data,2);
		       mod->video->config->root_directories = evas_list_append(mod->video->config->root_directories, root);
		    }
	       }
	  }
     }
}

/* Module interface */

static int
em_init(Enna_Module *em)
{

   mod = calloc(1, sizeof(Enna_Module_Music));
   mod->em = em;
   em->mod = mod;

   _class_init_music(0);
   _class_init_video(0);

   return 1;
}



static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Music *mod;

    mod = em->mod;;
    free(mod->music);
    free(mod->video);
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
