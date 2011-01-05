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
#include "enna_config.h"
#include "logs.h"

#define ENNA_MODULE_NAME "spotify"

static Ecore_Timer *timer = NULL;
static sp_session *session = NULL;
static int is_logged_out = 1;
void (*metadata_updated_fn)(void);

typedef struct _Playlist_Container Playlist_Container;
struct _Playlist_Container
{
    Enna_Browser *browser;
    sp_playlistcontainer *pl_cont;
    Eina_List *playlists;
    Eina_List *tokens;
};

typedef struct _Playlist_Item Playlist_Item;
struct _Playlist_Item
{
    Enna_Browser *browser;
    Enna_File *file;
    sp_playlist *playlist;
};

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
void userinfo_updated(sp_session *session);
void play_token_lost(sp_session *session);
void log_message(sp_session *session, const char *data);

static sp_session_callbacks sp_callbacks = {
    &logged_in,
    &logged_out,
    &metadata_updated,
    &connection_error,
    NULL,
    &notify_main_thread,
    NULL,
    NULL,
    &log_message,
    NULL,
    NULL,
    &userinfo_updated,
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
    printf("t\n");
    return ECORE_CALLBACK_RENEW;
}

/**
 * This callback is called when an attempt to login has succeeded or failed.
 *
 * @sa sp_session_callbacks#logged_in
 */
void logged_in(sp_session *session, sp_error error)
{
    sp_user *me;
    const char *my_name;

    if (SP_ERROR_OK != error) {
        fprintf(stderr, "failed to log in to Spotify: %s\n",
                sp_error_message(error));
        sp_session_release(session);
        return;
    }

    // Let us print the nice message...
    me = sp_session_user(session);
    my_name = (sp_user_is_loaded(me) ? sp_user_display_name(me) : sp_user_canonical_name(me));



    DBG("Logged in to Spotify as user %s", my_name);
    is_logged_out = 0;
}

/**
 * This callback is called when the session has logged out of Spotify.
 *
 * @sa sp_session_callbacks#logged_out
 */
void logged_out(sp_session *session)
{
    is_logged_out = 1;  // Will exit mainloop
}

/**
 * Callback called when libspotify has new metadata available
 *
 * Not used in this example (but available to be able to reuse the session.c file
 * for other examples.)
 *
 * @sa sp_session_callbacks#metadata_updated
 */
void metadata_updated(sp_session *session)
{
    if(metadata_updated_fn)
        metadata_updated_fn();
}

/**
 * This callback is called when the user was logged in, but the connection to
 * Spotify was dropped for some reason.
 *
 * @sa sp_session_callbacks#connection_error
 */
void connection_error(sp_session *session, sp_error error)
{

    ERR("Connection to Spotify failed: %s", sp_error_message(error));
}


void notify_main_thread(sp_session *session)
{
    int tmp;
    sp_session_process_events(session, &tmp);
}

/**
 * Called after user info (anything related to sp_user objects) have been updated.
 *
 * @param[in]  session    Session
 */
void userinfo_updated(sp_session *session)
{
     int num;
    int i;
    sp_user *user;

    printf("Userinfo updated friends\n");

    num = sp_session_num_friends(session);
    for (i = 0; i < num; i++)
    {
        user = sp_session_friend(session, i);
        DBG("User %s | %s", sp_user_canonical_name(user),  sp_user_display_name(user));
    }
}

/**
 * This callback is called for log messages.
 *
 * @sa sp_session_callbacks#log_message
 */
void log_message(sp_session *session, const char *data)
{
    EVT("Session log : %s", data);
}

static void
playlist_renamed(sp_playlist *playlist, void *userdata)
{
    Playlist_Item *pl_item = userdata;
    const char *name;

    DBG("playlist renamed  new name : %s", sp_playlist_name(playlist));
    //if (sp_playlist_is_loaded(playlist))
    {
        name = sp_playlist_name(playlist);
        DBG("ASync renamed %s", sp_playlist_name(playlist));
        eina_stringshare_replace(&pl_item->file->name, name);
        eina_stringshare_replace(&pl_item->file->label, name);
        enna_browser_file_update(pl_item->browser, pl_item->file);

    }
}

static void
playlist_state_changed(sp_playlist *playlist, void *userdata)
{
    DBG("Playlist state changed");
}
static void
playlist_update_in_progress(sp_playlist *playlist, bool done, void *userdata)
{

    DBG("Update in progress : %s", done ? "no": "yes");

}

static void
description_changed(sp_playlist *playlist, const char *desc, void *userdata)
{
    DBG("Description changed : %s", desc);
}

static void image_changed(sp_playlist *playlist, const byte *image, void *userdata)
{
    DBG("Playlist image changed");
}

