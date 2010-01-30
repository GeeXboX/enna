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
#include <Ecore_File.h>
#include <Elementary.h>
#include <Edje.h>

#include "enna.h"
#include "enna_config.h"
#include "logs.h"
#include "module.h"
#include "content.h"
#include "mainmenu.h"
#include "view_list.h"
#include "vfs.h"
#include "url_utils.h"
#include "xml_utils.h"
#include "utils.h"
#include "bookstore.h"
#include "bookstore_onemanga.h"
#include "xdg.h"

#define ENNA_MODULE_NAME                 "onemanga"

#define OM_URL                           "http://www.onemanga.com"
#define OM_DIRECTORY                     "directory/pop/"

#define OM_PATH                          "onemanga"

#define OM_NEEDLE_START \
  "<select name=\"page\" id=\"id_page_select\" class=\"page-select\">"
#define OM_NEEDLE_END                    "</select>"

#define OM_PAGE_START                    "<img class=\"manga-page\" src=\""
#define OM_PAGE_END                      "\""

typedef struct manga_s {
    char *name;
    char *uri;
    int chapters;
} manga_t;

typedef struct _BookStore_Service_OneManga {
    url_t url;
    Eina_List *manga_list;
    int main_menu;
    manga_t *current_manga;
    int current_chapter;
    Eina_List *page_list;

    char *path;
    Evas_Object *edje;
    Evas_Object *list;
} BookStore_Service_OneManga;

static BookStore_Service_OneManga *mod;

/****************************************************************************/
/*                         OneManga.com Module API                          */
/****************************************************************************/

static manga_t *
manga_new (const char *name, const char *uri, int chapters)
{
    manga_t *m;

    if (!name || !uri)
        return NULL;

    m           = calloc(1, sizeof (manga_t));
    m->name     = strdup(name);
    m->uri      = strdup(uri);
    m->chapters = chapters;

    return m;
}

static void
manga_free (manga_t *m)
{
    if (!m)
        return;

    ENNA_FREE(m->name);
    ENNA_FREE(m->uri);
    ENNA_FREE(m);
}

static void
om_page_prev (void)
{
    if (mod->page_list && mod->page_list->prev)
        mod->page_list = mod->page_list->prev;
}

static void
om_page_next (void)
{
    if (mod->page_list && mod->page_list->next)
        mod->page_list = mod->page_list->next;
}

static void
om_set_manga_page (void)
{
    char img_dst[1024] = { 0 };
    char query[1024] = { 0 };
    url_data_t data = { 0 };
    char *ptr_start, *ptr_end;
    char *img_src = NULL;
    char pg[32] = { 0 };
    xmlNode *n, *n2;
    xmlDocPtr doc;
    manga_t *m;

    m = mod->current_manga;
    if (!m)
        return;

    /* determine the requested page */
    snprintf(pg, sizeof(pg), "%s", mod->page_list ?
             (char *) eina_list_data_get(mod->page_list) : "01");

    /* get expected destination file name */
    snprintf(img_dst, sizeof(img_dst), "%s/%s_%d_%s",
             mod->path, m->uri, mod->current_chapter, pg);

    /* no need to perform a web request for an already existing file */
    if (mod->page_list && ecore_file_exists(img_dst))
        goto page_show;

    /* compute the requested URL */
    snprintf(query, sizeof(query), "%s/%s/%d/%s",
             OM_URL, m->uri, mod->current_chapter, pg);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Query Request: %d", query);

    /* perform request */
    data = url_get_data(mod->url, query);
    if (!data.buffer)
        goto err_url;

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Query Result: %d", data.buffer);

    /* find URL for the scanned image */
    ptr_start = strstr(data.buffer, OM_PAGE_START);
    if (!ptr_start)
        goto err_url_img;

    ptr_start += strlen(OM_PAGE_START);
    ptr_end = strstr(ptr_start, OM_PAGE_END);
    if (!ptr_end)
        goto err_url_img;

    img_src = strndup(ptr_start, strlen(ptr_start) - strlen(ptr_end));

    /* find list of available pages */
    if (!mod->page_list)
    {
        char *ctx_pages;

        ptr_start = strstr(data.buffer, OM_NEEDLE_START);
        if (!ptr_start)
            goto err_pages;

        ptr_end = strstr(ptr_start, OM_NEEDLE_END);
        if (!ptr_end)
            goto err_pages;
        ptr_end += strlen(OM_NEEDLE_END);
        ctx_pages = strndup(ptr_start, strlen(ptr_start) - strlen(ptr_end));

        doc = get_xml_doc_from_memory(ctx_pages);
        if (!doc)
            goto err_doc;

        n = xmlDocGetRootElement(doc);
        for (n2 = n->children; n2; n2 = n2->next)
        {
            if (n2->type == XML_ELEMENT_NODE)
            {
                xmlChar *c;

                c = xmlNodeGetContent(n2);
                if (!c)
                    continue;

                mod->page_list =
                    eina_list_append (mod->page_list, strdup ((char *) c));
            }
        }
        om_page_next();

        if (doc)
        {
            xmlFreeDoc(doc);
            doc = NULL;
        }
    err_doc:
        ENNA_FREE(ctx_pages);
    }

    /* download and save the manga page file */
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Saving Manga Page URL %s to %s", img_src, img_dst);
    url_save_to_disk(mod->url, img_src, img_dst);

err_pages:
    ENNA_FREE(img_src);
err_url_img:
    ENNA_FREE(data.buffer);
err_url:
page_show:
    /* display the manga page */
    bs_service_page_show(img_dst);
}

