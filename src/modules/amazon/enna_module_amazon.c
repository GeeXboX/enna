/* Interface */

#include "enna.h"

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#define DEBUG 1

#define MAX_URL_SIZE        1024
#define MAX_KEYWORD_SIZE    1024
#define MAX_BUF_LEN         65535

#define AMAZON_HOSTNAME     "webservices.amazon.com"
#define AMAZON_LICENSE_KEY  "0P1862RFDFSF4KYZQNG2"

#define AMAZON_SEARCH_MUSIC "http://%s/onca/xml?Service=AWSECommerceService&SubscriptionId=%s&Operation=ItemSearch&Keywords=%s&SearchIndex=Music"

#define AMAZON_SEARCH_COVER "http://%s/onca/xml?Service=AWSECommerceService&SubscriptionId=%s&Operation=ItemLookup&ItemId=%s&ResponseGroup=Images"

/*****************************************************************************/
/*                         Private Module API                                */
/*****************************************************************************/

static char * amazon_music_cover_get (const char *artist, const char *album);

static Enna_Class_CoverPlugin class =
  {
    "amazon",
    amazon_music_cover_get
  };

typedef struct _Enna_Module_Amazon {
  Evas *evas;
  Enna_Module *em;
  CURL *curl;
} Enna_Module_Amazon;

static Enna_Module_Amazon *mod;

EAPI Enna_Module_Api module_api =
  {
    ENNA_MODULE_VERSION,
    "amazon"
  };

static int
em_init (Enna_Module *em)
{
  mod = calloc (1, sizeof (Enna_Module_Amazon));

  mod->em = em;
  mod->evas = em->evas;
  
  curl_global_init (CURL_GLOBAL_DEFAULT);
  mod->curl = curl_easy_init ();
  enna_cover_plugin_register (&class);
  
  return 1;
}

static int
em_shutdown (Enna_Module *em)
{
  if (mod->curl)
    curl_easy_cleanup (mod->curl);
  curl_global_cleanup ();
  free (mod);
  
  return 1;
}

/*****************************************************************************/
/*                             Amazon Helpers                                */
/*****************************************************************************/

static void
url_escape_string (char *outbuf, const char *inbuf)
{
  unsigned char c, c1, c2;
  int i, len;

  len = strlen (inbuf);

  for  (i = 0; i < len; i++)
  {
    c = inbuf[i];
    if ((c == '%') && i < len - 2)
    { /* need 2 more characters */
      c1 = toupper (inbuf[i + 1]);
      c2 = toupper (inbuf[i + 2]);
      /* need uppercase chars */
    }
    else
    {
      c1 = 129;
      c2 = 129;
      /* not escaped chars */
    }

    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9') || (c >= 0x7f))
      *outbuf++ = c;
    else if (c =='%'
             && ((c1 >= '0' && c1 <= '9') || (c1 >= 'A' && c1 <= 'F'))
             && ((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'F')))
    {
      /* check if part of an escape sequence */
      *outbuf++ = c; /* already escaped */
    }
    else
    {
      /* all others will be escaped */
      c1 = ((c & 0xf0) >> 4);
      c2 = (c & 0x0f);
      if (c1 < 10)
        c1 += '0';
      else
        c1 += 'A' - 10;
      if (c2 < 10)
        c2 += '0';
      else
        c2 += 'A' - 10;
      *outbuf++ = '%';
      *outbuf++ = c1;
      *outbuf++ = c2;
    }
  }
  *outbuf++='\0';
}

static xmlNode *
get_node_xml_tree (xmlNode *root, const char *prop)
{
  xmlNode *n, *children_node;
  
  for (n = root; n; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE
        && xmlStrcmp ((unsigned char *) prop, n->name) == 0)
      return n;
    
    if ((children_node = get_node_xml_tree(n->children, prop)))
      return children_node;
  }
  
  return NULL;
}

static xmlChar *
get_prop_value_from_xml_tree (xmlNode *root, const char *prop)
{
  xmlNode *node;
  
  node = get_node_xml_tree (root, prop);
  if (!node)
    return NULL;
  
  if (xmlNodeGetContent (node))
    return xmlNodeGetContent (node);
  
  return NULL;
}

static int
curl_http_buffer_get (void *buffer, size_t size, size_t nmemb, void *stream)
{
  char *msg = (char *) stream;

  if (!msg)
    return -1;

  strncat (msg, buffer, (size * nmemb));

  return (size * nmemb);
}

