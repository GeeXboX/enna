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

#include <Ecore.h>
#include <libspotify/api.h>

#include "enna.h"
#include "vfs.h"
#include "module.h"
#include "buffer.h"

#define ENNA_MODULE_NAME "spotify"

static Ecore_Timer *timer = NULL;
static int count = 0;
static sp_session *session;

const uint8_t g_appkey[] = {
        0x01, 0x2A, 0x17, 0x2B, 0x48, 0x84, 0x49, 0x2C, 0x43, 0x85, 0x43, 0xD7, 0x08, 0x8D, 0xE3, 0xDA,
        0x70, 0xE7, 0x1D, 0xD5, 0x01, 0x44, 0x11, 0xBE, 0xD3, 0x8E, 0xE3, 0xDF, 0x7E, 0x3B, 0xE3, 0x38,
        0x49, 0x4D, 0x92, 0x1C, 0xC5, 0x9F, 0xE7, 0x4C, 0xD0, 0xF3, 0x14, 0xFF, 0xFD, 0xF0, 0x0C, 0xFD,
        0x69, 0x78, 0xD2, 0x30, 0x80, 0xAA, 0xDB, 0x07, 0x21, 0xDC, 0xF4, 0x21, 0x33, 0xB7, 0xF0, 0x48,
        0x9D, 0x1A, 0xBC, 0x09, 0x1D, 0xD7, 0x54, 0x84, 0xB3, 0xF8, 0xAA, 0x47, 0xCE, 0x6A, 0x95, 0x9B,
        0x89, 0xF3, 0xB0, 0x38, 0x1D, 0x4C, 0x50, 0xC7, 0x80, 0x70, 0x00, 0x3B, 0x6C, 0x8E, 0x59, 0xCD,
        0x93, 0x32, 0x1D, 0x0E, 0x14, 0x07, 0xB2, 0xD7, 0x28, 0x16, 0x6E, 0xA8, 0x75, 0x8B, 0xEA, 0x24,
        0x5E, 0xC2, 0x69, 0xDA, 0x18, 0x06, 0x0A, 0xA7, 0xFD, 0x42, 0x06, 0xD4, 0xCC, 0xE1, 0x05, 0xC0,
        0xC1, 0xAB, 0x8F, 0x9A, 0xF6, 0x97, 0x1E, 0xE4, 0x8C, 0x09, 0xB3, 0x0F, 0x32, 0x83, 0x53, 0xC2,
        0xB5, 0xAA, 0x81, 0xB2, 0x55, 0xF1, 0x70, 0xA0, 0x82, 0xB1, 0x3D, 0x1B, 0x24, 0x7A, 0x9E, 0x09,
        0x1B, 0xB3, 0x75, 0x97, 0x5E, 0x1D, 0x48, 0xB8, 0xD9, 0xD2, 0x69, 0x67, 0x6B, 0x43, 0x6A, 0xB6,
        0x5A, 0x80, 0x33, 0x90, 0xDF, 0x27, 0x90, 0xAE, 0xF3, 0xA7, 0xAA, 0x05, 0x86, 0x8F, 0x3C, 0xF4,
        0x66, 0xB5, 0x8C, 0xC3, 0xDB, 0x7D, 0xF3, 0x64, 0xC2, 0x6E, 0x73, 0xCD, 0xAE, 0xDB, 0x74, 0x5E,
        0xCC, 0x4C, 0x5D, 0x22, 0x93, 0xF7, 0x4A, 0xE2, 0x80, 0x4E, 0xBE, 0xB2, 0x66, 0xD0, 0xA4, 0x1C,
        0x68, 0x5A, 0xD9, 0x81, 0xFC, 0x8D, 0x86, 0x31, 0x93, 0x12, 0xEE, 0x5E, 0xF3, 0x71, 0x24, 0xA6,
        0x9B, 0xCF, 0x94, 0x77, 0xFB, 0x21, 0x32, 0x4B, 0x4F, 0x63, 0x9C, 0x60, 0x60, 0xD0, 0x20, 0x86,
        0xED, 0x97, 0x66, 0xA5, 0xDA, 0x88, 0x07, 0x06, 0x4B, 0xEF, 0x1E, 0xAA, 0x48, 0x6B, 0xEF, 0x7F,
        0xAA, 0x2E, 0x22, 0x77, 0xF4, 0xED, 0x40, 0x83, 0xAC, 0x91, 0xE5, 0xC3, 0xC3, 0xB7, 0x41, 0xEB,
        0x6B, 0x7C, 0xBC, 0x1B, 0x05, 0x5D, 0x37, 0x7E, 0x73, 0x9D, 0xB0, 0xA7, 0x88, 0x04, 0x44, 0x5D,
        0x54, 0xF6, 0x22, 0x1A, 0xF1, 0x81, 0xF3, 0xE3, 0x42, 0xED, 0x9A, 0xE0, 0x38, 0x48, 0xBC, 0x4A,
        0xB8,
};
const size_t g_appkey_size = sizeof(g_appkey);
void logged_in(sp_session *session, sp_error error);
void logged_out(sp_session *session);
void metadata_updated(sp_session *session);
void connection_error(sp_session *session, sp_error error);
void notify_main_thread(sp_session *session);
int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
void play_token_lost(sp_session *session);
void log_message(sp_session *session, const char *data);

