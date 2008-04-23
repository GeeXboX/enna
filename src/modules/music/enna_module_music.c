/* Interface */

#include "enna.h"

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
    Evas_Object *menu;
    Enna_Module *em;
};

static Enna_Module_Music *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "music"
};

static Enna_Module_Class class =
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
};

static void _class_init(int dummy)
{
    printf("class init\n");
}

static void _class_shutdown(int dummy)
{
}

static void _class_show(int dummy)
{
    printf("Show Music Module\n");
}

static void _class_hide(int dummy)
{
    printf("Hide Music Module\n");
}

static void _crate_gui()
{
  Evas_Object *o;

  o = edje_object_add(mod->em->evas);
  edje_object_file_set(o, enna_theme_get(), "module/music");
  enna_content_add("music", o);
}



/* Module interface */

static int
em_init(Enna_Module *em)
{
    printf("Module Music Init\n");

    mod = calloc(1, sizeof(Enna_Module_Music));
    mod->em = em;
    em->mod = mod;


    //enna_module_class_register(em, class);
    enna_module_activity_add(mod->em, &class);

    return 1;
}


static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Music *mod;

    mod = em->mod;

    printf("Module Music Shutdown\n");

    evas_object_del(mod->menu);

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
