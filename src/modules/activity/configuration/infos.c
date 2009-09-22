/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "enna.h"
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "buffer.h"


#ifdef BUILD_LIBSVDRP
#include "utils.h"
#endif

#ifdef BUILD_LIBXRANDR
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#endif

#include <player.h>

#ifdef BUILD_BROWSER_VALHALLA
#include <valhalla.h>
#endif

/* Refresh period : 2s */
#define INFOS_REFRESH_PERIOD 2.0

#define BUF_LEN 1024
#define BUF_DEFAULT "Unknown"
#define LSB_FILE "/etc/lsb-release"
#define DEBIAN_VERSION_FILE "/etc/debian_version"
#define DISTRIB_ID "DISTRIB_ID="
#define E_DISTRIB_ID "Distributor ID:"
#define E_RELEASE "Release:"
#define DISTRIB_ID_LEN strlen (DISTRIB_ID)
#define DISTRIB_RELEASE "DISTRIB_RELEASE="
#define DISTRIB_RELEASE_LEN strlen (DISTRIB_RELEASE)

#define ENNA_MODULE_NAME "infos"

#define STR_CPU "processor"
#define STR_MODEL "model name"
#define STR_MHZ "cpu MHz"

#define STR_MEM_TOTAL "MemTotal:"
#define STR_MEM_ACTIVE "Active:"

#define SMART_NAME "enna_INFOS"

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{
    Evas_Coord x, y, w, h;
    Evas_Object *obj;
    Evas_Object *o_edje;
    Ecore_Timer *update_timer;
    char *lsb_distrib_id;
    char *lsb_release;
    Ecore_Event_Handler *lsb_release_data_handler;
    Ecore_Exe *lsb_release_exe;
};

/* local subsystem functions */
static void _smart_reconfigure(Smart_Data * sd);
static void _smart_init(void);

/* local subsystem globals */
static Evas_Smart *_smart = NULL;


/****************************************************************************/
/*                       Enna Information API                               */
/****************************************************************************/

static void
set_enna_information (buffer_t *b)
{
    if (!b)
        return;

    buffer_append (b, _("<c>Enna Information</c><br><br>"));
    buffer_appendf (b, _("<hilight>Enna: </hilight>%s<br>"), VERSION);
    buffer_appendf (b, _("<hilight>libplayer: </hilight>%s<br>"),
                    LIBPLAYER_VERSION);
#ifdef BUILD_BROWSER_VALHALLA
    buffer_appendf (b, _("<hilight>libvalhalla: </hilight>%s<br>"),
                    LIBVALHALLA_VERSION_STR);
#endif
#ifdef BUILD_LIBSVDRP
    buffer_appendf (b, _("<hilight>libsvdrp: </hilight>%s<br>"),
                    LIBSVDRP_VERSION);
#endif
    buffer_append (b, "<br>");
}

/****************************************************************************/
/*                      System Information API                              */
/****************************************************************************/

static void
get_distribution (Smart_Data *sd, buffer_t *b)
{
    FILE *f;
    char buffer[BUF_LEN];
    char *id = NULL, *release = NULL;


    if (!sd->lsb_distrib_id || !sd->lsb_release)
    //FIXME: i'm pretty sure that there's no need to try to read files 'cause the command lsb_release seems to be available anywhere
    //if so, this can be stripped from here (Billy)
    {
        f = fopen (LSB_FILE, "r");
        if (f)
        {
            while (fgets (buffer, BUF_LEN, f))
            {
                if (!strncmp (buffer, DISTRIB_ID, DISTRIB_ID_LEN))
                {
                    id = strdup (buffer + DISTRIB_ID_LEN);
                    id[strlen (id) - 1] = '\0';
                }
                if (!strncmp (buffer, DISTRIB_RELEASE, DISTRIB_RELEASE_LEN))
                {
                    release = strdup (buffer + DISTRIB_RELEASE_LEN);
                    release[strlen (release) - 1] = '\0';
                }
                memset (buffer, '\0', BUF_LEN);
            }
            fclose (f);
        }
        if(!id || !release) {
            f = fopen (DEBIAN_VERSION_FILE, "r");
            if (f)
            {
                id = strdup ("Debian");
                id[strlen (id)] = '\0';
                while (fgets (buffer, BUF_LEN, f))
                {
                    release = strdup (buffer);
                    release[strlen (release) - 1] = '\0';
                    memset (buffer, '\0', BUF_LEN);
                }
                fclose (f);
            }
        }
    }

    buffer_append (b, _("<hilight>Distribution: </hilight>"));
    if (sd->lsb_distrib_id && sd->lsb_release)
        buffer_appendf (b, "%s %s", sd->lsb_distrib_id, sd->lsb_release);
    else if (id && release)
        buffer_appendf (b, "%s %s", id, release);
    else
        buffer_append (b, BUF_DEFAULT);
    buffer_append (b, "<br>");

    if (id)
        free (id);
    if (release)
        free (release);
}

