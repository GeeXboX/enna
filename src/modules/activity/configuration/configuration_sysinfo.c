/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2010 The Enna Project
 *
 * This file is part of Enna.
 *
 * Enna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Enna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Enna; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
#include "configuration_sysinfo.h"
#include "utils.h"

#ifdef BUILD_LIBXRANDR
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#endif

#include <player.h>

#include <valhalla.h>

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

#define ENNA_MODULE_NAME "sysinfo" // NEEDED ??

#define STR_CPU "processor"
#define STR_MODEL "model name"
#define STR_MHZ "cpu MHz"

#define STR_MEM_TOTAL "MemTotal:"
#define STR_MEM_ACTIVE "Active:"


/* local globals */
static Evas_Object *o_edje = NULL;
static Ecore_Timer *update_timer = NULL;


/****************************************************************************/
/*                       Enna Information API                               */
/****************************************************************************/

static void
set_enna_information(buffer_t *b)
{
    unsigned int ver;

    if (!b)
        return;

    buffer_append(b, "<c>");
    buffer_append(b, _("Enna information"));
    buffer_append(b, "</c><br><br><hilight>");
    buffer_append(b, _("Enna:"));
    ver = libplayer_version();
    buffer_appendf(b, "</hilight> %s<br><hilight>libplayer:</hilight> %u.%u.%u<br>",
                    VERSION, ver >> 16, ver >> 8 & 0xFF, ver & 0xFF);
    ver = libvalhalla_version();
    buffer_appendf(b, "<hilight>libvalhalla:</hilight> %u.%u.%u<br>",
                    ver >> 16, ver >> 8 & 0xFF, ver & 0xFF);
#ifdef BUILD_LIBSVDRP
    buffer_appendf(b, "<hilight>libsvdrp:</hilight> %s<br>",
                    LIBSVDRP_VERSION);
#endif
    buffer_append(b, "<hilight>");
    buffer_append(b, _("Video renderer:"));
    buffer_appendf(b, "</hilight> %s<br>", enna_config->engine);
}

/****************************************************************************/
/*                      System Information API                              */
/****************************************************************************/

static void
get_distribution(buffer_t *b)
{
    FILE *f;
    char buffer[BUF_LEN];
    char *id = NULL, *release = NULL;
    static const char *lsb_distrib_id = NULL;
    static const char *lsb_release = NULL;

    if (!lsb_distrib_id || !lsb_release)
    /* FIXME: i'm pretty sure that there's no need to try to read files
     * 'cause the command lsb_release seems to be available anywhere
     * if so, this can be stripped from here (Billy)
     */
    {
        f = fopen(LSB_FILE, "r");
        if (f)
        {
            while (fgets(buffer, BUF_LEN, f))
            {
                if (!strncmp(buffer, DISTRIB_ID, DISTRIB_ID_LEN))
                {
                    id = strdup(buffer + DISTRIB_ID_LEN);
                    id[strlen(id) - 1] = '\0';
                }
                if (!strncmp(buffer, DISTRIB_RELEASE, DISTRIB_RELEASE_LEN))
                {
                    release = strdup(buffer + DISTRIB_RELEASE_LEN);
                    release[strlen(release) - 1] = '\0';
                }
                memset(buffer, '\0', BUF_LEN);
            }
            fclose(f);
        }
        if (!id || !release)
        {
            f = fopen(DEBIAN_VERSION_FILE, "r");
            if (f)
            {
                id = strdup("Debian");
                id[strlen(id)] = '\0';
                while (fgets(buffer, BUF_LEN, f))
                {
                    release = strdup(buffer);
                    release[strlen(release) - 1] = '\0';
                    memset(buffer, '\0', BUF_LEN);
                }
                fclose(f);
            }
        }
    }

    buffer_append(b, "<hilight>");
    buffer_append(b, _("Distribution:"));
    buffer_append(b, "</hilight> ");
    if (lsb_distrib_id && lsb_release)
        buffer_appendf(b, "%s %s", lsb_distrib_id, lsb_release);
    else if (id && release)
        buffer_appendf(b, "%s %s", id, release);
    else
        buffer_append(b, BUF_DEFAULT);
    buffer_append(b, "<br>");

    if (id)
        free(id);
    if (release)
        free(release);
}

static void
get_uname(buffer_t *b)
{
    struct utsname name;

    buffer_append(b, "<hilight>");
    buffer_append(b, _("OS:"));
    buffer_append(b, "</hilight> ");
    if (uname(&name) == -1) {
        buffer_append(b, BUF_DEFAULT);
    }
    else
    {
        buffer_appendf(b, "%s %s ", name.sysname, name.release);
        buffer_append(b, _("for"));
        buffer_appendf(b, " %s", name.machine);
    }
    buffer_append(b, "<br>");
}