static char *
curl_http_get (char *url)
{
  char *buf;

  buf = malloc (MAX_BUF_LEN);
  memset (buf, '\0', MAX_BUF_LEN);
  
  curl_easy_setopt (mod->curl, CURLOPT_URL, url);
  curl_easy_setopt (mod->curl, CURLOPT_WRITEFUNCTION, curl_http_buffer_get);
  curl_easy_setopt (mod->curl, CURLOPT_WRITEDATA, buf);

  return (curl_easy_perform (mod->curl) == CURLE_OK) ? buf : NULL;
}

static char *
amazon_cover_album_get (const char *artist, const char *album)
{
  char *buf, *cover;
  char url[MAX_URL_SIZE];
  char keywords[MAX_KEYWORD_SIZE];
  char escaped_keywords[2*MAX_KEYWORD_SIZE];
  
  xmlDocPtr doc;
  xmlNode *img;
  xmlChar *asin, *cover_url;

  if (!artist || !album)
    return NULL;
  
  /* 1. Format the keywords */
  memset (keywords, '\0', MAX_KEYWORD_SIZE);
  memset (escaped_keywords, '\0', 2 * MAX_KEYWORD_SIZE);
  sprintf (keywords, "%s %s", artist, album);
  url_escape_string (escaped_keywords, keywords);

  /* 2. Prepare Amazon WebService URL for Music Search */
  memset (url, '\0', MAX_URL_SIZE);
  snprintf (url, MAX_URL_SIZE, AMAZON_SEARCH_MUSIC,
            AMAZON_HOSTNAME, AMAZON_LICENSE_KEY, escaped_keywords);

#ifdef DEBUG
  printf ("Music Search Request: %s\n", url);
#endif

  /* 3. Perform request */
  buf = curl_http_get (url);
  if (!buf)
    return NULL;

#ifdef DEBUG
  printf ("Music Search Reply: %s\n", buf);
#endif

  /* 4. Parse the answer to get ASIN value */
  doc = xmlReadMemory (buf, strlen (buf), "noname.xml", NULL, 0);
  free (buf);
  
  if (!doc)
    return NULL;

  asin = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc), "ASIN");
  xmlFreeDoc (doc);

  if (!asin)
  {
    printf ("Amazon: Unable to find the item \"%s - %s\"\n", artist, album);
    return NULL;
  }

#ifdef DEBUG
  printf ("Found Amazon ASIN: %s\n", asin);
#endif

  /* 5. Prepare Amazon WebService URL for Cover Search */
  memset (url, '\0', MAX_URL_SIZE);
  snprintf (url, MAX_URL_SIZE, AMAZON_SEARCH_COVER,
            AMAZON_HOSTNAME, AMAZON_LICENSE_KEY, asin);
  xmlFree (asin);

#ifdef DEBUG
  printf ("Cover Search Request: %s\n", url);
#endif

  /* 6. Perform request */
  buf = curl_http_get (url);
  if (!buf)
    return NULL;

#ifdef DEBUG
  printf ("Cover Search Reply: %s\n", buf);
#endif
  
  /* 7. Parse the answer to get cover URL */
  doc = xmlReadMemory (buf, strlen (buf), "noname.xml", NULL, 0);
  free (buf);
  
  if (!doc)
    return NULL;

  img = get_node_xml_tree (xmlDocGetRootElement (doc), "LargeImage");
  if (!img)
    img = get_node_xml_tree (xmlDocGetRootElement (doc), "MediumImage");
  if (!img)
    img = get_node_xml_tree (xmlDocGetRootElement (doc), "SmallImage");

  if (!img)
  {
    xmlFreeDoc (doc);
    return NULL;
  }

  cover_url = get_prop_value_from_xml_tree (img, "URL");
  if (!cover_url)
  {
    printf ("Amazon: Unable to find the cover for %s - %s\n", artist, album);
    xmlFreeDoc (doc);
    return NULL;
  }

  xmlFreeDoc (doc);
  
  /* 8. Download cover and save to disk */
  cover = malloc (MAX_URL_SIZE);
  snprintf (cover, MAX_URL_SIZE,
            "%s/.enna/covers/%s-%s.png",
            enna_util_user_home_get (), artist, album);

  printf ("Saving %s to %s\n", cover_url, cover);
  ecore_file_download ((const char *) cover_url, cover, NULL, NULL, NULL);
  xmlFree (cover_url);
  
  return cover;
}

static char *
amazon_music_cover_get (const char *artist, const char *album)
{
  return amazon_cover_album_get (artist, album);
}

/*****************************************************************************/
/*                          Public Module API                                */
/*****************************************************************************/

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