static void
get_uname (buffer_t *b)
{
    struct utsname name;

    buffer_append (b, _("<hilight>OS: </hilight>"));
    if (uname (&name) == -1)
        buffer_append (b, BUF_DEFAULT);
    else
        buffer_appendf (b, "%s %s for %s",
                        name.sysname, name.release, name.machine);
    buffer_append (b, "<br>");
}

static void
get_cpuinfos (buffer_t *b)
{
  FILE *f;
  char buf[256] = { 0 };

  f = fopen ("/proc/cpuinfo", "r");
  if (!f)
    return;

  buffer_append (b, _("<hilight>Available CPUs: </hilight>"));
  buffer_append (b, "<br>");
  while (fgets (buf, sizeof (buf), f))
  {
    char *x;
    buf[strlen (buf) - 1] = '\0';
    if (!strncmp (buf, STR_CPU, strlen (STR_CPU)))
    {
      x = strchr (buf, ':');
      buffer_appendf (b, " * CPU #%s: ", x + 2);
    }
    else if (!strncmp (buf, STR_MODEL, strlen (STR_MODEL)))
    {
      char *y;
      x = strchr (buf, ':');
      y = x + 2;
      while (*y)
      {
	if (*y != ' ')
	  buffer_appendf (b, "%c", *y);
	else
	{
	  if (*(y + 1) != ' ')
	    buffer_appendf (b, " ");
	}
	(void) *y++;
      }
    }
    else if (!strncmp (buf, STR_MHZ, strlen (STR_MHZ)))
    {
      x = strchr (buf, ':');
      buffer_appendf (b, _(", running at %d MHz"), (int) atof (x + 2));
      buffer_append (b, "<br>");
    }
  }

  fclose (f);
}

static void
get_loadavg (buffer_t *b)
{
  FILE *f;
  char buf[256] = { 0 };
  char *ld, *x;
  float load;
  char *prev_locale;

  f = fopen ("/proc/loadavg", "r");
  if (!f)
    return;

  x = fgets (buf, sizeof (buf), f);
  x = strchr (buf, ' ');
  if (!x)
    goto err_loadavg;

  ld = strndup (buf, sizeof (x));
  prev_locale = setlocale (LC_NUMERIC, NULL);
  setlocale (LC_NUMERIC, "C");
  load = strtod (ld, NULL) * 100;
  setlocale (LC_NUMERIC, prev_locale);

  buffer_append  (b, _("<hilight>CPU Load: </hilight>"));
  buffer_appendf (b, "%d%%<br>", (int) load);

 err_loadavg:
  if (ld)
    free (ld);
  if (f)
    fclose (f);
}

static void
get_ram_usage (buffer_t *b)
{
  FILE *f;
  char buf[256] = { 0 };
  int mem_total = 0, mem_active = 0;

  f = fopen ("/proc/meminfo", "r");
  if (!f)
    return;

  while (fgets (buf, sizeof (buf), f))
  {
    if (!strncmp (buf, STR_MEM_TOTAL, strlen (STR_MEM_TOTAL)))
    {
      char *x;
      /* remove the trailing ' kB' from buffer */
      buf[strlen (buf) - 4] = '\0';
      x = strrchr (buf, ' ');
      if (x)
	mem_total = atoi (x + 1) / 1024;
    }
    else if (!strncmp (buf, STR_MEM_ACTIVE, strlen (STR_MEM_ACTIVE)))
    {
      char *x;
      /* remove the trailing ' kB' from buffer */
      buf[strlen (buf) - 4] = '\0';
      x = strrchr (buf, ' ');
      if (x)
	mem_active = atoi (x + 1) / 1024;
    }
  }

  buffer_append (b, _("<hilight>Memory: </hilight>"));
  buffer_appendf (b, _("%d MB used on %d MB total (%d%%)"),
		  mem_active, mem_total,
		  (int) (mem_active * 100 / mem_total));
  buffer_append (b, "<br>");
  fclose (f);
}