static void
om_set_manga_name (void)
{
    manga_t *m;

    m = mod->current_manga;
    if (!m || !m->name)
    {
        edje_object_part_text_set(mod->edje,
                                  "service.book.name.str", "OneManga");
        return;
    }

    if (mod->current_chapter)
    {
        char name[128] = { 0 };

        snprintf(name, sizeof(name), "%s - %d - %s",
                 m->name, mod->current_chapter, mod->page_list ?
                 (char *) eina_list_data_get(mod->page_list) : "01");
        edje_object_part_text_set(mod->edje, "service.book.name.str", name);
        return;
    }
    else
        edje_object_part_text_set(mod->edje, "service.book.name.str", m->name);
}

static void
om_display (void)
{
    om_set_manga_name();
    om_set_manga_page();
}

static void
om_page_list_free (void)
{
    Eina_List *l;
    char *pg;

    if (!mod->page_list)
        return;

    EINA_LIST_FOREACH(mod->page_list, l, pg)
        ENNA_FREE(pg);
    eina_list_free(mod->page_list);
    mod->page_list = NULL;
}

static void
om_reset_current_settings (void)
{
    mod->current_manga   = NULL;
    mod->current_chapter = 0;

    om_page_list_free();
    bs_service_page_show(NULL);
    om_display();
}

static void
om_select_chapter(void *data)
{
    Enna_Vfs_File *item = data;

    mod->current_chapter = atoi(item->uri);
    om_page_list_free();

    om_display();
}

static void
om_create_menu_manga_chapters_list (manga_t *m)
{
    Evas_Object *o;
    int i;

    mod->main_menu = 0;
    ENNA_OBJECT_DEL(mod->list);
    o = enna_list_add(enna->evas);

    for (i = m->chapters; i; i--)
    {
        Enna_Vfs_File *item;
        char name[64] = { 0 };
        char chapter[8] = { 0 };

        snprintf(name, sizeof(name), "Chapter %d", i);
        snprintf(chapter, sizeof(chapter), "%d", i);

        item = calloc(1, sizeof(Enna_Vfs_File));
        item->label   = strdup(name);
        item->uri     = strdup(chapter);
        item->is_menu = 1;

        enna_list_file_append(o, item, om_select_chapter, (void *) item);
    }

    enna_list_select_nth(o, 0);
    mod->list = o;
    edje_object_part_swallow(mod->edje, "service.browser.swallow", o);
}

