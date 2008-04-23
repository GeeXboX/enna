/* Interface */

#include "enna.h"

static void           _class_init(int dummy);
static void           _class_shutdown(int dummy);
static void           _class_show(int dummy);
static void           _class_hide(int dummy);

static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

typedef struct _Enna_Module_Video Enna_Module_Video;

struct _Enna_Module_Video
{
    Evas *e;
    Evas_Object *o_edje;
    Enna_Module *em;
};

static Enna_Module_Video *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "video"
};

static Enna_Module_Class class =
{
    "video",
    1,
    "video",
    NULL,
    "icon/video",
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
    printf("Show Video Module\n");
    evas_object_signal_emit(mod->o_edje, "show", "enna");
}

static void _class_hide(int dummy)
{
    printf("Hide Video Module\n");
    evas_object_signal_emit(mod->o_edje, "hide", "enna");
}

static void _create_gui()
{
  Evas_Object *o;

  o = edje_object_add(mod->em->evas);
  edje_object_file_set(o, enna_config_theme_get(), "module/video");

  mod->o_edje = o;
}



/* Module interface */

static int
em_init(Enna_Module *em)
{
    printf("Module Video Init\n");

    mod = calloc(1, sizeof(Enna_Module_Video));
    mod->em = em;
    em->mod = mod;

    enna_module_activity_add(mod->em, &class);
    _create_gui();
    //enna_content_add(mod->o_edje);

    return 1;
}


static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Video *mod;

    mod = em->mod;

    printf("Module Video Shutdown\n");

    evas_object_del(mod->o_edje);

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
