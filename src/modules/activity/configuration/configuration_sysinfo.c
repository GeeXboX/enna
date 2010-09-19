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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/route.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

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

#include <ifaddrs.h>

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

#define ENNA_MODULE_NAME "sysinfo"

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
set_enna_information(Enna_Buffer *b)
{
    unsigned int ver;

    if (!b)
        return;

    enna_buffer_append(b, "<c>");
    enna_buffer_append(b, _("Enna information"));
    enna_buffer_append(b, "</c><br><br><hilight>");
    enna_buffer_append(b, _("Enna:"));
#ifndef BUILD_BACKEND_EMOTION
    ver = libplayer_version();
    enna_buffer_appendf(b, "</hilight> %s<br>"
                       "<hilight>libplayer:</hilight> %u.%u.%u<br>",
                       VERSION, ver >> 16, ver >> 8 & 0xFF, ver & 0xFF);
#endif /* BUILD_BACKEND_EMOTION */
    ver = libvalhalla_version();
    enna_buffer_appendf(b, "<hilight>libvalhalla:</hilight> %u.%u.%u<br>",
                   ver >> 16, ver >> 8 & 0xFF, ver & 0xFF);
#ifdef BUILD_LIBSVDRP
    enna_buffer_appendf(b, "<hilight>libsvdrp:</hilight> %s<br>",
                        LIBSVDRP_VERSION);
#endif /* BUILD_LIBSVDRP */
    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Video renderer:"));
    enna_buffer_appendf(b, "</hilight> %s<br>", enna_config->engine);
}

/****************************************************************************/
/*                      System Information API                              */
/****************************************************************************/

static void
get_distribution(Enna_Buffer *b)
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

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Distribution:"));
    enna_buffer_append(b, "</hilight> ");
    if (lsb_distrib_id && lsb_release)
        enna_buffer_appendf(b, "%s %s", lsb_distrib_id, lsb_release);
    else if (id && release)
        enna_buffer_appendf(b, "%s %s", id, release);
    else
#ifdef __FreeBSD__
        enna_buffer_append(b, "FreeBSD");
#else /* __FreeBSD__ */
        enna_buffer_append(b, BUF_DEFAULT);
#endif /* !__FreeBSD__ */
    enna_buffer_append(b, "<br>");

    if (id)
        free(id);
    if (release)
        free(release);
}

static void
get_uname(Enna_Buffer *b)
{
    struct utsname name;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("OS:"));
    enna_buffer_append(b, "</hilight> ");
    if (uname(&name) == -1) {
        enna_buffer_append(b, BUF_DEFAULT);
    }
    else
    {
        enna_buffer_appendf(b, "%s %s ", name.sysname, name.release);
        enna_buffer_append(b, _("for"));
        enna_buffer_appendf(b, " %s", name.machine);
    }
    enna_buffer_append(b, "<br>");
}

