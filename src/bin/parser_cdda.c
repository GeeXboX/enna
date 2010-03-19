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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#if defined(__linux__)
#include <linux/cdrom.h>
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <sys/cdio.h>
#elif defined(__MINGW32__) || defined(__CYGWIN__)
#include <ddk/ntddcdrm.h>
#elif (__bsdi__)
#include <dvd.h>
#elif defined(__APPLE__) || defined(__DARWIN__)
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#endif

#include "enna.h"
#include "logs.h"
#include "parser_cdda.h"

#define MODULE_NAME "cdda"

#ifdef BUILD_LIBCDDB
#include <cddb/cddb.h>
#endif

static cdda_track_t *
cdda_track_new (void)
{
    cdda_track_t *t;

    t = calloc (1, sizeof (cdda_track_t));

    return t;
}

static void
cdda_track_free (cdda_track_t *t)
{
    if (!t)
      return;

    if (t->name)
        free (t->name);
    free (t);
    t = NULL;
}

static cdda_t *
cdda_new (void)
{
    cdda_t *c;

    c = calloc (1, sizeof (cdda_t));

    return c;
}

void
cdda_free (cdda_t *c)
{
    int i;

    if (!c)
        return;

    if (c->artist)
        free (c->artist);
    if (c->title)
        free (c->title);
    if (c->ext_data)
        free (c->ext_data);
    if (c->genre)
        free (c->genre);

    for (i = 0; i < c->total_tracks; i++)
        cdda_track_free (c->tracks[i]);
    free (c->tracks);

    free (c);
}

