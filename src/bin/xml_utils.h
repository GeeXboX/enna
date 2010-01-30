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

#ifndef XML_UTILS_H
#define XML_UTILS_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

xmlDocPtr get_xml_doc_from_memory (char *buffer);
xmlNode *get_node_xml_tree(xmlNode *root, const char *prop);
xmlChar *get_prop_value_from_xml_tree(xmlNode *root, const char *prop);
xmlNode *get_node_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                         const char *attr_name,
                                         const char *attr_value);
xmlChar *get_prop_value_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                               const char *attr_name,
                                               const char *attr_value);
xmlChar *get_attr_value_from_xml_tree (xmlNode *root, const char *prop,
                                       const char *attr_name);
xmlChar *get_attr_value_from_node (xmlNode *node, const char *attr_name);
xmlXPathObjectPtr get_xnodes_from_xml_tree (xmlDocPtr doc, xmlChar *xpath);
int xml_search_str (xmlNode *n, const char *node, char **str);
int xml_search_int (xmlNode *n, const char *node, int *val);
int xml_search_year (xmlNode *n, const char *node, int *year);

#endif /* XML_UTILS_H */