static void
get_cpuinfos(Enna_Buffer *b)
{
#ifdef __FreeBSD__
    int nbcpu = 0, i, freq;
    char cpu_model[256];
    size_t len;
    char buf[256];

    len = sizeof(cpu_model);
    if (sysctlbyname("hw.model", &cpu_model, &len, NULL, 0))
        return;

    len = sizeof(nbcpu);
    if (sysctlbyname("hw.ncpu", &nbcpu, &len, NULL, 0))
        return;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Available CPUs:"));
    enna_buffer_append(b, "</hilight><br>");

    for (i = 0; i < nbcpu; i++)
    {
        snprintf(buf, 256, "dev.cpu.%d.freq", i);
        len = sizeof(freq);

        if (sysctlbyname(buf, &freq, &len, NULL, 0))
            continue;

        enna_buffer_appendf(b, " * CPU #%d: %s, running at %dMHz<br>",
                            i, cpu_model, freq);
    }

#else /* __FreeBSD__ */
    FILE *f;
    char buf[256] = { 0 };

    f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Available CPUs:"));
    enna_buffer_append(b, "</hilight><br>");
    while (fgets(buf, sizeof(buf), f))
    {
        char *x;
        buf[strlen(buf) - 1] = '\0';
        if (!strncmp(buf, STR_CPU, strlen(STR_CPU)))
        {
            x = strchr(buf, ':');
            enna_buffer_appendf(b, " * CPU #%s: ", x + 2);
        }
        else if (!strncmp(buf, STR_MODEL, strlen(STR_MODEL)))
        {
            char *y;
            x = strchr(buf, ':');
            y = x + 2;
            while (*y)
            {
                if (*y != ' ')
                    enna_buffer_appendf(b, "%c", *y);
                else
                {
                    if (*(y + 1) != ' ')
                    enna_buffer_appendf(b, " ");
                }
                (void) *y++;
            }
        }
        else if (!strncmp(buf, STR_MHZ, strlen(STR_MHZ)))
        {
            x = strchr(buf, ':');
            enna_buffer_append(b, _(", running at"));
            enna_buffer_appendf(b, " %d MHz<br>", (int) enna_util_atof(x + 2));
        }
    }

    fclose(f);
#endif /* !__FreeBSD__ */
}

static void
get_loadavg(Enna_Buffer *b)
{
    float load;
#ifdef __FreeBSD__
    double loadavg[3];

    if (getloadavg(loadavg, 3) == -1)
        return;

    load = loadavg[0] * 100.0;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("CPU Load:"));
    enna_buffer_appendf(b, "</hilight> %d%%<br>", (int) load);
#else /* __FreeBSD__ */
    FILE *f;
    char buf[256] = { 0 };
    char *ld, *x;

    f = fopen("/proc/loadavg", "r");
    if (!f)
        return;

    x = fgets(buf, sizeof(buf), f);
    x = strchr(buf, ' ');
    if (!x)
        goto err_loadavg;

    ld = strndup(buf, sizeof(x));
    load = enna_util_atof(ld) * 100.0;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("CPU Load:"));
    enna_buffer_appendf(b, "</hilight> %d%%<br>", (int) load);

 err_loadavg:
    if (ld)
        free(ld);
    if (f)
        fclose(f);
#endif /* !__FreeBSD__ */
}

static void
get_ram_usage(Enna_Buffer *b)
{
#ifdef __FreeBSD__
    int mem_total = 0, mem_active = 0;
    int page_size;
    size_t len;

    len = sizeof(mem_total);
    if (sysctlbyname("hw.physmem", &mem_total, &len, NULL, 0))
        return;

    mem_total = mem_total / 1024 / 1024;

    len = sizeof(mem_active);

    if (sysctlbyname("vm.stats.vm.v_active_count", &mem_active, &len, NULL, 0))
        return;

    len = sizeof(page_size);
    if (sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0))
        return;

    mem_active = mem_active * page_size / 1024 / 1024;
#else /* __FreeBSD__ */
    int mem_total = 0, mem_active = 0;
    FILE *f;
    char buf[256] = { 0 };

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

    fclose(f);
#endif /* !__FreeBSD__ */

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Memory:"));
    enna_buffer_appendf(b, "</hilight> %d MB ", mem_active);
    enna_buffer_append(b, _("used on"));
    enna_buffer_appendf(b, " %d MB ", mem_total);
    enna_buffer_append(b, _("total"));
    enna_buffer_appendf(b, " (%d%%)</hilight><br>",
                        (int) (mem_active * 100 / mem_total));
}

#ifdef BUILD_LIBXRANDR
static void
get_resolution(Enna_Buffer *b)
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

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Screen resolution:"));
    enna_buffer_appendf(b, "</hilight> %dx%d ",
                   DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
    enna_buffer_append(b, _("at"));
    enna_buffer_appendf(b, " %d Hz (", rate);
    enna_buffer_append(b, _("min:"));
    enna_buffer_appendf(b, " %dx%d, ", minWidth, minHeight);
    enna_buffer_append(b, _("max:"));
    enna_buffer_appendf(b, " %dx%d)<br>", maxWidth, maxHeight);
    XCloseDisplay(dpy);
}
#endif /* BUILD_LIBXRANDR */