static void
get_cpuinfos(buffer_t *b)
{
    FILE *f;
    char buf[256] = { 0 };

    f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return;

    buffer_append(b, "<hilight>");
    buffer_append(b, _("Available CPUs:"));
    buffer_append(b, "</hilight><br>");
    while (fgets(buf, sizeof(buf), f))
    {
        char *x;
        buf[strlen(buf) - 1] = '\0';
        if (!strncmp(buf, STR_CPU, strlen(STR_CPU)))
        {
            x = strchr(buf, ':');
            buffer_appendf(b, " * CPU #%s: ", x + 2);
        }
        else if (!strncmp(buf, STR_MODEL, strlen(STR_MODEL)))
        {
            char *y;
            x = strchr(buf, ':');
            y = x + 2;
            while (*y)
            {
                if (*y != ' ')
                    buffer_appendf(b, "%c", *y);
                else
                {
                    if (*(y + 1) != ' ')
                    buffer_appendf(b, " ");
                }
                (void) *y++;
            }
        }
        else if (!strncmp(buf, STR_MHZ, strlen(STR_MHZ)))
        {
            x = strchr(buf, ':');
            buffer_append(b, _(", running at"));
            buffer_appendf(b, " %d MHz<br>", (int) enna_util_atof(x + 2));
        }
    }

    fclose(f);
}

static void
get_loadavg(buffer_t *b)
{
    FILE *f;
    char buf[256] = { 0 };
    char *ld, *x;
    float load;

    f = fopen("/proc/loadavg", "r");
    if (!f)
        return;

    x = fgets(buf, sizeof(buf), f);
    x = strchr(buf, ' ');
    if (!x)
        goto err_loadavg;

    ld = strndup(buf, sizeof (x));
    load = enna_util_atof(ld) * 100.0;

    buffer_append(b, "<hilight>");
    buffer_append(b, _("CPU Load:"));
    buffer_appendf(b, "</hilight> %d%%<br>", (int) load);

 err_loadavg:
    if (ld)
        free(ld);
    if (f)
        fclose(f);
}

static void
get_ram_usage(buffer_t *b)
{
    FILE *f;
    char buf[256] = { 0 };
    int mem_total = 0, mem_active = 0;

    f = fopen("/proc/meminfo", "r");
    if (!f)
        return;

    while (fgets(buf, sizeof(buf), f))
    {
        if (!strncmp(buf, STR_MEM_TOTAL, strlen(STR_MEM_TOTAL)))
        {
            char *x;
            /* remove the trailing ' kB' from buffer */
            buf[strlen(buf) - 4] = '\0';
            x = strrchr(buf, ' ');
            if (x)
                mem_total = atoi(x + 1) / 1024;
        }
        else if (!strncmp(buf, STR_MEM_ACTIVE, strlen(STR_MEM_ACTIVE)))
        {
            char *x;
            /* remove the trailing ' kB' from buffer */
            buf[strlen(buf) - 4] = '\0';
            x = strrchr(buf, ' ');
            if (x)
                mem_active = atoi(x + 1) / 1024;
        }
    }

    buffer_append(b, "<hilight>");
    buffer_append(b, _("Memory:"));
    buffer_appendf(b, "</hilight> %d MB ", mem_active);
    buffer_append(b, _("used on"));
    buffer_appendf(b, " %d MB ", mem_total);
    buffer_append(b, _("total"));
    buffer_appendf(b, " (%d%%)</hilight><br>",
                   (int) (mem_active * 100 / mem_total));
    fclose(f);
}

#ifdef BUILD_LIBXRANDR
static void
get_resolution(buffer_t *b)
{
    XRRScreenConfiguration *sc;
    Window root;
    Display *dpy;
    short rate;
    int screen;
    int minWidth, maxWidth, minHeight, maxHeight;

    dpy = XOpenDisplay(":0.0");
    if (!dpy)
        return;

    screen = DefaultScreen(dpy);
    if (screen >= ScreenCount(dpy))
        return;

    root = RootWindow(dpy, screen);
    if (root < 0)
        return;

    sc = XRRGetScreenInfo(dpy, root);
    if (!sc)
        return;

    rate = XRRConfigCurrentRate(sc);
    XRRGetScreenSizeRange(dpy, root,
                          &minWidth, &minHeight, &maxWidth, &maxHeight);

    buffer_append(b, "<hilight>");
    buffer_append(b, _("Screen resolution:"));
    buffer_appendf(b, "</hilight> %dx%d ",
                   DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
    buffer_append(b, _("at"));
    buffer_appendf(b, " %d Hz (", rate);
    buffer_append(b, _("min:"));
    buffer_appendf(b, " %dx%d, ", minWidth, minHeight);
    buffer_append(b, _("max:"));
    buffer_appendf(b, " %dx%d)<br>", maxWidth, maxHeight);
    XCloseDisplay(dpy);
}
#endif

#ifdef BUILD_LIBSVDRP
static void
get_vdr(buffer_t *b)
{
    svdrp_t *svdrp = enna_svdrp_get();

    buffer_append(b, "<hilight> ");
    buffer_append(b, _("VDR:"));
    buffer_append(b, "</hilight> ");
    if (svdrp && svdrp_try_connect(svdrp)) {
        buffer_appendf(b, _("connected to VDR"));
        buffer_appendf(b, " %s )",
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_VERSION));
	buffer_append(b, _("on"));
        buffer_appendf(b, " %s on %s (%s)",
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_NAME),
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_HOSTNAME));
    } else {
        buffer_append(b, _("not connected"));
    }
    buffer_append(b, "<br>");
}
#endif

