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

#define _GNU_SOURCE
#include <string.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Elementary.h>
#include <Edje.h>
#include <time.h>

#include "enna.h"
#include "enna_config.h"
#include "logs.h"
#include "module.h"
#include "content.h"
#include "mainmenu.h"
#include "view_list.h"
#include "vfs.h"
#include "url_utils.h"
#include "utils.h"
#include "bookstore.h"
#include "bookstore_gocomics.h"
#include "xdg.h"

#define ENNA_MODULE_NAME         "gocomics"

#define GOCOMICS_QUERY           "http://www.gocomics.com/%s/%s"
#define GOCOMICS_NEEDLE_START    "<link rel=\"image_src\" href=\""
#define GOCOMICS_NEEDLE_END      "\""
#define GOCOMICS_PATH            "gocomics"

typedef struct _BookStore_Service_GoComics {
    char *path;
    url_t url;
    char *comic_name;
    char *comic_id;
    int year;
    int month;
    int day;
    struct tm *t;
    Evas_Object *edje;
    Evas_Object *list;
} BookStore_Service_GoComics;

static const struct {
  const char *name;
  const char *id;
} gocomics_list_map[] = {
    { "B.C.",                        "bc"                      },
    { "Bloom County",                "bloomcounty"             },
    { "Calvin and Hobbes",           "calvinandhobbes"         },
    { "For Better or For Worse",     "forbetterorforworse"     },
    { "Garfield",                    "garfield"                },
    { "Non Sequitur",                "nonsequitur"             },
    { "Pickles",                     "pickles"                 },
    { "Shoe",                        "shoe"                    },
    { "Wizard of ID",                "wizardofid"              },
    { NULL,                          NULL                      }
};

static BookStore_Service_GoComics *mod;

/****************************************************************************/
/*                         GoComics.com Module API                          */
/****************************************************************************/

#define DATE_SET()                              \
    {                                           \
      mod->t     = localtime(&ti);              \
      mod->year  = mod->t->tm_year + 1900;      \
      mod->month = mod->t->tm_mon + 1;          \
      mod->day   = mod->t->tm_mday;             \
    }

#define DATE_WHOLE_DAY 86400 /* (60x60x24) */

static void
gocomics_date_set_current (void)
{
    time_t ti;

    ti         = time(NULL);
    DATE_SET();
}

static void
gocomics_date_previous (void)
{
    time_t ti;

    if (!mod->t)
        gocomics_date_set_current ();

    ti  = mktime(mod->t);
    ti -= DATE_WHOLE_DAY;
    DATE_SET();
}

static void
gocomics_date_next (void)
{
    time_t ti, ti_current;

    if (!mod->t)
        gocomics_date_set_current ();

    ti  = mktime(mod->t);
    ti += DATE_WHOLE_DAY;
    ti_current = time(NULL);

    /* prevent from going into future (could change space-time continum) */
    if (ti > ti_current)
        return;

    DATE_SET();
}

static void
gocomics_set_comic_strip (void)
{
    char img_dst[1024] = { 0 };
    char query[1024] = { 0 };
    char tdate[16] = { 0 };
    char *ptr_start, *ptr_end, *img_src = NULL;
    url_data_t data = { 0 };

    if (!mod->comic_id)
        return;

    /* get expected destination file name */
    snprintf(img_dst, sizeof(img_dst),
             "%s/%s_%.4d_%.2d_%.2d", mod->path, mod->comic_id,
             mod->year, mod->month, mod->day);

    /* no need to perform a web request for an already existing file */
    if (ecore_file_exists(img_dst))
        goto comic_strip_show;

    /* compute the requested URL */
    snprintf (tdate, sizeof (tdate), "%.4d/%.2d/%.2d\n",
              mod->year, mod->month, mod->day);
    snprintf (query, sizeof(query), GOCOMICS_QUERY, mod->comic_id, tdate);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Query Request: %s", query);

    /* perform request */
    data = url_get_data(mod->url, query);
    if (!data.buffer)
        return;

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Query Result: %s", data.buffer);

    /* find the comic strip url */
    ptr_start = strstr(data.buffer, GOCOMICS_NEEDLE_START);
    if (!ptr_start)
        goto err_url;

    ptr_start += strlen(GOCOMICS_NEEDLE_START);
    ptr_end = strstr(ptr_start, GOCOMICS_NEEDLE_END);
    if (!ptr_end)
        goto err_url;

    /* download and save the comic strip file */
    img_src = strndup (ptr_start, strlen(ptr_start) - strlen(ptr_end));
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Saving Comic Strip URL %s to %s", img_src, img_dst);
    url_save_to_disk(mod->url, img_src, img_dst);

    /* display the comic strip */
comic_strip_show:
    bs_service_page_show(img_dst);

err_url:
    ENNA_FREE(img_src);
    ENNA_FREE(data.buffer);
}

