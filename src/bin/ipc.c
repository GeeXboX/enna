/*
 * GeeXboX Enna Media Center.
 * Copyright (C) 2005-2009 The Enna Project
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
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <Ecore.h>

#include <Ecore_Ipc.h>
#include <Ecore_File.h>

#include "gettext.h"
#include "utils.h"
#include "thumb.h"

/* local subsystem functions */
static int _ipc_cb_client_add(void *data, int type, void *event);
static int _ipc_cb_client_del(void *data, int type, void *event);
static int _ipc_cb_client_data(void *data, int type, void *event);

/* local subsystem globals */
static Ecore_Ipc_Server *_ipc_server = NULL;


/* externally accessible functions */
EAPI int
enna_ipc_init(void)
{

   char buf[1024];
   char *tmp, *user, *disp;
   int pid;

   ecore_ipc_init();

   tmp = getenv("TMPDIR");
   if (!tmp) tmp = "/tmp";
   user = getenv("USER");
   if (!user) user = "__unknown__";
   disp = getenv("DISPLAY");
   if (!disp) disp = ":0";
   pid = (int)getpid();
   snprintf(buf, sizeof(buf), "%s/enna-%s", tmp, user);
   if (mkdir(buf, S_IRWXU) == 0)
     {
     }
   else
     {
        struct stat st;

        if (stat(buf, &st) == 0)
          {
             if ((st.st_uid ==
                  getuid()) &&
                 ((st.st_mode & (S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO)) ==
                  (S_IRWXU|S_IFDIR)))
               {
               }
             else
               {
                  printf(_("Possible IPC Hack Attempt. The IPC socket\n"
                                         "directory already exists BUT has permissions\n"
                                         "that are too leanient (must only be readable\n" "and writable by the owner, and nobody else)\n"
                                         "or is not owned by you. Please check:\n"
                                         "%s/enlightenment-%s\n"), tmp, user);
                  return 0;
               }
          }
        else
          {
             printf(_("The IPC socket directory cannot be created or\n"
                                    "examined.\n"
                                    "Please check:\n"
                                    "%s/enlightenment-%s\n"),
                                  tmp, user);
             return 0;
          }
     }
   snprintf(buf, sizeof(buf), "%s/enna-%s/disp-%s-%i", tmp, user, disp, pid);
   _ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_SYSTEM, buf, 0, NULL);
   enna_util_env_set("ENNA_IPC_SOCKET", "");
   if (!_ipc_server) return 0;
   enna_util_env_set("ENNA_IPC_SOCKET", buf);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, _ipc_cb_client_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, _ipc_cb_client_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, _ipc_cb_client_data, NULL);

   return 1;
}

EAPI int
enna_ipc_shutdown(void)
{

   if (_ipc_server)
     {
        ecore_ipc_server_del(_ipc_server);
        _ipc_server = NULL;
     }

   ecore_ipc_shutdown();

   return 1;
}


/* local subsystem globals */
static int
_ipc_cb_client_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _ipc_server) return 1;
   return 1;
}

static int
_ipc_cb_client_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Del *e;
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _ipc_server) return 1;
   /* delete client sruct */
   enna_thumb_client_del(e);
   return 1;
}

static int
_ipc_cb_client_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _ipc_server) return 1;
   switch (e->major)
     {
      case 5:
        enna_thumb_client_data(e);
        break;
      default:
        break;
     }
   return 1;
}
