/* Interface */

#include "enna.h"

static void           _class_init(int dummy);
static void           _class_shutdown(int dummy);
static Evas_List     *_class_browse_up(const char *path);
static Evas_List     *_class_browse_down();

static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

typedef struct _Enna_Module_Lms Enna_Module_Lms;
typedef struct _Module_Config Module_Config;



struct _Module_Config
{
   Evas_List *root_directories;
};

struct _Enna_Module_Lms
{
   Evas *e;
   Enna_Module *em;
   Module_Config *config;
};

static Enna_Module_Lms *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "lms"
};

static Enna_Class_Vfs class =
{
    "lms",
    1,
    "Browse Library",
    NULL,
    "icon/dd",
    {
        _class_init,
        _class_shutdown,
	_class_browse_up,
	_class_browse_down,
    },
};


static Evas_List *_class_browse_up(const char *path)
{
    return NULL;
}



static Evas_List *_class_browse_down()
{
     return NULL;
}

static void _class_init(int dummy)
{
}

static void _class_shutdown(int dummy)
{
}

/* Module interface */

static int
em_init(Enna_Module *em)
{
   mod = calloc(1, sizeof(Enna_Module_Lms));
   mod->em = em;
   em->mod = mod;
   enna_vfs_append("lms", ENNA_CAPS_MUSIC, &class);

   return 1;
}


static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Lms *mod;
    mod = em->mod;;
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