static sp_playlist_callbacks pl_cb = {
    NULL,
    NULL,
    NULL,
    playlist_renamed,
    playlist_state_changed,
    playlist_update_in_progress,
    NULL,
    NULL,
    NULL,
    description_changed,
    image_changed
};
#if 0
void tracks_added(sp_playlist *pl, sp_track * const *tracks, int num_tracks, int position, void *userdata)
{
  char mrl[4096];
  sp_link *link;
  sp_track *track;
  sp_album *album;
  int i;

  for (i = 0; i < num_tracks; i++)
    {
      track = tracks[i];
      album = sp_track_album(track);
      artist = sp_track_artist(track, 0);

      link = sp_link_create_from_track(track, 0);
      sp_link_as_string(link, mrl, sizeof(mrl));
      name = eina_stringshare_printf("%s",  sp_track_name(track));
      name = eina_stringshare_printf("%s (%s - %s)", sp_track_name(track),
	 sp_artist_name(artist),
	 sp_album_name(album));
      //uri = eina_stringshare_printf("%s/%d", enna_browser_uri_get(pl->browser), i);
      DBG("Name : %s, mrl %s\n", name, mrl);

      f1 = enna_file_file_add(name, uri, mrl, name, "icon/music");
      enna_browser_file_update(pl->browser, f1);
    }
}

