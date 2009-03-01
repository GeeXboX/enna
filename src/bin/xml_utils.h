#ifndef __ENNA_XMLUTILS_H__
#define __ENNA_XMLUTILS_H__

#include <libxml/parser.h>
#include <libxml/tree.h>

xmlDocPtr get_xml_doc_from_memory (char *buffer);
xmlNode *get_node_xml_tree(xmlNode *root, const char *prop);
xmlChar *get_prop_value_from_xml_tree(xmlNode *root, const char *prop);
xmlChar *get_prop_value_from_xml_tree_by_attr (xmlNode *root, const char *prop,
                                               const char *attr_name,
                                               const char *attr_value);
xmlChar *get_attr_value_from_xml_tree (xmlNode *root, const char *prop,
                                       const char *attr_name);

#endif