#ifdef BUILD_LIBXRANDR
static void
get_resolution (buffer_t *b)
{
    XRRScreenConfiguration *sc;
    Window root;
    Display *dpy;
    short rate;
    int screen;
    int minWidth, maxWidth, minHeight, maxHeight;

    dpy = XOpenDisplay (":0.0");
    if (!dpy)
        return;

    screen = DefaultScreen (dpy);
    if (screen >= ScreenCount (dpy))
        return;

    root = RootWindow (dpy, screen);

    sc = XRRGetScreenInfo (dpy, root);
    if (!sc)
        return;

    rate = XRRConfigCurrentRate (sc);
    XRRGetScreenSizeRange (dpy, root,
                           &minWidth, &minHeight, &maxWidth, &maxHeight);

    buffer_append (b, _("<hilight>Screen Resolution: </hilight>"));
    buffer_appendf (b, _("%dx%d at %d Hz (min: %dx%d, max: %dx%d)"),
                    DisplayWidth (dpy, screen), DisplayHeight (dpy, screen),
                    rate, minWidth, minHeight, maxWidth, maxHeight);
    buffer_append (b, "<br>");
}
#endif

#ifdef BUILD_LIBSVDRP
static void
get_vdr (buffer_t *b)
{
    svdrp_t *svdrp = enna_svdrp_get();

    buffer_append (b, _("<hilight>VDR:</hilight> "));
    if (svdrp && svdrp_try_connect(svdrp))
        buffer_appendf(b, _("connected to VDR %s on %s (%s)<br>"),
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_VERSION),
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_NAME),
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_HOSTNAME));
    else
        buffer_append (b, _("not connected<br>"));
}
#endif

static void
get_network (buffer_t *b)
{
    int s, n, i;
    struct ifreq *ifr;
    struct ifconf ifc;
    char buf[1024];

    buffer_append (b, _("<hilight>Available network interfaces:</hilight><br>"));

    /* get a socket handle. */
    s = socket (AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return;

    /* query available interfaces. */
    ifc.ifc_len = sizeof (buf);
    ifc.ifc_buf = buf;
    if (ioctl (s, SIOCGIFCONF, &ifc) < 0)
        goto err_net;

    /* iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    n = ifc.ifc_len / sizeof (struct ifreq);
    for (i = 0; i < n; i++)
    {
        struct ifreq *item = &ifr[i];

        /* discard loopback interface */
        if (!strcmp (item->ifr_name, "lo"))
          continue;

        /* show the device name and IP address */
        buffer_appendf (b, _("  * %s (IP: %s, "), item->ifr_name,
                inet_ntoa (((struct sockaddr_in *)&item->ifr_addr)->sin_addr));

        if (ioctl (s, SIOCGIFNETMASK, item) < 0)
            continue;

        buffer_appendf (b, _("Netmask: %s)<br>"),
              inet_ntoa (((struct sockaddr_in *)&item->ifr_netmask)->sin_addr));
    }

 err_net:
    close (s);
}

static void
get_default_gw (buffer_t *b)
{
    char devname[64];
    unsigned long d, g, m;
    int res, flgs, ref, use, metric, mtu, win, ir;
    FILE *fp;

    fp = fopen ("/proc/net/route", "r");
    if (!fp)
        return;

    if (fscanf (fp, "%*[^\n]\n") < 0) /* Skip the first line. */
        return;

    buffer_append (b, _("<hilight>Default Gateway: </hilight>"));
    res = 0;

    while (1)
    {
        struct in_addr gw;
        int r;

        r = fscanf (fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                    devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                    &mtu, &win, &ir);

        if (r != 11)
            if ((r < 0) && feof (fp))
                break;

        /* we only care about default gateway */
        if (d != 0)
            continue;

        gw.s_addr = g;
        buffer_appendf (b, "%s<br>", inet_ntoa (gw));
        res = 1;
        break;
    }

    if (!res)
       buffer_append (b, _("None<br>"));

    fclose (fp);
}

static void
set_system_information (Smart_Data *sd, buffer_t *b)
{
    if (!b)
        return;

    buffer_append (b, _("<c>System Information</c><br><br>"));
    get_distribution (sd, b);
    get_uname (b);
    get_cpuinfos (b);
    get_loadavg (b);
    get_ram_usage (b);
#ifdef BUILD_LIBSVDRP
    get_vdr (b);
#endif
#ifdef BUILD_LIBXRANDR
    get_resolution (b);
#endif
    get_network (b);
    get_default_gw (b);
}