static sp_playlist_callbacks pl_track_cb = {
    tracks_added,
    tracks_removed,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif

static void
playlist_added(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata)
{
    DBG("Pl added : %s at pos(%d)", sp_playlist_name(playlist), position);
}

static void
playlist_removed(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata)
{
    DBG("Pl removed : %s at pos(%d)", sp_playlist_name(playlist), position);
}
static void
playlist_moved(sp_playlistcontainer *pc, sp_playlist *playlist, int position, int new_position, void *userdata)
{
    DBG("Pl moved : %s at from(%d) to (%d)", sp_playlist_name(playlist), position, new_position);
}


static void
container_loaded(sp_playlistcontainer *pc, void *userdata)
{
    int num;
    int i;
    Playlist_Container *pl = userdata;
    const char *name;
    Enna_Buffer *uri;
    Enna_File *f1;
    sp_playlist *playlist;
    Playlist_Item *pl_item;

    DBG("container loaded");
    num = sp_playlistcontainer_num_playlists(pc);

    for (i = 0; i < num; i++)
    {
        playlist = sp_playlistcontainer_playlist(pc, i);
        name = sp_playlist_name(playlist);
        if (!name || !name[0])
            name = eina_stringshare_add("Loading ...");
        DBG("ASync added %s", sp_playlist_name(playlist));
        uri = enna_buffer_new();
        enna_buffer_appendf(uri, "%s/%d", enna_browser_uri_get(pl->browser), i);
        DBG("URI : %s", uri->buf);

        f1 = enna_file_menu_add(name, uri->buf, name, "icon/playlist");
        f1 = enna_browser_file_update(pl->browser, f1);

        pl_item = calloc(1, sizeof(Playlist_Item));
        pl_item->browser = pl->browser;
        pl_item->file = f1;
        pl_item->playlist = playlist;
        sp_playlist_add_callbacks(playlist, &pl_cb, pl_item);
        pl->playlists = eina_list_append(pl->playlists, pl_item);
        enna_buffer_free(uri);

    }
}

static sp_playlistcontainer_callbacks pc_cb = {
    playlist_added,
    playlist_removed,
    playlist_moved,
    container_loaded,
};


static const char *
_pl_meta_get(void *data, const Enna_File *file, const char *key)
{
    sp_playlist *pl = data;

    if (!strcmp(key, "nb_elements"))
    {
        return eina_stringshare_printf("%d", sp_playlist_num_tracks(pl));
    }

    return eina_stringshare_add("Test");
}

const char *_get_image(sp_image *image)
{
    const void *data;
    size_t size;
    FILE *fp;
    char name[45];
    const char *filename;
    const byte *id;
    int i,j;

    id = sp_image_image_id(image);
    printf("Image id : %s\n", id);
    for (i = 0, j = 0; i < 20; i++, j += 2)
    {
        sprintf(name+j, "%02X", id[i]);
        printf("%02X", id[i]);
    }
    name[40] = '.';
    name[41] = 'j';
    name[42] = 'p';
    name[43] = 'g';
    name[44] = '\0';


    filename = eina_stringshare_printf("/home/nico/.cache/enna/%s", name);
    fp = fopen(filename, "wb");
    printf("Create file : %s\n", filename);
    data = sp_image_data(image, &size);
    fwrite(data, size, 1, fp);
    fclose(fp);
    return filename;
}

static void
_image_loaded_cb(sp_image *image, void *userdata)
{
    const char *filename;
    filename = _get_image(image);
}

static const char *
_track_meta_get(void *data, const Enna_File *file, const char *key)
{
    sp_track *track = data;

    if (!track)
        return NULL;


    if (!strcmp(key, "cover"))
    {
        FILE *fp;
        const char *filename;
        int i, j;
        sp_album *album;
        sp_image *image;
        const byte *id;

        album = sp_track_album(track);
        printf("Album %p\n", album);
        id = sp_album_cover(album);
        printf("Album id : %s\n", id);
        image = sp_image_create(session, id);
        if (sp_image_is_loaded(image))
        {
            printf("Image is loaded\n");
            return _get_image(image);
        }
        else
        {
            printf("Cover loading...\n");
            sp_image_add_load_callback(image, _image_loaded_cb, NULL);
            return NULL;
        }
    }
    else if (!strcmp(key, "title"))
    {
        const char *track_name;
        track_name = sp_track_name(track);
        DBG("track name : %s\n", track_name);
        if (track_name[0])
            return eina_stringshare_add(track_name);
        return NULL;
    }
    else if (!strcmp(key, "author"))
    {
        sp_artist *artist;
        const char *artist_name;
        artist = sp_track_artist(track, 0);
        artist_name = sp_artist_name(artist);
        DBG("artist name : %s\n", artist_name);
        if (artist_name[0])
            return eina_stringshare_add(artist_name);
        return NULL;
    }
    else if (!strcmp(key, "album"))
    {
        sp_album *album;
        const char *album_name;
        album = sp_track_album(track);
        album_name = sp_album_name(album);
        DBG("Album name : %s\n", album_name);
        if (album_name[0])
            return eina_stringshare_add(album_name);
        return NULL;
    }
    else if (!strcmp(key, "starred"))
    {
        const char *starred;
        if (sp_track_is_starred(session, track))
            return eina_stringshare_add("starred");
        else
            return NULL;
    }


    return eina_stringshare_add("Test");
}

static Enna_File_Meta_Class track_meta = {
    _track_meta_get,
    NULL
};

static Enna_File_Meta_Class pl_meta = {
    _pl_meta_get,
    NULL
};

static void
_browse_root(Eina_List *tokens, Enna_Browser *browser)
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
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "playlists");

    if (is_logged_out)
      return;

    f1 = enna_file_menu_add("playlists", uri->buf, "Playlists", "icon/playlist");
    enna_buffer_free(uri);
    uri = enna_buffer_new();
    enna_buffer_appendf(uri, "%s/%s", buf->buf, "friends");

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
_browse_playlists(Eina_List *tokens, Enna_Browser *browser, Playlist_Container *pl)
{
    int num;
    int i;
    const char *name;
    Enna_File *f1;
    sp_playlist *playlist;
    Playlist_Item *pl_item;
    Enna_Buffer *buf;
    Enna_Buffer *uri;
    Eina_List *l;
    char *p;

    DBG("Spotify Playlists");
    buf = enna_buffer_new();
    uri = enna_buffer_new();

    enna_buffer_appendf(uri, "%s/%s", enna_browser_uri_get(pl->browser), "starred");

    f1 = enna_file_menu_add("starred", uri->buf, "Starred tracks", "icon/favorite");
    enna_browser_file_add(browser, f1);

    num = sp_playlistcontainer_num_playlists(pl->pl_cont);

    for (i = 0; i < num; i++)
    {
        playlist = sp_playlistcontainer_playlist(pl->pl_cont, i);

        name = sp_playlist_name(playlist);
        if (!name || !name[0])
            name = eina_stringshare_add("Loading ...");
        DBG("Sync added %s", name);
        uri = enna_buffer_new();
        enna_buffer_appendf(uri, "%s/%d", enna_browser_uri_get(pl->browser), i);
        DBG("%s", uri->buf);

        f1 = enna_file_menu_add(name, uri->buf, name, "icon/playlist");
        enna_file_meta_add(f1, &pl_meta, playlist);
        enna_browser_file_update(browser, f1);

        pl_item = calloc(1, sizeof(Playlist_Item));
        pl_item->browser = browser;
        pl_item->file = f1;
        pl_item->playlist = playlist;
        pl->playlists = eina_list_append(pl->playlists, pl_item);
        sp_playlist_add_callbacks(playlist, &pl_cb, pl_item);
        enna_buffer_free(uri);

    }
}

static void
_browse_friends(Eina_List *tokens, Enna_Browser *browser)
{
    int num;
    int i;
    sp_user *user;

    printf("Browser friends\n");

    num = sp_session_num_friends(session);
    for (i = 0; i < num; i++)
    {
        user = sp_session_friend(session, i);
        DBG("User %s | %s", sp_user_canonical_name(user),  sp_user_display_name(user));
    }
}

