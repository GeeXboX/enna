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
#include "content.h"
#include "mainmenu.h"
#include "module.h"
#include "buffer.h"
#include "event_key.h"

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

typedef struct _Enna_Module_Infos {
    Evas *e;
    Evas_Object *edje;
    Enna_Module *em;
    char *lsb_distrib_id;
    char *lsb_release;
} Enna_Module_Infos;

static Enna_Module_Infos *mod;

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
get_distribution (buffer_t *b)
{
    FILE *f;
    char buffer[BUF_LEN];
    char *id = NULL, *release = NULL;


    if (!mod->lsb_distrib_id || !mod->lsb_release)
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
    if (mod->lsb_distrib_id && mod->lsb_release)
        buffer_appendf (b, "%s %s", mod->lsb_distrib_id, mod->lsb_release);
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
set_system_information (buffer_t *b)
{
    if (!b)
        return;

    buffer_append (b, _("<c>System Information</c><br><br>"));
    get_distribution (b);
    get_uname (b);
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
        if (mod->lsb_distrib_id)
            free(mod->lsb_distrib_id);
        mod->lsb_distrib_id=strdup(id);
    }
    if (release)
    {
        if (mod->lsb_release)
            free(mod->lsb_release);
        mod->lsb_release=strdup(release);
    }
    return 0;
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
create_gui (void)
{
    mod->edje = edje_object_add (mod->em->evas);
    edje_object_file_set (mod->edje,
                          enna_config_theme_get (), "module/infos");
}

static void
_class_init (int dummy)
{
    Ecore_Exe *lsb_release_exe;
    Ecore_Event_Handler *lsb_release_data_handler;

    mod->lsb_distrib_id = mod->lsb_release = NULL;
    lsb_release_exe=ecore_exe_pipe_run("lsb_release -i -r", ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED, NULL);
    if (lsb_release_exe)
        lsb_release_data_handler=ecore_event_handler_add(ECORE_EXE_EVENT_DATA, lsb_release_event_data, NULL);
    create_gui ();
    enna_content_append (ENNA_MODULE_NAME, mod->edje);
}

static void
_class_shutdown (int dummy)
{
    ENNA_OBJECT_DEL (mod->edje);
    if (mod->lsb_distrib_id)
        free(mod->lsb_distrib_id);
    if (mod->lsb_release)
        free(mod->lsb_release);
}

static void
_class_show (int dummy)
{
    buffer_t *b;

    b = buffer_new();
    set_enna_information (b);
    set_system_information (b);
    edje_object_part_text_set (mod->edje, "infos.text", b->buf);
    edje_object_signal_emit (mod->edje, "infos,show", "enna");
}

static void
_class_hide (int dummy)
{
    edje_object_signal_emit (mod->edje, "infos,hide", "enna");
}

static void
_class_event (void *event_info)
{
    Evas_Event_Key_Down *ev = event_info;
    enna_key_t key = enna_get_key (ev);

    if (key != ENNA_KEY_CANCEL)
        return;

    enna_content_hide ();
    enna_mainmenu_show (enna->o_mainmenu);
}

static Enna_Class_Activity class = {
    ENNA_MODULE_NAME,
    10,
    N_("Infos"),
    NULL,
    "icon/infos",
    {
        _class_init,
        NULL,
        _class_shutdown,
        _class_show,
        _class_hide,
        _class_event
    },
    NULL
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_ACTIVITY,
    "activity_infos"
};

void
module_init (Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc (1, sizeof (Enna_Module_Infos));
    mod->em = em;
    em->mod = mod;

    enna_activity_add (&class);
}

void
module_shutdown (Enna_Module *em)
{
    evas_object_del (mod->edje);
    free (mod);
}
