/* Interface */

#include "enna.h"

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
/*
EAPI Enna_Module_Class *class =
{
    "music",
    ENNA_MODULE_CLASS_ACTIVITY,
    {
        _class_init,
        _class_shutdown,
        _class_show,
        _class_hide
    }func;
}
*/
void _class_init(int dummy)
{
    enna_registry_activity_add("music", 0, "Music", NULL, "icon/music", NULL);
    enna_registry_activity_add("video", 1, "Video", NULL, "icon/video", NULL);
    enna_registry_activity_add("photos", 2, "Photos", NULL, "icon/music", NULL);
    enna_registry_activity_add("config", 3, "Configuration", NULL, "icon/music", NULL);
}

void _class_shutdown(int dummy)
{
}

void _class_show(int dummy)
{
    printf("Show Music Module\n");
}

void _class_hide(int dummy)
{
    printf("Hide Music Module\n");
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

    mod->menu = enna_list_add(em->evas);

    

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
