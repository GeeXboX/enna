/* Interface */

#include "enna.h"

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#define SHOUTCAST_URL      "http://www.shoutcast.com"

#define SHOUTCAST_GENRE    "shoutcast://"
#define SHOUTCAST_LIST     "http://www.shoutcast.com/sbin/newxml.phtml"

#define SHOUTCAST_STATION  "http://www.shoutcast.com/sbin/newxml.phtml?genre="

#define MAX_URL 1024

static void           _class_init();
static void           _class_shutdown();
static Enna_Vfs_File *_class_vfs_get(void);

static Evas_List     *_class_browse_up(const char *path);
static Evas_List     *_class_browse_down(void);
static Enna_Vfs_File *_class_vfs_get(void);

static int            em_init(Enna_Module *em);
static int            em_shutdown(Enna_Module *em);

typedef struct curl_data_s {
  char *buffer;
  size_t size;
} curl_data_t;

typedef struct Enna_Module_Music_s {
  Evas *e;
  Enna_Module *em;
  CURL *curl;
} Enna_Module_Music;

static Enna_Module_Music *mod;

EAPI Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    "shoutcast"
};

static Enna_Class_Vfs class_shoutcast =
{
    "shoutcast",
    1,
    "SHOUTcast Streaming",
    NULL,
    "icon/shoutcast",
    {
        _class_init,
        _class_shutdown,
	_class_browse_up,
	_class_browse_down,
	_class_vfs_get,
    },
};

static xmlNode *
get_node_xml_tree (xmlNode *root, const char *prop)
{
  xmlNode *n, *children_node;
  
  for (n = root; n; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE
        && xmlStrcmp ((unsigned char *) prop, n->name) == 0)
      return n;
    
    if ((children_node = get_node_xml_tree (n->children, prop)))
      return children_node;
  }
  
  return NULL;
}

static size_t
http_info_get (void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  curl_data_t *mem = (curl_data_t *) data;

  mem->buffer = realloc (mem->buffer, mem->size + realsize + 1);
  if (mem->buffer)
  {
    memcpy (&(mem->buffer[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->buffer[mem->size] = 0;
  }
  
  return realsize;
}

static curl_data_t
http_get_data (CURL *curl, char *url)
{
  curl_data_t chunk;

  chunk.buffer = NULL; /* we expect realloc(NULL, size) to work */
  chunk.size = 0;      /* no data at this point */

  curl_easy_setopt (curl, CURLOPT_URL, url);
  curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, http_info_get);
  curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) &chunk);

  curl_easy_perform (curl);

  return chunk;
}

static Evas_List *
browse_list (void)
{
  curl_data_t chunk;
  xmlDocPtr doc;
  xmlNode *list, *n;
  Evas_List *files = NULL;

  chunk = http_get_data (mod->curl, SHOUTCAST_LIST);

  doc = xmlReadMemory (chunk.buffer, chunk.size,
                       NULL, NULL, 0);
  if (!doc)
    return NULL;

  list = get_node_xml_tree (xmlDocGetRootElement (doc), "genrelist");
  for (n = list->children; n; n = n->next)
  {
    Enna_Vfs_File *file;
    xmlChar *genre;
     
    if (!xmlHasProp (n, (xmlChar *) "name"))
      continue;

    genre = xmlGetProp (n, (xmlChar *) "name");
   
    file = calloc (1, sizeof (Enna_Vfs_File));
    file->uri = malloc (strlen (SHOUTCAST_GENRE)
                        + strlen ((char *) genre) + 1);
    sprintf (file->uri, "%s%s", SHOUTCAST_GENRE, genre);
    file->label = strdup ((char *) genre);
    file->icon_file = NULL;
    file->is_directory = 1;
    file->icon = "icon/webradio";
    files = evas_list_append (files, file);
  }
  
  free (chunk.buffer);
  xmlFreeDoc (doc);

  return files;
}

static Evas_List *
browse_by_genre (const char *path)
{
    curl_data_t chunk;
    xmlDocPtr doc;
    xmlNode *list, *n;
    char url[MAX_URL];
    Evas_List *files = NULL;
    xmlChar *tunein = NULL;
    const char *genre = path + strlen (SHOUTCAST_GENRE);

    memset (url, '\0', MAX_URL);
    snprintf (url, MAX_URL, "%s%s", SHOUTCAST_STATION, genre);
    chunk = http_get_data (mod->curl, url);

    doc = xmlReadMemory (chunk.buffer, chunk.size,
                         NULL, NULL, 0);
    if (!doc)
      return NULL;

    list = get_node_xml_tree (xmlDocGetRootElement (doc), "stationlist");
    for (n = list->children; n; n = n->next)
    {
      if (!n->name)
        continue;
      
      if (!xmlStrcmp (n->name, (xmlChar *) "tunein"))
      {
        if (!xmlHasProp (n, (xmlChar *) "base"))
          continue;
            
        tunein = xmlGetProp (n, (xmlChar *) "base");
        continue;
      }

      if (!xmlStrcmp (n->name, (xmlChar *) "station"))
      {
        Enna_Vfs_File *file;
        xmlChar *id, *name;
        char uri[MAX_URL];

        if (!xmlHasProp (n, (xmlChar *) "name")
            || !xmlHasProp (n, (xmlChar *) "id") || !tunein)
          continue;

        name = xmlGetProp (n, (xmlChar *) "name");
        id = xmlGetProp (n, (xmlChar *) "id");
        memset (uri, '\0', MAX_URL);
        snprintf (uri, MAX_URL, "%s%s?id=%s", SHOUTCAST_URL, tunein, id);

        file = calloc (1, sizeof (Enna_Vfs_File));
        file->uri = strdup (uri);
        file->label = strdup ((char *) name);
        file->icon = "icon/music";
        file->is_directory = 0;
        files = evas_list_append (files, file);
      }
    }
  
    free (chunk.buffer);
    xmlFreeDoc (doc);

    return files;
}

static Evas_List *
_class_browse_up (const char *path)
{
  if (!path)
    return browse_list ();

  if (strstr (path, SHOUTCAST_GENRE))
    return browse_by_genre (path);
  
  return NULL;
}

static Evas_List *
_class_browse_down (void)
{
  return browse_list ();
}

static Enna_Vfs_File *
_class_vfs_get (void)
{
   Enna_Vfs_File *f;

   f = calloc (1, sizeof (Enna_Vfs_File));
   f->uri = NULL;
   f->label = NULL;
   f->icon = (char *) evas_stringshare_add ("icon/music");
   f->is_directory = 1;

   return f;
}

static void
_class_init (int dummy)
{
  /* dummy */
}

static void
_class_shutdown (int dummy)
{
    /* dummy */
}

/* Module interface */

static int
em_init (Enna_Module *em)
{
  mod = calloc (1, sizeof (Enna_Module_Music));
  mod->em = em;
  em->mod = mod;
  
  curl_global_init (CURL_GLOBAL_DEFAULT);
  mod->curl = curl_easy_init ();

  enna_vfs_append ("shoutcast", ENNA_CAPS_MUSIC, &class_shoutcast);
  
  return 1;
}

static int
em_shutdown (Enna_Module *em)
{
  Enna_Module_Music *mod;

  mod = em->mod;;  

  if (mod->curl)
    curl_easy_cleanup (mod->curl);
  curl_global_cleanup ();
    
  return 1;
}

EAPI void
module_init (Enna_Module *em)
{
  if (!em)
    return;

  if (!em_init (em))
    return;
}

EAPI void
module_shutdown (Enna_Module *em)
{
  em_shutdown (em);
}
