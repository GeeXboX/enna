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
             file = enna_vfs_create_directory (root->uri, root->label,
                                               "icon/hd", NULL);
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

                  f = enna_vfs_create_directory (dir, filename,
                                                 "icon/directory", NULL);
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

                  f = enna_vfs_create_file (dir, filename,
                                            "icon/music", NULL);
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
             file = enna_vfs_create_directory (root->uri, root->label,
                                               "icon/hd", NULL);
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

                  f = enna_vfs_create_directory (dir, filename,
                                                 "icon/directory", NULL);
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

                  f = enna_vfs_create_file (dir, filename,
                                            "icon/video", NULL);
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
                  file = enna_vfs_create_directory (root->uri, root->label,
                                                    "icon/hd", NULL);
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
                  file = enna_vfs_create_directory (root->uri, root->label,
                                                    "icon/hd", NULL);
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
   switch(type)
   {
    case ENNA_CAPS_MUSIC:
       return enna_vfs_create_directory ((char *) mod->music->uri,
                                         (char *) ecore_file_file_get (mod->music->uri),
                                         (char *) evas_stringshare_add ("icon/music"), NULL);
    case ENNA_CAPS_VIDEO:
       return enna_vfs_create_directory ((char *) mod->video->uri,
                                         (char *) ecore_file_file_get (mod->video->uri),
                                         (char *) evas_stringshare_add ("icon/video"), NULL);

   }

   return NULL;
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

static void __class_init(const char *name, Class_Private_Data **priv,
                         ENNA_VFS_CAPS caps, Enna_Class_Vfs *class, char *key)
{
   Class_Private_Data *data;
   Enna_Config_Data *cfgdata;
   Evas_List *l;

   data = calloc(1, sizeof(Class_Private_Data));
   *priv = data;
   
   enna_vfs_append(name, caps, class);
   data->prev_uri = NULL;

   data->config = calloc(1, sizeof(Module_Config));
   data->config->root_directories = NULL;

   cfgdata = enna_config_module_pair_get("localfiles");
   if (!cfgdata) return;
   for (l = cfgdata->pair; l; l = l->next)
     {
	Config_Pair *pair = l->data;
	if (!strcmp(pair->key, key))
	  {
	     Evas_List *dir_data;
	     enna_config_value_store(&dir_data, key, ENNA_CONFIG_STRING_LIST, pair);
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
		       printf ("Root Data: %s\n", root->uri);
		       root->icon = evas_list_nth(dir_data,2);
		       data->config->root_directories = evas_list_append(mod->music->config->root_directories, root);
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

   __class_init ("localfiles_music", &mod->music,
                 ENNA_CAPS_MUSIC, &class_music, "path_music");
   __class_init ("localfiles_video", &mod->video,
                 ENNA_CAPS_VIDEO, &class_video, "path_video");

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