static sp_session_callbacks sp_callbacks = {
    &logged_in,
    &logged_out,
    &metadata_updated,
    &connection_error,
    NULL,
    &notify_main_thread,
    NULL,/*&music_delivery,*/
    NULL,/*play_token_lost,*/
    &log_message,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static Eina_Bool
_timeout_cb(void *data)
{
    int timeout = 1000;
    sp_session_process_events(session, &timeout);
    ecore_timer_interval_set(timer, (double)(timeout / 1000.0));
    return ECORE_CALLBACK_RENEW;
}

static void *
_add(Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{

    return NULL;
}

static void
_get_children(void *priv, Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    Enna_File *f1;
    Enna_File *f2;
    Enna_Buffer *buf;
    Enna_Buffer *uri;
    Eina_List *l;
    char *p;

    buf = enna_buffer_new();
    uri = enna_buffer_new();

    EINA_LIST_FOREACH(tokens, l, p)
    enna_buffer_appendf(buf, "/%s", p);
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "artist");

    f1 = enna_file_menu_add("playlists", uri->buf, "Playlists", "icon/playlists");
    enna_buffer_free(uri);
    uri = enna_buffer_new();
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "playlits");

    f2 = enna_file_menu_add("friends", uri->buf, "Friends", "icon/friends");
    enna_buffer_free(uri);
    uri = enna_buffer_new();
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "friends");

    enna_buffer_free(uri);
    enna_buffer_free(buf);

    enna_browser_file_add(browser, f1);
    enna_browser_file_add(browser, f2);



}

static void
_del(void *priv)
{
    ecore_timer_del(timer);
    count = 0;
    timer = NULL;
}

static Enna_Vfs_Class class = {
    "spotify",
    1,
    N_("Enjoy your music with Spotify"),
    NULL,
    "icon/spotify",
    {
        _add,
        _get_children,
        _del
    },
    NULL
};


/* Module interface */

#ifdef USE_STATIC_MODULES
#undef MOD_PREFIX
#define MOD_PREFIX enna_mod_browser_jamendo
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    sp_session_config config;
    sp_error error;
    char *username;
    char *password;


    if (!em)
        return;

    username = enna_config_string_get("spotify", "username");
    password = enna_config_string_get("spotify", "password");

    config.api_version = SPOTIFY_API_VERSION;
    config.cache_location = "tmp";
    config.settings_location = "tmp";
    config.application_key = g_appkey;
    config.application_key_size = g_appkey_size;
    config.user_agent = "enna";
    config.callbacks = &sp_callbacks;
    error = sp_session_create(&config, &session);
    if (SP_ERROR_OK != error)
      {
        printf("failed to create session: %s\n",
               sp_error_message(error));
        return;
      }

    error = sp_session_login(session, username, password);
    if (SP_ERROR_OK != error)
      {        printf("failed to login: %s\n", sp_error_message(error));
        return;
      }

    timer = ecore_timer_add(3.0, _timeout_cb, NULL);

    enna_vfs_register(&class, ENNA_CAPS_MUSIC);
}

static void
module_shutdown(Enna_Module *em)
{

}

Enna_Module_Api ENNA_MODULE_API =
{
    ENNA_MODULE_VERSION,
    "browser_spotify",
    N_("Spotify Music Library"),
    "icon/spotify",
    N_("Browse files of spotify music library"),
    "bla bla bla<br><b>bla bla bla</b><br><br>bla.",
    {
        module_init,
        module_shutdown
    }
};

