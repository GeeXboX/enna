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

static Enna_Class_Input class =
{
    "lirc",
    {
        _class_init,
        _class_shutdown,
        NULL
    },
    NULL
};

static void _class_init(int dummy)
{
    printf("class LIRC INPUT init\n");
}

static void _class_shutdown(int dummy)
{
    printf("class LIRC INPUT shutdown\n");
}


/* Module interface */

static int
em_init(Enna_Module *em)
{

    mod = calloc(1, sizeof(Enna_Module_Video));
    mod->em = em;
    em->mod = mod;

    enna_input_register(em, &class);

    return 1;
}


static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Video *mod;

    mod = em->mod;

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
