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

#include <stdio.h>
#include <string.h>
#include <mntent.h>

#include "enna.h"
#include "enna_config.h"
#include "module.h"
#include "volumes.h"
#include "logs.h"

#define ENNA_MODULE_NAME   "mtab"

#define MTAB_FILE          "/etc/mtab"

typedef enum {
    MTAB_TYPE_NONE,
    MTAB_TYPE_NFS,
    MTAB_TYPE_SMB,
} MTAB_TYPE;

static Eina_List *_mount_points = NULL;

/***************************************/
/*           mtab handling             */
/***************************************/

static void
mtab_add_mnt(MTAB_TYPE t, char *fsname, char *dir)
{
    Enna_Volume *v;
    char name[512], tmp[1024], srv[128], share[128];
    char *p;
    ENNA_VOLUME_TYPE type = VOLUME_TYPE_UNKNOWN;

    if(t == MTAB_TYPE_NONE)
        return;

    if(!fsname || !dir)
        return;

    memset(name,  '\0', sizeof(name));
    memset(tmp,   '\0', sizeof(tmp));
    memset(srv,   '\0', sizeof(srv));
    memset(share, '\0', sizeof(share));

    snprintf(tmp, sizeof(tmp), "file://%s", dir);

    switch(t)
    {
    case MTAB_TYPE_NFS:
        p = strchr(fsname, ':');
        if(!p)
            return;
        strncpy(srv, fsname, p - fsname);
        strcpy(share, p + 1);
        snprintf(name, sizeof(name), _("[NFS] %s on %s"), share, srv);
        type = VOLUME_TYPE_NFS;
        break;

    case MTAB_TYPE_SMB:
        p = strchr(fsname + 2, '/');
        if(!p)
            return;
        strncpy(srv, fsname + 2, p - (fsname + 2));
        strcpy(share, p + 1);
        snprintf(name, sizeof(name), _("[SAMBA] %s on %s"), share, srv);
        type = VOLUME_TYPE_SMB;
        break;

    default:
        break;
    }

    v              = enna_volume_new ();
    v->type        = type;
    v->device_name = eina_stringshare_add(srv);
    v->label       = eina_stringshare_add(name);
    v->mount_point = eina_stringshare_add(tmp);

    enna_log(ENNA_MSG_EVENT, "mtab",
             "New mount point discovered at %s", fsname);
    enna_log(ENNA_MSG_EVENT, "mtab",
             "Add mount point [%s] %s", v->label, v->mount_point);
    _mount_points = eina_list_append(_mount_points, v);

    enna_volumes_add_emit(v);
}

static void
mtab_parse(void)
{
    struct mntent *mnt;
    FILE *fp;

    fp = fopen(MTAB_FILE, "r");
    if(!fp)
        return;

    while((mnt = getmntent(fp)))
    {
        MTAB_TYPE type = MTAB_TYPE_NONE;

        if(!strcmp(mnt->mnt_type, "nfs") ||
            !strcmp(mnt->mnt_type, "nfs4"))
            type = MTAB_TYPE_NFS;
        else if(!strcmp(mnt->mnt_type, "smbfs") ||
                 !strcmp(mnt->mnt_type, "cifs"))
            type = MTAB_TYPE_SMB;
        else
            continue;

        mtab_add_mnt(type, mnt->mnt_fsname, mnt->mnt_dir);
    }

    endmntent(fp);
}

/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_volume_mtab
#endif /* USE_STATIC_MODULES */

Enna_Module_Api ENNA_MODULE_API = {
    ENNA_MODULE_VERSION,
    "volume_mtab",
    N_("Volumes from mtab"),
    NULL,
    N_("This module get volumes from the mtab file"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla."
};

void
ENNA_MODULE_INIT(Enna_Module *em)
{
    mtab_parse();
}

void
ENNA_MODULE_SHUTDOWN(Enna_Module *em)
{
    Enna_Volume *v;
    Eina_List *l, *l_next;
    EINA_LIST_FOREACH_SAFE(_mount_points, l, l_next, v)
    {
        enna_log(ENNA_MSG_EVENT, "mtab", "Remove %s", v->label);
        _mount_points = eina_list_remove(_mount_points, v);
        enna_volume_free (v);
    }
    _mount_points = NULL;
    enna_log(ENNA_MSG_EVENT, "mtab", "mtab module shutdown");
}
