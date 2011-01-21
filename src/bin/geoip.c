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

#include <string.h>

#include <Ecore_Con.h>

#include "enna.h"
#include "enna_config.h"
#include "xml_utils.h"
#include "url_utils.h"
#include "utils.h"
#include "logs.h"
#include "geoip.h"

#define ENNA_MODULE_NAME      "geoip"

#define GEOIP_QUERY           "http://www.ipinfodb.com/ip_query.php"
#define MAX_URL_SIZE          1024

int ENNA_EVENT_GEO_LOC_DETECTED;

static Eina_Bool
url_data_cb(void *data __UNUSED__, int ev_type __UNUSED__, void *ev) 
{ 
    xmlDocPtr doc = NULL;
    xmlNode *n;
    xmlChar *tmp;
    Ecore_Con_Event_Url_Data *urldata = ev;
    Geo *geo = NULL;
    
    ecore_con_url_data_get(urldata->url_con); 

    if (!urldata->size)
        goto error;

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Search Reply: %s", urldata->data);

    /* parse the XML answer */
    doc = get_xml_doc_from_memory((char*)urldata->data);
 
    if (!doc)
        goto error;

    n = xmlDocGetRootElement(doc);

    /* check for existing city */
    tmp = get_prop_value_from_xml_tree(n, "Status");
    if (!tmp || xmlStrcmp(tmp, (unsigned char *) "OK"))
    {
        enna_log(ENNA_MSG_WARNING, ENNA_MODULE_NAME,
                 "Error returned by website.");
        if (tmp)
            xmlFree(tmp);
        goto error;
    }
    xmlFree(tmp);

    geo = calloc(1, sizeof(Geo));

    tmp = get_prop_value_from_xml_tree(n, "Latitude");
    if (tmp)
    {
        geo->latitude = enna_util_atof((char *) tmp);
        xmlFree(tmp);
    }

    tmp = get_prop_value_from_xml_tree(n, "Longitude");
    if (tmp)
    {
        geo->longitude = enna_util_atof((char *) tmp);
        xmlFree(tmp);
    }

    tmp = get_prop_value_from_xml_tree(n, "CountryCode");
    if (tmp)
    {
        geo->country = strdup((char *) tmp);
        xmlFree(tmp);
    }

    tmp = get_prop_value_from_xml_tree(n, "City");
    if (tmp)
    {
        geo->city = strdup((char *) tmp);
        xmlFree(tmp);
    }

    if (geo->city)
    {
        char res[256];
        if (geo->country)
            snprintf(res, sizeof(res), "%s, %s", geo->city, geo->country);
        else
            snprintf(res, sizeof(res), "%s", geo->city);
        geo->geo = strdup(res);

        enna_log(ENNA_MSG_INFO, ENNA_MODULE_NAME,
                 "Geolocalized in: %s (%f ; %f).", geo->geo, geo->latitude, geo->longitude);
    }

error:
    if (doc)
    {
        xmlFreeDoc(doc);
        doc = NULL;
    }

    enna->geo_loc = geo;
    ecore_event_add(ENNA_EVENT_GEO_LOC_DETECTED, NULL, NULL, NULL);

    return 0; 
} 


void
enna_get_geo_by_ip (void)
{
    Ecore_Con_Url *url;
    Ecore_Event_Handler *handler;

    ENNA_EVENT_GEO_LOC_DETECTED = ecore_event_type_new();
    ecore_con_url_init();
    url = ecore_con_url_new(GEOIP_QUERY);
    handler = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, 
                                      url_data_cb, NULL); 

    /* proceed with IP Geolocalisation request */
    ecore_con_url_send(url, NULL, 0, NULL);

    enna_log(ENNA_MSG_EVENT, ENNA_MODULE_NAME,
             "Search Request: %s", GEOIP_QUERY);

  

    return;
}

void
enna_geo_free (Geo *geo)
{
    if (!geo)
        return;

    if (geo->city)
        ENNA_FREE (geo->city);

    if (geo->country)
        ENNA_FREE (geo->country);

    if (geo->geo)
        ENNA_FREE (geo->geo); 

    ENNA_FREE (geo);
}