static void
om_select_manga(void *data)
{
    Enna_Vfs_File *item = data;
    Eina_List *l;
    manga_t *m;

    if (!item || !item->label)
        return;

    EINA_LIST_FOREACH(mod->manga_list, l, m)
    {
        if (!strcmp(m->name, item->label))
        {
            mod->current_manga = m;
            om_set_manga_name();
            om_create_menu_manga_chapters_list(m);
            break;
        }
    }
}

static char *
om_parse_manga_list_item_subject (xmlNode *n)
{
  xmlNode *n2;

  if (!n)
    return NULL;

  n2 = n->children;
  if (!n2)
    return NULL;

  if (xmlStrcmp(n2->name, (unsigned char *) "a") != 0)
    return NULL;

  return xmlNodeGetContent(n2) ?
    strdup ((char *) xmlNodeGetContent(n2)) : NULL;
}

static char *
om_parse_manga_list_item_chapter (xmlNode *n)
{
  xmlNode *n2;
  xmlAttr *attr;
  xmlChar *c;

  if (!n)
    return NULL;

  n2 = n->children;
  if (!n2)
    return NULL;

  if (xmlStrcmp(n2->name, (unsigned char *) "a") != 0)
    return NULL;

  attr = n2->properties;
  if (!attr || !attr->children)
    return NULL;

  c = xmlNodeGetContent(attr->children);
  if (!c)
    return NULL;;

  return c ? strdup((char *) c) : NULL;
}

static void
om_parse_manga_list_create_entry (const char *subject, const char *chapter)
{
    char *ptr, *url, *total;
    manga_t *m;

    if (!subject || !chapter)
        return;

    ptr = strstr (chapter + 1, "/");
    if (!ptr)
        return;

    url   = strndup(chapter + 1, strlen(chapter) - strlen(ptr) - 1);
    total = strndup(ptr + 1, strlen(ptr) - 2);

    m = manga_new(subject, url, atoi (total));
    if (m)
        mod->manga_list = eina_list_append(mod->manga_list, m);

    ENNA_FREE(url);
    ENNA_FREE(total);
}

static void
om_parse_manga_list_item (xmlNode *n)
{
  char *subject = NULL, *chapter = NULL;
  xmlNode *n2;

  if (!n)
    return;

  for (n2 = n->children; n2; n2 = n2->next)
  {
    xmlAttr *attr;
    xmlChar *c;

    if (n2->type != XML_ELEMENT_NODE)
      continue;

    attr = n2->properties;
    if (!attr || !attr->children)
      continue;

    c = xmlNodeGetContent(attr->children);
    if (!c)
      continue;

    if (!xmlStrcmp (c, (unsigned char *) "ch-subject"))
        subject = om_parse_manga_list_item_subject(n2);
    else if (!xmlStrcmp (c, (unsigned char *) "ch-chapter"))
        chapter = om_parse_manga_list_item_chapter(n2);

    xmlFree (c);
  }

  om_parse_manga_list_create_entry(subject, chapter);

  ENNA_FREE(subject);
  ENNA_FREE(chapter);
}

static void
om_parse_manga_list (void)
{
    url_data_t data;
    xmlDocPtr doc;
    xmlNode *n, *n2;
    char url[128] = { 0 };

    snprintf (url, sizeof(url), "%s/%s", OM_URL, OM_DIRECTORY);
    data = url_get_data(mod->url, url);
    if (!data.buffer)
        return;

    doc = get_xml_doc_from_memory(data.buffer);
    if (!doc)
        goto err_doc;

    n = get_node_from_xml_tree_by_attr(xmlDocGetRootElement(doc),
                                       "table", "class", "ch-table");
    if (!n)
        goto err_node;

    for (n2 = n; n2; n2 = n2->next)
    {
        xmlNode *n3;

        for (n3 = n2->children; n3; n3 = n3->next)
        {
          if (n3->type == XML_ELEMENT_NODE)
          {
            xmlAttr *attr;
            xmlChar *content;

            attr = n3->properties;
            if (!attr || !attr->children)
              continue;

            content = xmlNodeGetContent(attr->children);
            if (!content)
              continue;

            om_parse_manga_list_item(n3);
            xmlFree(content);
          }

        }
      }

err_node:
    if (doc)
    {
        xmlFreeDoc(doc);
        doc = NULL;
    }
err_doc:
    ENNA_FREE(data.buffer);
}

