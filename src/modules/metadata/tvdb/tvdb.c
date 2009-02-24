/*
 * TheTVDB.com metadata plugin.
 *  See http://thetvdb.com/wiki/index.php?title=Programmers_API
 */

#include "enna.h"
#include "xml_utils.h"
#include "url_utils.h"

#define ENNA_MODULE_NAME        "metadata_tvdb"
#define ENNA_GRABBER_NAME       "tvdb"
#define ENNA_GRABBER_PRIORITY   3

#define PATH_BACKDROPS          "backdrops"
#define PATH_COVERS             "covers"

#define MAX_URL_SIZE            1024

#define TVDB_DEFAULT_LANGUAGE   "en"

#define TVDB_HOSTNAME           "thetvdb.com"
#define TVDB_IMAGES_HOSTNAME    "images.thetvdb.com"

#define TVDB_API_KEY            "29739E1D50A2ACA9"

#define TVDB_QUERY_SEARCH       "http://%s/api/GetSeries.php?seriesname=%s"
#define TVDB_QUERY_INFO         "http://%s/api/%s/series/%s/%s.xml"
#define TVDB_COVERS_URL         "http://%s/banners/%s"

typedef struct _Enna_Metadata_Tvdb {
    Evas *evas;
    Enna_Module *em;
    CURL *curl;
} Enna_Metadata_Tvdb;

static Enna_Metadata_Tvdb *mod;

/****************************************************************************/
/*                           TheTVDB.com Helpers                            */
/****************************************************************************/

static void
tvdb_parse (Enna_Metadata *meta)
{
    char url[MAX_URL_SIZE];
    char *escaped_keywords;
    url_data_t data;

    xmlDocPtr doc = NULL;
    xmlChar *tmp;
    xmlNode *n;

    if (!meta || !meta->keywords)
        return;

    /* TMDB only has sense on video files */
    if (meta->type != ENNA_METADATA_VIDEO)
        return;

    /* get HTTP compliant keywords */
    escaped_keywords = calloc (1, 2 * strlen (meta->keywords));
    url_escape_string (escaped_keywords, meta->keywords);

    /* proceed with TVDB search request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, TVDB_QUERY_SEARCH,
              TVDB_HOSTNAME, escaped_keywords);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Search Request: %s", url);

    data = url_get_data (mod->curl, url);
    if (data.status != CURLE_OK)
        goto error;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Search Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = xmlReadMemory (data.buffer, data.size, NULL, NULL, 0);
    free (data.buffer);
    if (!doc)
        goto error;

    /* check for a known DB entry */
    n = get_node_xml_tree (xmlDocGetRootElement (doc), "Series");
    if (!n)
    {
        enna_log (ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                  "Unable to find the item \"%s\"", escaped_keywords);
        goto error;
    }

    /* get TVDB serie ID */
    tmp = get_prop_value_from_xml_tree (n, "seriesid");
    if (!tmp)
        goto error;

    xmlFreeDoc (doc);
    doc = NULL;

    /* proceed with TVDB info request */
    memset (url, '\0', MAX_URL_SIZE);
    snprintf (url, MAX_URL_SIZE, TVDB_QUERY_INFO,
              TVDB_HOSTNAME, TVDB_API_KEY, tmp, TVDB_DEFAULT_LANGUAGE);
    xmlFree (tmp);

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME, "Info Request: %s", url);

    data = url_get_data (mod->curl, url);
    if (data.status != CURLE_OK)
        goto error;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Info Reply: %s", data.buffer);

    /* parse the XML answer */
    doc = xmlReadMemory (data.buffer, data.size, NULL, NULL, 0);
    free (data.buffer);
    if (!doc)
        goto error;

    /* fetch movie overview description */
    if (!meta->overview)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "Overview");
        if (tmp)
        {
            meta->overview = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch movie runtime (in minutes) */
    if (!meta->runtime)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "Runtime");
        if (tmp)
        {
            meta->runtime = atoi ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch first air date */
    if (!meta->year)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "FirstAired");
        if (tmp)
        {
            int r, y, m, d;
            r = sscanf ((char *) tmp, "%d-%d-%d", &y, &m, &d);
            xmlFree (tmp);
            if (r == 3)
                meta->year = y;
        }
    }

    /* fetch movie categories */
    if (!meta->categories)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "Genre");
        if (tmp)
        {
            meta->categories = strdup ((char *) tmp);
            xmlFree (tmp);
        }
    }

    /* fetch movie poster/cover */
    if (!meta->cover)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "poster");

        if (tmp)
        {
            char tmp_url[MAX_URL_SIZE];
            char cover[MAX_URL_SIZE];

            memset (tmp_url, '\0', MAX_URL_SIZE);
            snprintf (tmp_url, sizeof (tmp_url),
                      TVDB_COVERS_URL, TVDB_IMAGES_HOSTNAME, tmp);

            snprintf (cover, sizeof (cover), "%s/.enna/%s/%s.png",
                      enna_util_user_home_get(), PATH_COVERS, meta->md5);
            url_save_to_disk (mod->curl, tmp_url, cover);
            xmlFree (tmp);

            meta->cover = strdup (cover);
        }
    }

    /* fetch movie backdrop */
    if (!meta->backdrop)
    {
        tmp = get_prop_value_from_xml_tree (xmlDocGetRootElement (doc),
                                            "fanart");

        if (tmp)
        {
            char tmp_url[MAX_URL_SIZE];
            char cover[MAX_URL_SIZE];

            memset (tmp_url, '\0', MAX_URL_SIZE);
            snprintf (tmp_url, sizeof (tmp_url),
                      TVDB_COVERS_URL, TVDB_IMAGES_HOSTNAME, tmp);

            snprintf (cover, sizeof (cover), "%s/.enna/%s/%s.png",
                      enna_util_user_home_get(), PATH_BACKDROPS, meta->md5);
            url_save_to_disk (mod->curl, tmp_url, cover);
            xmlFree (tmp);

            meta->backdrop = strdup (cover);
        }
    }

 error:
    if (doc)
        xmlFreeDoc (doc);
    ENNA_FREE (escaped_keywords);
}

/****************************************************************************/
/*                        Private Module API                                */
/****************************************************************************/

static void
tvdb_grab (Enna_Metadata *meta, int caps)
{
    if (!meta || !meta->keywords)
        return;

    enna_log (ENNA_MSG_EVENT, ENNA_MODULE_NAME,
              "Grabbing info from %s", meta->uri);

    tvdb_parse (meta);
}

static Enna_Metadata_Grabber grabber = {
    ENNA_GRABBER_NAME,
    ENNA_GRABBER_PRIORITY,
    1,
    ENNA_GRABBER_CAP_COVER,
    tvdb_grab,
};

/****************************************************************************/
/*                         Public Module API                                */
/****************************************************************************/

Enna_Module_Api module_api =
{
    ENNA_MODULE_VERSION,
    ENNA_MODULE_METADATA,
    ENNA_MODULE_NAME
};

void
module_init (Enna_Module *em)
{
    if (!em)
        return;

    mod = calloc(1, sizeof (Enna_Metadata_Tvdb));

    mod->em = em;
    mod->evas = em->evas;

    curl_global_init (CURL_GLOBAL_DEFAULT);
    mod->curl = curl_easy_init ();
    enna_metadata_add_grabber (&grabber);
}

void
module_shutdown (Enna_Module *em)
{
    if (mod->curl)
        curl_easy_cleanup (mod->curl);
    curl_global_cleanup ();
    free (mod);
}