/****************************************************************************/
/*                        Event Callbacks                                   */
/****************************************************************************/
static int
lsb_release_event_data(void *data, int type, void *event)
{
    Smart_Data *sd = data;
    Ecore_Exe_Event_Data *ev = event;
    char *id, *release;
    id = release = NULL;

    if ((ev->lines) && (ev->lines[0].line))
    {
        int i;
        for (i = 0; ev->lines[i].line; i++)
        {
            char *line= ev->lines[i].line;
            if (!strncmp(line, E_DISTRIB_ID, strlen(E_DISTRIB_ID)))
                id=line+strlen(E_DISTRIB_ID);
            if (!strncmp(line, E_RELEASE, strlen(E_RELEASE)))
                release=line+strlen(E_RELEASE);
        }
    }
    while (id && (id[0]==' ' || id[0]=='\t')) id++;
    while (release && (release[0]==' ' || release[0]=='\t')) release++;

    if (id)
    {
        if (sd->lsb_distrib_id)
            free(sd->lsb_distrib_id);
        sd->lsb_distrib_id=strdup(id);
    }
    if (release)
    {
        if (sd->lsb_release)
            free(sd->lsb_release);
        sd->lsb_release=strdup(release);
    }
    return 0;
}

static int
_update_infos_cb(void *data)
{
    buffer_t *b;
    Smart_Data *sd = data;

    b = buffer_new();
    set_enna_information (b);
    set_system_information (sd, b);
    edje_object_part_text_set (sd->o_edje, "infos.text", b->buf);
    edje_object_signal_emit (sd->o_edje, "infos,show", "enna");
    buffer_free(b);

    return 1;
}

/* local subsystem globals */
static void _smart_reconfigure(Smart_Data * sd)
{
    Evas_Coord x, y, w, h;

    x = sd->x;
    y = sd->y;
    w = sd->w;
    h = sd->h;

    evas_object_move(sd->o_edje, x, y);
    evas_object_resize(sd->o_edje, w, h);

}

static void _smart_add(Evas_Object * obj)
{
    Smart_Data *sd;

    sd = calloc(1, sizeof(Smart_Data));
    if (!sd)
        return;
    sd->o_edje = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set (sd->o_edje,
	enna_config_theme_get (), "module/infos");
    sd->lsb_distrib_id = sd->lsb_release = NULL;
    sd->lsb_release_exe=ecore_exe_pipe_run("lsb_release -i -r", ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED, NULL);
    if (sd->lsb_release_exe)
        sd->lsb_release_data_handler=ecore_event_handler_add(ECORE_EXE_EVENT_DATA, lsb_release_event_data, sd);
    _update_infos_cb(sd);
    sd->update_timer = ecore_timer_add(INFOS_REFRESH_PERIOD, _update_infos_cb, sd);

    sd->obj = obj;
    evas_object_smart_member_add(sd->o_edje, obj);
    evas_object_smart_data_set(obj, sd);
}

static void _smart_del(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_del (sd->o_edje);
    ENNA_TIMER_DEL (sd->update_timer);
    ecore_exe_free(sd->lsb_release_exe);
    ecore_event_handler_del (sd->lsb_release_data_handler);
    if (sd->lsb_distrib_id)
        free (sd->lsb_distrib_id);
    if (sd->lsb_release)
        free (sd->lsb_release);
    free (sd);
}

static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y)
{
    INTERNAL_ENTRY;

    if ((sd->x == x) && (sd->y == y))
        return;
    sd->x = x;
    sd->y = y;
    _smart_reconfigure(sd);
}

static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h)
{
    INTERNAL_ENTRY;

    if ((sd->w == w) && (sd->h == h))
        return;
    sd->w = w;
    sd->h = h;
    _smart_reconfigure(sd);
}

static void _smart_show(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_show(sd->o_edje);
}

static void _smart_hide(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_hide(sd->o_edje);
}

static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a)
{
    INTERNAL_ENTRY;
    evas_object_color_set(sd->o_edje, r, g, b, a);
}

static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip)
{
    INTERNAL_ENTRY;
    evas_object_clip_set(sd->o_edje, clip);
}

static void _smart_clip_unset(Evas_Object * obj)
{
    INTERNAL_ENTRY;
    evas_object_clip_unset(sd->o_edje);
}

static void _smart_init(void)
{
    static const Evas_Smart_Class sc =
    {
        SMART_NAME,
        EVAS_SMART_CLASS_VERSION,
        _smart_add,
        _smart_del,
        _smart_move,
        _smart_resize,
        _smart_show,
        _smart_hide,
        _smart_color_set,
        _smart_clip_set,
        _smart_clip_unset,
        NULL,
        NULL
    };

    if (!_smart)
       _smart = evas_smart_class_new(&sc);
}

static Evas_Object *
enna_infos_add(Evas * evas)
{
    _smart_init();
    return evas_object_smart_add(evas, _smart);
}

/* externally accessible functions */

Evas_Object *info_panel_show(void *data)
{
    printf("SHOW CB\n");

    return enna_infos_add (enna->evas);
}

void info_panel_hide(void *data)
{
    printf("HIDE CB\n");

}
