#ifndef _ENNA_MODULE_H_
#define _ENNA_MODULE_H_

#include "enna.h"

#define ENNA_MODULE_VERSION 1

typedef struct _Enna_Module Enna_Module;
typedef struct _Enna_Module_Api Enna_Module_Api;
typedef enum _ENNA_MODULE_CLASS_TYPE ENNA_MODULE_CLASS_TYPE;
typedef struct _Enna_Module_Class Enna_Module_Class;

enum
{
    ENNA_MODULE_CLASS_ACTIVITY
};

struct _Enna_Module_Class
{
    const char *name;
    struct
    {
        void (*class_init)(int dummy);
        void (*class_shutdown)(int dummy);
        void (*class_show)(int dummy);
        void (*class_hide)(int dummy);
    }
};

struct _Enna_Module
{
    const char *name;

    struct {
        void * (*init)        (Enna_Module *m);
        int    (*shutdown)    (Enna_Module *m);
    } func;

    Enna_Module_Api *api;
    unsigned char enabled;
    Ecore_Plugin *plugin;
    Evas *evas;
    void *mod;
    void *class;

};

struct _Enna_Module_Api
{
    int         version;
    const char *name;
};


EAPI int          enna_module_init(void);
EAPI int          enna_module_shutdown(void);
EAPI Enna_Module *enna_module_open(const char *name, Evas *evas);
EAPI int          enna_module_enable(Enna_Module *m);
EAPI int          enna_module_disable(Enna_Module *m);
EAPI int          enna_module_class_register(Enna_Module *em,  Enna_Module_Class *class);

#endif
