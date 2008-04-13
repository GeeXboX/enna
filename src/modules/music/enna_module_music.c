/* Interface */

#include "enna.h"

static int            em_init                     ();
static int            em_shutdown                 ();

EAPI Enna_Module_Api module_api =
  {
    ENNA_MODULE_VERSION,
    "music"
  };



/* Module interface */

static int
em_init()
{
   printf("Module Music Init\n");

   return 1;
}


static int
em_shutdown()
{
   printf("Module Music Shutdown\n");
   return 1;
}

EAPI void
module_init(Enna_Module *module)
{
   if (!module)
      return;

   if (!em_init())
      return;
}

EAPI void
module_shutdown(Enna_Module *module)
{
   em_shutdown();
}