#ifdef BUILD_LIBSVDRP
static void
get_vdr(Enna_Buffer *b)
{
    svdrp_t *svdrp = enna_svdrp_get();

    enna_buffer_append(b, "<hilight> ");
    enna_buffer_append(b, _("VDR:"));
    enna_buffer_append(b, "</hilight> ");
    if (svdrp && svdrp_try_connect(svdrp)) {
        enna_buffer_appendf(b, _("connected to VDR"));
        enna_buffer_appendf(b, " %s )",
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_VERSION));
        enna_buffer_append(b, _("on"));
        enna_buffer_appendf(b, " %s on %s (%s)",
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_NAME),
                       svdrp_get_property(svdrp, SVDRP_PROPERTY_HOSTNAME));
    } else {
        enna_buffer_append(b, _("not connected"));
    }
    enna_buffer_append(b, "<br>");
}
#endif /* BUILD_LIBSVDRP */

static void
get_network(Enna_Buffer *b)
{
    struct ifaddrs *ifr, *ifa;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Available network interfaces:"));
    enna_buffer_append(b, "</hilight><br>");

    if (getifaddrs(&ifr) == -1)
        return;

    for (ifa = ifr; ifa != NULL; ifa = ifa->ifa_next)
    {
        /* discard loopback interface */
        if (!strncmp(ifa->ifa_name, "lo", 2)) /* also remove the lo* */
            continue;

        if (   ifa->ifa_addr->sa_family != AF_INET
            && ifa->ifa_addr->sa_family != AF_INET6)
            continue;

        /* show the device name and IP address */
        enna_buffer_appendf(b, "  * %s (", ifa->ifa_name);
        enna_buffer_append(b, _("IP:"));
        enna_buffer_appendf(b, " %s, ",
                            inet_ntoa(((struct sockaddr_in *)
                                       ifa->ifa_addr)->sin_addr));

        enna_buffer_append(b, _("Netmask:"));
        enna_buffer_appendf(b, " %s)<br>",
                            inet_ntoa(((struct sockaddr_in *)
                                       ifa->ifa_netmask)->sin_addr));

    }

    freeifaddrs(ifr);
}


#ifdef __FreeBSD__
struct {
    struct  rt_msghdr m_rtm;
    char    m_space[512];
} m_rtmsg;

#define ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#define NEXTADDR(w, u)          \
    if (rtm_addrs & (w))        \
    {                           \
        l = ROUNDUP(u.sa_len);  \
        memmove(cp, &(u), l);   \
        cp += l;                \
    }

#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

#define rtm m_rtmsg.m_rtm
#endif /* __FreeBSD__ */

