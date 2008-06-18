#ifndef _ENNA_CONFIG_H_
#define _ENNA_CONFIG_H_

typedef struct _Enna_Config_Root_Directories Enna_Config_Root_Directories;
typedef struct _Enna_Config Enna_Config;

struct _Enna_Config_Root_Directories
{
   const char *uri;
   const char *label;
};

struct _Enna_Config
{
   /* Theme */
   const char                    *theme;
   const char                    *theme_file;
   int                            fullscreen;
   const char                    *engine;
   const char                    *backend;
   /* Module Music */
   Evas_List                     *music_local_root_directories;
   Evas_List                     *music_filters;
};

Enna_Config           *enna_config;

EAPI void             enna_config_init(void);
EAPI void             enna_config_shutdown(void);
EAPI const char      *enna_config_theme_get(void);

#endif
