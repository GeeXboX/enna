#ifndef _ENNA_ACTIVITY_H_
#define _ENNA_ACTIVITY_H_

typedef enum _ENNA_CLASS_TYPE ENNA_CLASS_TYPE;
typedef struct _Enna_Class_Activity Enna_Class_Activity;

enum ENNA_CLASS_TYPE
{
    ENNA_CLASS_ACTIVITY
};

struct _Enna_Class_Activity
{
  const char *name;
  int pri;
  const char *label;
  const char *icon_file;
  const char *icon;
  struct
  {
    void (*class_init)(int dummy);
    void (*class_shutdown)(int dummy);
    void (*class_show)(int dummy);
    void (*class_hide)(int dummy);
  }func;
};

EAPI int          enna_activity_add(Enna_Class_Activity *class);
EAPI Evas_List   *enna_activities_get(void);

#endif
