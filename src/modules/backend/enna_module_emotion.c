/* Interface */

#include "enna.h"

static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

typedef struct _Enna_Module_Emotion Enna_Module_Emotion;

struct _Enna_Module_Emotion
{
   Evas *e;
   Evas_Object *o_emotion;
};

static Enna_Module_Emotion *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "emotion"
};

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

    return 1;
}


static int
em_shutdown(Enna_Module *em)
{

    Enna_Module_Emotion *mod;

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