static int
cd_read_toc (cdda_t *cd, const char *dev)
{
    int first = 0, last = -1;
    int i;
#if defined(__MINGW32__) || defined(__CYGWIN__)
    HANDLE drive;
    DWORD r;
    CDROM_TOC toc;
    char device[10];

    if (!cd || !dev)
        return 1;

    sprintf(device, "\\\\.\\%s", dev);
    drive = CreateFile(device, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

    if(!DeviceIoControl(drive, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof(CDROM_TOC), &r, 0))
    {  
        enna_log(ENNA_MSG_ERR, "parser_cdda",
                 "Device IO Control Failed");
        return 1;
    }

    first = toc.FirstTrack - 1; last = toc.LastTrack;
    cd->total_tracks = last;
    cd->tracks = calloc (cd->total_tracks + 1, sizeof (cdda_track_t *));

    for (i = first; i <= last; i++)
    {
        cdda_track_t *track;

        track         = cdda_track_new ();
        track->min    = toc.TrackData[i].Address[1];
        track->sec    = toc.TrackData[i].Address[2];
        track->frame  = toc.TrackData[i].Address[3];

        cd->tracks[i] = track;
    }
    CloseHandle(drive);
#else /* !defined(__MINGW32__) && !defined(__CYGWIN__) */
    int drive;

    if (!cd || !dev)
        return 1;

    drive = open (dev, O_RDONLY | O_NONBLOCK);
    if (drive < 0)
        return 1;

#if defined(__linux__) || defined(__bsdi__)
    struct cdrom_tochdr tochdr;

    ioctl (drive, CDROMREADTOCHDR, &tochdr);
    first = tochdr.cdth_trk0 - 1;
    last  = tochdr.cdth_trk1;

    cd->total_tracks = last;
    cd->tracks = calloc (cd->total_tracks + 1, sizeof (cdda_track_t *));

    for (i = first; i <= last; i++)
    {
        struct cdrom_tocentry tocentry;
        cdda_track_t *track;

        tocentry.cdte_track = (i == last) ? 0xAA : i + 1;
        tocentry.cdte_format = CDROM_MSF;

        ioctl (drive, CDROMREADTOCENTRY, &tocentry);

        track         = cdda_track_new ();
        track->min    = tocentry.cdte_addr.msf.minute;
        track->sec    = tocentry.cdte_addr.msf.second;
        track->frame  = tocentry.cdte_addr.msf.frame;

        cd->tracks[i] = track;
    }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
    struct ioc_toc_header tochdr;

    ioctl(drive, CDIOREADTOCHEADER, &tochdr);
    first = tochdr.starting_track - 1;
    last = tochdr.ending_track;

    cd->total_tracks = last;
    cd->tracks = calloc (cd->total_tracks + 1, sizeof (cdda_track_t *));

    for (i = first; i <= last; i++)
    {
        struct ioc_read_toc_single_entry tocentry;
        cdda_track_t *track;

        tocentry.track = (i == last) ? 0xAA : i + 1;
        tocentry.address_format = CD_MSF_FORMAT;

        ioctl(drive, CDIOREADTOCENTRY, &tocentry);

        track         = cdda_track_new ();
        track->min    = tocentry.entry.addr.msf.minute;
        track->sec    = tocentry.entry.addr.msf.second;
        track->frame  = tocentry.entry.addr.msf.frame;

        cd->tracks[i] = track;
    }
#elif defined(__NetBSD__) || defined(__OpenBSD__)
    struct ioc_toc_header tochdr;

    ioctl(drive, CDIOREADTOCHEADER, &tochdr);
    first = tochdr.starting_track - 1;
    last = tochdr.ending_track;

    cd->total_tracks = last;
    cd->tracks = calloc (cd->total_tracks + 1, sizeof (cdda_track_t *));

    for (i = first; i <= last; i++)
    {
        struct ioc_read_toc_entry tocentry;
        struct cd_toc_entry toc_buffer;
        cdda_track_t *track;

        tocentry.starting_track = (i == last) ? 0xAA : i + 1;
        tocentry.address_format = CD_MSF_FORMAT;
        tocentry.data = &toc_buffer;
        tocentry.data_len = sizeof(toc_buffer);
        ioctl(drive, CDIOREADTOCENTRYS, &tocentry);

        track         = cdda_track_new ();
        track->min    = toc_buffer.addr.msf.minute;
        track->sec    = toc_buffer.addr.msf.second;
        track->frame  = toc_buffer.addr.msf.frame;

        cd->tracks[i] = track;
    }
#elif defined(__APPLE__) || defined(__DARWIN__)
    dk_cd_read_toc_t tochdr;
    uint8_t buf[4];
    uint8_t buf2[100 * sizeof(CDTOCDescriptor) + sizeof(CDTOC)];

    memset(&tochdr, 0, sizeof(tochdr));
    tochdr.bufferLength = sizeof(buf);
    tochdr.buffer = &buf;
    if (!ioctl(drive, DKIOCCDREADTOC, &tochdr) && tochdr.bufferLength == sizeof(buf))
    {
        first = buf[2] - 1;
        last = buf[3];
    }
    if (last >= 0)
    {
        memset(&tochdr, 0, sizeof(tochdr));
        tochdr.bufferLength = sizeof(buf2);
        tochdr.buffer = &buf2;
        tochdr.format = kCDTOCFormatTOC;
        if (ioctl(drive, DKIOCCDREADTOC, &tochdr) || tochdr.bufferLength < sizeof(CDTOC))
            last = -1;
    }
    if (last >= 0)
    {
        CDTOC *cdToc = (CDTOC *)buf2;
        CDTrackInfo lastTrack;
        dk_cd_read_track_info_t trackInfoParams;

        for (i = first; i < last; ++i)
        {
            CDMSF msf = CDConvertTrackNumberToMSF(i + 1, cdToc);
            cdda_track_t *track;

            track         = cdda_track_new ();
            track->min    = msf.minute;
            track->sec    = msf.second;
            track->frame  = msf.frame;

            cd->tracks[i] = track;
        }

        memset(&trackInfoParams, 0, sizeof(trackInfoParams));
        trackInfoParams.addressType = kCDTrackInfoAddressTypeTrackNumber;
        trackInfoParams.bufferLength = sizeof(lastTrack);
        trackInfoParams.address = last;
        trackInfoParams.buffer = &lastTrack;

        if (!ioctl(drive, DKIOCCDREADTRACKINFO, &trackInfoParams))
        {
            CDMSF msf = CDConvertLBAToMSF(be2me_32(lastTrack.trackStartAddress) + be2me_32(lastTrack.trackSize));
            cdda_track_t *track;

            track         = cdda_track_new ();
            track->min    = msf.minute;
            track->sec    = msf.second;
            track->frame  = msf.frame;

            cd->tracks[i] = track;
        }
    }
#endif /* defined(__APPLE__) || defined(__DARWIN__) */
    close (drive);
#endif /* defined(__MINGW32__) || defined(__CYGWIN__) */
    for (i = first; i <= last; i++)
        cd->tracks[i]->frame
            += (cd->tracks[i]->min * 60 + cd->tracks[i]->sec) * 75;

    return 0;
}

static unsigned int
cd_do_checksum (int n)
{
    unsigned int ret = 0;

    while (n > 0)
    {
        ret += (n % 10);
        n /= 10;
    }

    return ret;
}

static void
cd_get_discid (cdda_t *cd)
{
    unsigned int i, t, n = 0;

    if (!cd)
        return;

    for (i = 0; i < cd->total_tracks; i++)
        n += cd_do_checksum ((cd->tracks[i]->min * 60) + cd->tracks[i]->sec);

    t = ((cd->tracks[cd->total_tracks]->min * 60)
         + cd->tracks[cd->total_tracks]->sec) -
        ((cd->tracks[0]->min * 60) + cd->tracks[0]->sec);

    cd->id = (((unsigned long) n % 0xFF) << 24 | t << 8 | cd->total_tracks);
}

static cdda_t *
cd_identify (const char *dev)
{
    cdda_t *cd;
    int err;

    cd = cdda_new ();

    err = cd_read_toc (cd, dev);
    if (err)
    {
        enna_log (ENNA_MSG_ERROR, MODULE_NAME, "Unable to read CD TOC.");
        goto err_cd_read_toc;
    }

    cd_get_discid (cd);

    return cd;

 err_cd_read_toc:
    cdda_free (cd);

    return NULL;
}

#ifdef BUILD_LIBCDDB
static void
cd_get_metadata (cdda_t *cd, cddb_disc_t *disc)
{
    if (!cd || !disc)
        return;

    if (cddb_disc_get_artist (disc))
        cd->artist = strdup (cddb_disc_get_artist (disc));
    if (cddb_disc_get_title (disc))
        cd->title = strdup (cddb_disc_get_title (disc));
    if (cddb_disc_get_ext_data (disc))
        cd->ext_data = strdup (cddb_disc_get_ext_data (disc));
    if (cddb_disc_get_genre (disc))
        cd->genre = strdup (cddb_disc_get_genre (disc));

    cd->year = cddb_disc_get_year (disc);
    cd->length = cddb_disc_get_length (disc);
}

static void
cd_get_tracks (cdda_t *cd, cddb_disc_t *disc)
{
    cddb_track_t *track;
    int i = 0;

    if (!cd || !disc)
        return;

    for (track = cddb_disc_get_track_first (disc); track;
         track = cddb_disc_get_track_next (disc))
    {
        char name[128];
        int length;

        length = cddb_track_get_length (track);
        snprintf (name, 128, "%02d. %s (%02d:%02d)",
                  cddb_track_get_number (track),
                  cddb_track_get_title (track),
                  (length / 60), (length % 60));

        cd->tracks[i]->name = strdup (name);
        i++;
    }
}

static void
parse_cddb (cdda_t *cd)
{
    cddb_disc_t *disc;
    cddb_conn_t *conn;
    int res;

    if (!cd)
        return;

    libcddb_init ();
    conn = cddb_new ();
    disc = cddb_disc_new ();

    cddb_disc_set_category_str (disc, "misc");
    cddb_disc_set_discid (disc, cd->id);

    res = cddb_read (conn, disc);
    if (!res)
        goto err_cddb_read;

    cd_get_metadata (cd, disc);
    cd_get_tracks (cd, disc);

 err_cddb_read:
    cddb_disc_destroy (disc);
    libcddb_shutdown ();
}
#else
static void
cd_get_tracks (cdda_t *cd)
{
    int i;

    if (!cd)
        return;

    for (i = 1; i <= cd->total_tracks; i++)
    {
        int min, sec, frame;
        char name[128];

        frame = cd->tracks[i]->frame - cd->tracks[i-1]->frame;
        sec = frame / 75;
        frame -= sec * 75;
        min = sec / 60;
        sec -= min * 60;

        snprintf (name, 128, _("Track #%02d (%02d:%02d)"), i, min, sec);
        cd->tracks[i-1]->name = strdup (name);
    }
}
#endif

static void
cd_display_info (cdda_t *cd)
{
    int i;

    if (!cd)
        return;

    enna_log (ENNA_MSG_INFO, MODULE_NAME, "DiscID: %lu", cd->id);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Artist: %s", cd->artist);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Title: %s", cd->title);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Ext.Data: %s", cd->ext_data);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Genre: %s", cd->genre);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Year: %d", cd->year);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Length: %d seconds", cd->length);
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "");
    enna_log (ENNA_MSG_INFO, MODULE_NAME, "Tracks:");
    for (i = 0; i < cd->total_tracks; i++)
        enna_log (ENNA_MSG_INFO, MODULE_NAME, "  %s", cd->tracks[i]->name);
}

cdda_t *
cdda_parse (const char *device)
{
    cdda_t *cd;

    if (!device)
        return NULL;

    cd = cd_identify (device);

#ifdef BUILD_LIBCDDB
    parse_cddb (cd);
#else
    cd_get_tracks (cd);
#endif

    cd_display_info (cd);

    return cd;
}