static void
gocomics_set_comic_name (void)
{
    char name[128] = { 0 };

    if (!mod->comic_name)
    {
        edje_object_part_text_set (mod->edje,
                                   "service.book.name.str", "GoComics");
        return;
    }

    snprintf (name, sizeof(name), "%s - %.4d/%.2d/%.2d",
              mod->comic_name, mod->year, mod->month, mod->day);
    edje_object_part_text_set (mod->edje, "service.book.name.str", name);
}

static void
gocomics_display (void)
{
    gocomics_set_comic_name();
    gocomics_set_comic_strip();
}

static void
gocomics_select_comic(void *data)
{
    Enna_Vfs_File *item = data;

    gocomics_date_set_current ();

    ENNA_FREE(mod->comic_name);
    mod->comic_name = strdup (item->label);
    ENNA_FREE(mod->comic_id);
    mod->comic_id = strdup (item->uri);

    gocomics_display();
}

static void
gocomics_button_prev_clicked_cb(void *data, Evas_Object *obj, void *ev)
{
    gocomics_date_previous();
    gocomics_display();
}

static void
gocomics_button_next_clicked_cb(void *data, Evas_Object *obj, void *ev)
{
    gocomics_date_next();
    gocomics_display();
}

static void
gocomics_create_menu (void)
{
    Evas_Object *o;
    int i;

    /* Create List */
    ENNA_OBJECT_DEL(mod->list);
    o = enna_list_add(enna->evas);

    for (i = 0; gocomics_list_map[i].name; i++)
    {
        Enna_Vfs_File *item;

        item = calloc(1, sizeof(Enna_Vfs_File));
        item->label   = (char *) gocomics_list_map[i].name;
        item->uri     = (char *) gocomics_list_map[i].id;
        item->is_menu = 1;
        enna_list_file_append(o, item, gocomics_select_comic, (void *) item);
    }

    enna_list_select_nth(o, 0);
    mod->list = o;
    edje_object_part_swallow(mod->edje, "service.browser.swallow", o);
}

/****************************************************************************/
/*                         Private Service API                              */
/****************************************************************************/

static Eina_Bool
bs_gocomics_event (Evas_Object *edje, enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed gocomics : %d", event);

    switch (event)
    {
    case ENNA_INPUT_LEFT:
        gocomics_button_prev_clicked_cb(NULL, NULL, NULL);
        return ENNA_EVENT_BLOCK;
    case ENNA_INPUT_RIGHT:
        gocomics_button_next_clicked_cb(NULL, NULL, NULL);
        return ENNA_EVENT_BLOCK;
    case ENNA_INPUT_BACK:
        enna_content_hide();
        return ENNA_EVENT_CONTINUE;
    case ENNA_INPUT_OK:
        gocomics_select_comic(enna_list_selected_data_get(mod->list));
        return ENNA_EVENT_BLOCK;
    default:
        return enna_list_input_feed(mod->list, event);
    }
}

static void
bs_gocomics_show (Evas_Object *edje)
{
    char dst[1024] = { 0 };

    mod = calloc (1, sizeof(BookStore_Service_GoComics));

    /* create comic strips download destination path */
    snprintf(dst, sizeof(dst), "%s/%s",
             enna_cache_home_get(), GOCOMICS_PATH);
    if (!ecore_file_is_dir(dst))
        ecore_file_mkdir(dst);

    mod->path = strdup (dst);
    mod->url = url_new();
    mod->edje = edje;

    gocomics_set_comic_name();
    gocomics_create_menu();
}

static void
bs_gocomics_hide (Evas_Object *edje)
{
    url_free(mod->url);
    ENNA_FREE(mod->comic_name);
    ENNA_FREE(mod->comic_id);
    ENNA_OBJECT_DEL(mod->list);
    ENNA_FREE(mod->path);
    ENNA_FREE(mod);
}

/****************************************************************************/
/*                         Public Service API                               */
/****************************************************************************/

BookStore_Service bs_gocomics = {
    "GoComics",
    "background/gocomics",
    "icon/gocomics",
    bs_gocomics_show,
    bs_gocomics_hide,
    bs_gocomics_event,
    gocomics_button_prev_clicked_cb,
    gocomics_button_next_clicked_cb,
};