static void
om_create_menu_list (void)
{
    Evas_Object *o;
    Eina_List *l;
    manga_t *m;

    mod->main_menu = 1;
    om_reset_current_settings();

    ENNA_OBJECT_DEL(mod->list);
    o = enna_list_add(enna->evas);

    EINA_LIST_FOREACH(mod->manga_list, l, m)
    {
        Enna_Vfs_File *item;

        item = calloc(1, sizeof(Enna_Vfs_File));
        item->label   = strdup(m->name);
        item->is_menu = 1;
        enna_list_file_append(o, item, om_select_manga, (void *) item);
    }

    enna_list_select_nth(o, 0);
    mod->list = o;
    edje_object_part_swallow(mod->edje, "service.browser.swallow", o);
}

static void
om_button_prev_clicked_cb(void *data, Evas_Object *obj, void *ev)
{
    om_page_prev();
    om_display();
}

static void
om_button_next_clicked_cb(void *data, Evas_Object *obj, void *ev)
{
    om_page_next();
    om_display();
}

/****************************************************************************/
/*                         Private Service API                              */
/****************************************************************************/

static Eina_Bool
bs_onemanga_event (Evas_Object *edje, enna_input event)
{
    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Key pressed onemanga : %d", event);

    switch (event)
    {
    case ENNA_INPUT_LEFT:
        om_button_prev_clicked_cb(NULL, NULL, NULL);
        return ENNA_EVENT_BLOCK;
    case ENNA_INPUT_RIGHT:
        om_button_next_clicked_cb(NULL, NULL, NULL);
        return ENNA_EVENT_BLOCK;
    case ENNA_INPUT_EXIT:
        if (mod->main_menu)
        {
            enna_content_hide();
            return ENNA_EVENT_CONTINUE;
        }
        else
        {
            om_create_menu_list();
            return ENNA_EVENT_BLOCK;
        }
        break;
    case ENNA_INPUT_OK:
        if (mod->main_menu)
            om_select_manga(enna_list_selected_data_get(mod->list));
        else
            om_select_chapter(enna_list_selected_data_get(mod->list));
        return ENNA_EVENT_BLOCK;
    default:
        return enna_list_input_feed(mod->list, event);
    }
}

static void
bs_onemanga_show (Evas_Object *edje)
{
   char dst[1024] = { 0 };

    mod = calloc (1, sizeof(BookStore_Service_OneManga));

    /* create manga pages download destination path */
    snprintf(dst, sizeof(dst), "%s/%s",
             enna_cache_home_get(), OM_PATH);
    if (!ecore_file_is_dir(dst))
        ecore_file_mkdir(dst);

    mod->path = strdup(dst);
    mod->main_menu = 1;
    mod->url = url_new();
    mod->edje = edje;

    /* parse manga list, once for all */
    if (!mod->manga_list)
    {
        om_parse_manga_list();
        om_create_menu_list();
    }
}

static void
bs_onemanga_hide (Evas_Object *edje)
{
    Eina_List *l;
    manga_t *m;

    url_free(mod->url);
    ENNA_OBJECT_DEL(mod->list);

    EINA_LIST_FOREACH(mod->manga_list, l, m)
        manga_free(m);
    eina_list_free(mod->manga_list);
    mod->manga_list = NULL;

    ENNA_FREE(mod->path);
    ENNA_FREE(mod);
}

/****************************************************************************/
/*                         Public Service API                               */
/****************************************************************************/

BookStore_Service bs_onemanga = {
    "OneManga",
    "background/onemanga",
    "icon/onemanga",
    bs_onemanga_show,
    bs_onemanga_hide,
    bs_onemanga_event,
    om_button_prev_clicked_cb,
    om_button_next_clicked_cb,
};