static void
get_network(buffer_t *b)
{
    int s, n, i;
    struct ifreq *ifr;
    struct ifconf ifc;
    char buf[1024];

    buffer_append(b, "<hilight>");
    buffer_append(b, _("Available network interfaces:"));
    buffer_append(b, "</hilight><br>");

    /* get a socket handle. */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return;

    /* query available interfaces. */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
        goto err_net;

    /* iterate through the list of interfaces. */
    ifr = ifc.ifc_req;
    n = ifc.ifc_len / sizeof(struct ifreq);
    for (i = 0; i < n; i++)
    {
        struct ifreq *item = &ifr[i];

        /* discard loopback interface */
        if (!strcmp(item->ifr_name, "lo"))
            continue;

        /* show the device name and IP address */
        buffer_appendf(b, "  * %s (", item->ifr_name);
        buffer_append(b, _("IP:"));
	buffer_appendf(b, " %s, ",
                 inet_ntoa(((struct sockaddr_in *) &item->ifr_addr)->sin_addr));

        if (ioctl(s, SIOCGIFNETMASK, item) < 0)
            continue;

        buffer_append(b, _("Netmask:"));
        buffer_appendf(b, " %s)<br>",
              inet_ntoa(((struct sockaddr_in *) &item->ifr_netmask)->sin_addr));
    }

 err_net:
    close(s);
}

static void
get_default_gw(buffer_t *b)
{
    char devname[64];
    unsigned long d, g, m;
    int res, flgs, ref, use, metric, mtu, win, ir;
    FILE *fp;

    fp = fopen("/proc/net/route", "r");
    if (!fp)
        return;

    if (fscanf(fp, "%*[^\n]\n") < 0) /* Skip the first line. */
        return;

    buffer_append(b, "<hilight>");
    buffer_append(b, _("Default gateway:"));
    buffer_append(b, "</hilight> ");
    res = 0;

    while (1)
    {
        struct in_addr gw;
        int r;

        r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                   &mtu, &win, &ir);

        if (r != 11)
            if ((r < 0) && feof(fp))
                break;

        /* we only care about default gateway */
        if (d != 0)
            continue;

        gw.s_addr = g;
        buffer_appendf(b, "%s<br>", inet_ntoa (gw));
        res = 1;
        break;
    }

    if (!res) {
        buffer_append(b, _("None"));
        buffer_append(b, "<br>");
    }


    fclose(fp);
}

static void
set_system_information(buffer_t *b)
{
    if (!b)
        return;

    buffer_append(b, "<c>");
    buffer_append(b, _("System information"));
    buffer_append(b, "</c><br><br>");
    get_distribution(b);
    get_uname(b);
    get_cpuinfos(b);
    get_loadavg(b);
    get_ram_usage(b);
#ifdef BUILD_LIBSVDRP
    get_vdr(b);
#endif
#ifdef BUILD_LIBXRANDR
    get_resolution(b);
#endif
    get_network(b);
    get_default_gw(b);
}

/* ecore timer callback */
static int
_update_infos_cb(void *data)
{
    buffer_t *b;
    Evas_Object *obj;
    obj = data;
    b = buffer_new();
    set_enna_information(b);
    set_system_information(b);
    edje_object_part_text_set(obj, "sysinfo.text", b->buf);
    edje_object_signal_emit(obj, "sysinfo,show", "enna");
    buffer_free(b);

    return ECORE_CALLBACK_RENEW;
}

/* externally accessible functions */
Evas_Object *
info_panel_show(void *data)
{
    // create the panel main object
    o_edje = edje_object_add(enna->evas);
    edje_object_file_set(o_edje, enna_config_theme_get (),
                         "activity/configuration/sysinfo");

    // update info once and fire the first callback
    _update_infos_cb(o_edje);
    update_timer =
        ecore_timer_add(INFOS_REFRESH_PERIOD, _update_infos_cb, o_edje);

    return o_edje;
}

void
info_panel_hide(void *data)
{
    ENNA_TIMER_DEL(update_timer);
    ENNA_OBJECT_DEL(o_edje);
}
