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

#include "enna.h"
#include "xml_utils.h"

xmlDocPtr
get_xml_doc_from_memory (char *buffer)
{
    xmlDocPtr doc = NULL;

    if (!buffer)
        return NULL;

    doc = xmlReadMemory (buffer, strlen (buffer), NULL, NULL,
                         XML_PARSE_RECOVER |
                         XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
    return doc;
}

xmlNode *
get_node_xml_tree(xmlNode *root, const char *prop)
{
    xmlNode *n, *children_node;

    for (n = root; n; n = n->next)
    {
        if (n->type == XML_ELEMENT_NODE && xmlStrcmp((unsigned char *) prop,
                n->name) == 0)
            return n;

        if ((children_node = get_node_xml_tree(n->children, prop)))
            return children_node;
    }

    return NULL;
}

xmlChar *
get_prop_value_from_xml_tree(xmlNode *root, const char *prop)
{
    xmlNode *node;

    node = get_node_xml_tree(root, prop);
    if (!node)
        return NULL;

        return xmlNodeGetContent(node);
}

xmlNode *
get_node_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                const char *attr_name,
                                const char *attr_value)
{
    xmlNode *n, *node;
    xmlAttr *attr;

    node = get_node_xml_tree (root, prop);
    if (!node)
        return NULL;

    for (n = node; n; n = n->next)
    {
        xmlChar *content;

        attr = n->properties;
        if (!attr || !attr->children)
            continue;

        if (xmlStrcmp ((unsigned char *) attr_name, attr->name) != 0)
            continue;

        content = xmlNodeGetContent (attr->children);
        if (!content)
            continue;

        if (xmlStrcmp ((unsigned char *) attr_value, content) != 0)
        {
            xmlFree (content);
            continue;
        }

        xmlFree (content);
        return n;
    }

    return NULL;
}

xmlChar *
get_prop_value_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                      const char *attr_name,
                                      const char *attr_value)
{
    xmlNode *n;

    n = get_node_from_xml_tree_by_attr (root, prop, attr_name, attr_value);
    return n ? xmlNodeGetContent (n) : NULL;
}

xmlChar *
get_attr_value_from_xml_tree (xmlNode *root,
                              const char *prop, const char *attr_name)
{
    xmlNode *n, *node;
    xmlAttr *attr;

    node = get_node_xml_tree (root, prop);
    if (!node)
        return NULL;

    for (n = node; n; n = n->next)
    {
        xmlChar *content;

        attr = n->properties;
        if (!attr || !attr->children)
            continue;

        if (xmlStrcmp ((unsigned char *) attr_name, attr->name) != 0)
            continue;

        content = xmlNodeGetContent (attr->children);
        if (content)
            return content;
    }

    return NULL;
}

xmlChar *
get_attr_value_from_node (xmlNode *node, const char *attr_name)
{
    xmlNode *n;
    xmlAttr *attr;

    if (!node || !attr_name)
        return NULL;

    for (n = node; n; n = n->next)
    {
        xmlChar *content;

        attr = n->properties;
        if (!attr || !attr->children)
            continue;

        if (xmlStrcmp ((unsigned char *) attr_name, attr->name) != 0)
            continue;

        content = xmlNodeGetContent (attr->children);
        if (content)
            return content;
    }

    return NULL;
}

xmlXPathObjectPtr
get_xnodes_from_xml_tree (xmlDocPtr doc, xmlChar *xpath)
{
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);

    if (!context)
        return NULL;

    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);

    if (!result)
        return NULL;

    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
    {
        xmlXPathFreeObject(result);
        return NULL;
    }

    return result;
}

int
xml_search_str (xmlNode *n, const char *node, char **str)
{
    xmlChar *tmp;

    if (*str)
        return 1;

    tmp = get_prop_value_from_xml_tree (n, node);
    if (!tmp)
        return 1;

    *str = strdup ((char *) tmp);
    xmlFree (tmp);

    return 0;
}

int
xml_search_int (xmlNode *n, const char *node, int *val)
{
    xmlChar *tmp;

    if (*val)
        return 1;

    tmp = get_prop_value_from_xml_tree (n, node);
    if (!tmp)
        return 1;

    *val = atoi ((char *) tmp);
    xmlFree (tmp);

    return 0;
}

int
xml_search_year (xmlNode *n, const char *node, int *year)
{
    xmlChar *tmp;
    int r, y, m, d;

    if (*year)
        return 1;

    tmp = get_prop_value_from_xml_tree (n, node);
    if (!tmp)
        return 1;

    r = sscanf ((char *) tmp, "%d-%d-%d", &y, &m, &d);
    xmlFree (tmp);
    if (r == 3)
    {
        *year = y;
        return 0;
    }

    return 1;
}