static void
_browse_playlist_id(Enna_Browser *browser, int id, Playlist_Container *pl)
{
    Playlist_Item *item;
    sp_playlist *playlist = sp_playlistcontainer_playlist(pl->pl_cont, id);
    int num;
    int i;
    sp_track *track;
    sp_artist *artist;
    sp_album *album;
    const char *name;
    Enna_File *f1;
    const char *uri;
    //printf("pl_cont %p\n", pl_cont);

    item = calloc(1, sizeof(Playlist_Item));
    item->browser = browser;
    item->playlist = playlist;

    //sp_playlist_add_callbacks(playlist, &pl_track_cb, pl);
    pl->playlists = eina_list_append(pl->playlists, item);

    num = sp_playlist_num_tracks(playlist);
    for (i = 0; i < num; i++)
    {
        char mrl[4096];
	sp_link *link;
        track = sp_playlist_track(playlist, i);
        album = sp_track_album(track);
        artist = sp_track_artist(track, 0);

	link = sp_link_create_from_track(track, 0);
	sp_link_as_string(link, mrl, sizeof(mrl));
	name = eina_stringshare_printf("%s",  sp_track_name(track));
	/* name = eina_stringshare_printf("%s (%s - %s)", sp_track_name(track),
                                       sp_artist_name(artist),
                                       sp_album_name(album));*/
        uri = eina_stringshare_printf("%s/%d", enna_browser_uri_get(pl->browser), i);
        DBG("Name : %s, mrl %s\n", name, mrl);

        f1 = enna_file_track_add(name, uri, mrl, name, "icon/music");
        enna_file_meta_add(f1, &track_meta, track);
        enna_browser_file_update(pl->browser, f1);
    }

}



static void *
_add(Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{
    Playlist_Container *pl;
    pl = calloc(1, sizeof(Playlist_Container));

    pl->browser = browser;
    pl->tokens = tokens;

    if (enna_browser_level_get(browser) == 2)
    {
        _browse_root(tokens, browser);
    }
    else if (enna_browser_level_get(browser) == 3)
    {
        char *token =  eina_list_nth(tokens, 2);
        DBG("%s", token);
        if (!strcmp(token, "friends"))
        {
            _browse_friends(tokens, browser);
        }
        else if (!strcmp(token, "playlists"))
        {
            pl->pl_cont = sp_session_playlistcontainer(session);
            _browse_playlists(tokens, browser, pl);
            sp_playlistcontainer_add_callbacks(pl->pl_cont, &pc_cb, pl);
        }
    }
    else if (enna_browser_level_get(browser) == 4)
    {
        char *parent, *child;
        parent = eina_list_nth(tokens, 2);
        child = eina_list_nth(tokens, 3);
        if (!strcmp(parent, "playlists"))
        {
            int id = atoi(child);
            printf("Browse Playlist ID : %d\n", id);
            pl->pl_cont = sp_session_playlistcontainer(session);
            _browse_playlist_id(browser, id, pl);
        }
    }

    return pl;
}

static void
_get_children(void *priv, Eina_List *tokens, Enna_Browser *browser, ENNA_VFS_CAPS caps)
{


}

static void
_del(void *priv)
{
    Playlist_Container *pl = priv;
    Playlist_Item *pl_item;

    if (pl->pl_cont)
         sp_playlistcontainer_remove_callbacks(pl->pl_cont, &pc_cb, pl);

    EINA_LIST_FREE(pl->playlists, pl_item)
    {
        sp_playlist_remove_callbacks(pl_item->playlist, &pl_cb, pl_item);
        if (pl_item) free(pl_item);
    }

    free(pl);

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
#define MOD_PREFIX enna_mod_browser_spotify
#endif /* USE_STATIC_MODULES */

static void
module_init(Enna_Module *em)
{
    sp_session_config config;
    sp_error error;
    const char *username;
    const char *password;


    if (!em)
        return;

    username = enna_config_string_get("spotify", "username");
    password = enna_config_string_get("spotify", "password");

    DBG("%s %s", username, password);

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
        ERR("failed to create session: %s",
            sp_error_message(error));
        return;
    }

    error = sp_session_login(session, username, password);
    if (SP_ERROR_OK != error)
    {        ERR("failed to login: %s", sp_error_message(error));
        return;
    }

    timer = ecore_timer_add(0.1, _timeout_cb, NULL);

    enna_vfs_register(&class, ENNA_CAPS_MUSIC);
}

static void
module_shutdown(Enna_Module *em)
{
    sp_session_logout(session);
    sp_session_release(session);
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

