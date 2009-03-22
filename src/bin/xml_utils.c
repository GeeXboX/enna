/*
 * Copyright (C) 2005-2009 The Enna Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software and its Copyright notices. In addition publicly
 * documented acknowledgment must be given that this software has been used if
 * no source code of this software is made available publicly. This includes
 * acknowledgments in either Copyright notices, Manuals, Publicity and
 * Marketing documents or any documentation provided with any product
 * containing this software. This License does not apply to any software that
 * links to the libraries provided by this software (statically or
 * dynamically), but only to the software provided.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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

xmlNode * get_node_xml_tree(xmlNode *root, const char *prop)
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

xmlChar * get_prop_value_from_xml_tree(xmlNode *root, const char *prop)
{
    xmlNode *node;

    node = get_node_xml_tree(root, prop);
    if (!node)
        return NULL;

    if (xmlNodeGetContent(node))
        return xmlNodeGetContent(node);

    return NULL;
}

xmlChar *
get_prop_value_from_xml_tree_by_attr (xmlNode *root, const char *prop,
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
            continue;
        
        return xmlNodeGetContent (n);
    }
    
    return NULL;
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