static void
get_default_gw(Enna_Buffer *b)
{
#ifdef __FreeBSD__
    int s, seq, l, pid, rtm_addrs, i;
    struct sockaddr so_dst, so_mask;
    char *cp = m_rtmsg.m_space;
    struct sockaddr *gate = NULL, *sa;
    struct rt_msghdr *rtm_aux;

    pid = getpid();
    seq = 0;
    rtm_addrs = RTA_DST | RTA_NETMASK;

    memset(&so_dst,  0, sizeof(so_dst));
    memset(&so_mask, 0, sizeof(so_mask));
    memset(&rtm,     0, sizeof(struct rt_msghdr));

    rtm.rtm_type = RTM_GET;
    rtm.rtm_flags = RTF_UP | RTF_GATEWAY;
    rtm.rtm_version = RTM_VERSION;
    rtm.rtm_seq = ++seq;
    rtm.rtm_addrs = rtm_addrs;

    so_dst.sa_family = AF_INET;
    so_dst.sa_len = sizeof(struct sockaddr_in);
    so_mask.sa_family = AF_INET;
    so_mask.sa_len = sizeof(struct sockaddr_in);

    NEXTADDR(RTA_DST, so_dst);
    NEXTADDR(RTA_NETMASK, so_mask);

    rtm.rtm_msglen = l = cp - (char *)&m_rtmsg;

    s = socket(PF_ROUTE, SOCK_RAW, 0);

    if (write(s, (char *)&m_rtmsg, l) < 0)
        return;

    do
    {
      l = read(s, (char *)&m_rtmsg, sizeof(m_rtmsg));
    }
    while (l > 0 && (rtm.rtm_seq != seq || rtm.rtm_pid != pid));

    close(s);

    rtm_aux = &rtm;

    cp = ((char *)(rtm_aux + 1));
    if (rtm_aux->rtm_addrs)
    {
        for (i = 1; i; i <<= 1)
            if (i & rtm_aux->rtm_addrs)
            {
                sa = (struct sockaddr *)cp;
                if (i == RTA_GATEWAY )
                    gate = sa;
                ADVANCE(cp, sa);
            }
    }
    else
        return;

    if (gate)
    {
        enna_buffer_append(b, "<hilight>");
        enna_buffer_append(b, _("Default gateway:"));
        enna_buffer_append(b, "</hilight> ");

        enna_buffer_appendf(b, "%s<br>",
                            inet_ntoa(((struct sockaddr_in *)gate)->sin_addr));
    }
#else /* __FreeBSD__ */
    char devname[64];
    unsigned long d, g, m;
    int res, flgs, ref, use, metric, mtu, win, ir;
    FILE *fp;

    fp = fopen("/proc/net/route", "r");
    if (!fp)
        return;

    if (fscanf(fp, "%*[^\n]\n") < 0) /* Skip the first line. */
        return;

    enna_buffer_append(b, "<hilight>");
    enna_buffer_append(b, _("Default gateway:"));
    enna_buffer_append(b, "</hilight> ");
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
        enna_buffer_appendf(b, "%s<br>", inet_ntoa (gw));
        res = 1;
        break;
    }

    if (!res) {
        enna_buffer_append(b, _("None"));
        enna_buffer_append(b, "<br>");
    }


    fclose(fp);
#endif /* !__FreeBSD__ */
}

static void
set_system_information(Enna_Buffer *b)
{
    if (!b)
        return;

    enna_buffer_append(b, "<c>");
    enna_buffer_append(b, _("System information"));
    enna_buffer_append(b, "</c><br><br>");
    get_distribution(b);
    get_uname(b);
    get_cpuinfos(b);
    get_loadavg(b);
    get_ram_usage(b);
#ifdef BUILD_LIBSVDRP
    get_vdr(b);
#endif /* BUILD_LIBSVDRP */
#ifdef BUILD_LIBXRANDR
    get_resolution(b);
#endif /* BUILD_LIBXRANDR */
    get_network(b);
    get_default_gw(b);
}

/* ecore timer callback */
static Eina_Bool
_update_infos_cb(void *data)
{
    Enna_Buffer *b;
    Evas_Object *obj;
    obj = data;
    b = enna_buffer_new();
    set_enna_information(b);
    set_system_information(b);
    edje_object_part_text_set(obj, "sysinfo.text", b->buf);
    edje_object_signal_emit(obj, "sysinfo,show", "enna");
    enna_buffer_free(b);

    return ECORE_CALLBACK_RENEW;
}

/* externally accessible functions */
Evas_Object *
info_panel_show(void *data)
{
    /* create the panel main object */
    o_edje = edje_object_add(enna->evas);
    edje_object_file_set(o_edje, enna_config_theme_get (),
                         "activity/configuration/sysinfo");

    /* update info once and fire the first callback */
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
