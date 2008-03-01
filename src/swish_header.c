/*
 * This file is part of libswish3
 * Copyright (C) 2008 Peter Karman
 *
 *  libswish3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libswish3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libswish3; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* check a swish3 header file for correct syntax */

#include <libxml/xmlstring.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>
#include <libxml/hash.h>
#include <libxml/xmlreader.h>
#include "libswish3.h"

extern int      SWISH_DEBUG;

typedef struct {
    boolean         isprops;
    boolean         ismetas;
    const xmlChar   *parent_name;
    swish_Config    *config;
} configmaker;

static void
do_property_aliases(
    xmlChar *str,
    configmaker *c,
    swish_Property *prop
)
{
    swish_StringList * strlist;
    int i;

    strlist = swish_make_stringlist(str);

    /* loop over each alias and create a Property for each,
       setting alias_for to prop->name
    */
    for (i=0; i < strlist->n; i++) {
        
        if (! swish_hash_exists( c->config->properties, strlist->word[i]) ) {
            swish_Property *newprop = swish_init_property( swish_str_tolower(strlist->word[i]) );
            newprop->ref_cnt++;
            newprop->alias_for = swish_xstrdup( prop->name );
            swish_hash_add( c->config->properties, newprop->name, newprop );
            swish_debug_property(newprop);
        } 
        else {
            SWISH_CROAK("Cannot alias Property %s to %s because %s is already a real Property",
                strlist->word[i], prop->name, strlist->word[i]);
        }
   

    }
    
    swish_free_stringlist(strlist);

}


static void
do_property_attr(
    const xmlChar *attr, 
    const xmlChar *attr_val, 
    swish_Property *prop,
    configmaker *c
)
{

    if (xmlStrEqual(attr, (xmlChar*)"ignore_case")) {
        prop->ignore_case = (boolean)strtol((char*)attr_val, (char**)NULL, 10);
    }
    else if (xmlStrEqual(attr, (xmlChar*)"max")) {
        prop->max = (int)strtol((char*)attr_val, (char**)NULL, 10);
    }
    else if (xmlStrEqual(attr, (xmlChar*)"verbatim")) {
        prop->verbatim = (boolean)strtol((char*)attr_val, (char**)NULL, 10);
    }
    else if (xmlStrEqual(attr, (xmlChar*)"sort")) {
        prop->sort = (boolean)strtol((char*)attr_val, (char**)NULL, 10);
    }
    else if (xmlStrEqual(attr, (xmlChar*)"type")) {
        if (xmlStrEqual(attr_val, (xmlChar*)"int")) {
            prop->type = SWISH_PROP_INT;
        }
        else if (xmlStrEqual(attr_val, (xmlChar*)"date")) {
            prop->type = SWISH_PROP_DATE;
        }
        else {
            prop->type = SWISH_PROP_STRING;
        }
    }
    else if (xmlStrEqual(attr, (xmlChar*)"alias")) {
        do_property_aliases( (xmlChar*)attr_val, c, prop );
    }

}

static void
do_property(xmlTextReaderPtr reader, configmaker *c) 
{
    xmlChar *value;
    swish_Property *prop;
    
    prop  = swish_init_property( swish_str_tolower( (xmlChar*)xmlTextReaderConstName(reader) ) );
    prop->ref_cnt++;
    value = NULL;
        
    if (    xmlTextReaderHasValue(reader) 
        &&  xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT ) {
        prop->name = swish_str_tolower( (xmlChar*)c->parent_name );
        do_property_aliases( xmlTextReaderValue(reader), c, prop );
        return;
    }
    
    
    if ( xmlTextReaderHasAttributes(reader) ) {
    
        xmlTextReaderMoveToFirstAttribute(reader);
        do_property_attr(
                xmlTextReaderConstName(reader),
                xmlTextReaderConstValue(reader),
                prop,
                c
                );
        
        while(xmlTextReaderMoveToNextAttribute(reader) == 1) {
            do_property_attr(
                xmlTextReaderConstName(reader),
                xmlTextReaderConstValue(reader),
                prop,
                c
                );
        }
    
    }
        
    if (!swish_hash_exists( c->config->properties, prop->name )) {
        swish_hash_add( c->config->properties, prop->name, prop );
    } else {
        swish_debug_config( c->config );
        SWISH_CROAK("Property %s is already defined", prop->name);
    }
    
    swish_debug_property(prop);
    
    if (value != NULL) {
        xmlFree(value);
    }
    
    c->parent_name = xmlTextReaderConstName(reader);

}

static void
process_node(xmlTextReaderPtr reader, configmaker *c) 
{
    const xmlChar *name, *value;
    xmlChar *attr, *attr_val;
    int i, depth, type, is_empty, has_value, has_attr;

    name = xmlTextReaderConstName(reader);
    if (name == NULL)
	name = BAD_CAST "--";

    value = xmlTextReaderConstValue(reader);
    
    if (    swish_str_all_ws((xmlChar*)value) 
        &&  xmlStrEqual(name, (xmlChar*)"#text")) 
    {
        return;
    }

    depth = xmlTextReaderDepth(reader);
    type  = xmlTextReaderNodeType(reader);
    is_empty    = xmlTextReaderIsEmptyElement(reader);
    has_value   = xmlTextReaderHasValue(reader);
    has_attr    = xmlTextReaderHasAttributes(reader);
    

    for (i=0; i < depth; i++) {
        printf(" ");
    }
    printf("> %s [type: %d  is_empty: %d  has_value: %d  has_attr: %d]",
        name, type, is_empty, has_value, has_attr
    );
    
    if (has_attr) {
    
        printf("\n");
        xmlTextReaderMoveToFirstAttribute(reader);
        attr = xmlTextReaderName(reader);
        attr_val = xmlTextReaderValue(reader);
        printf("  attr: %s = %s ", attr, attr_val);
        while(xmlTextReaderMoveToNextAttribute(reader) == 1) {
            attr = xmlTextReaderName(reader);
            attr_val = xmlTextReaderValue(reader);
            printf(" attr: %s = %s  ", attr, attr_val);
        }
        
    }
    
    xmlTextReaderMoveToElement(reader); // move back to element for do_property()

    
    if (value == NULL)
	    printf("\n");
    else {
        if (xmlStrlen(value) > 40)
            printf(" value: %.40s...\n", value);
        else
	    printf(" value: %s\n", value);
    }
    
    if (type == XML_READER_TYPE_END_ELEMENT) {
        if ( xmlStrEqual(name, (const xmlChar*)SWISH_PROP) ) {
            c->isprops = 0;
            return;
        }
        if ( xmlStrEqual(name, (const xmlChar*)SWISH_META) ) {
            c->ismetas = 0;
            return;
        }
    }
    else {

        if ( xmlStrEqual(name, (const xmlChar*)SWISH_PROP) ) {
            c->isprops = 1;
            return;
        }
        if ( xmlStrEqual(name, (const xmlChar*)SWISH_META) ) {
            c->ismetas = 1;
            return;
        }
        
    }
    
    if (c->isprops && type != XML_READER_TYPE_END_ELEMENT) {
        do_property(reader, c);
    }
    
    
    
}


static void
read_header(const char *filename, configmaker *c) 
{
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlReaderForFile(filename, NULL, 0);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            process_node(reader, c);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) {
            SWISH_CROAK("%s : failed to parse\n", filename);
        }
    } else {
        SWISH_CROAK("Unable to open %s\n", filename);
    }
}

int 
main(int argc, char **argv)
{
#ifdef LIBXML_READER_ENABLED
    int i;
    configmaker *c;
    
    c = swish_xmalloc(sizeof(configmaker));
    c->config = swish_init_config();
    c->isprops = 0;
    c->ismetas = 0;

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    for (i=1; i < argc; i++) {

        printf("config file %s\n", argv[i]);
        read_header( argv[i], c );

    }
    
    swish_debug_config( c->config );
    swish_free_config( c->config );
    swish_xfree( c );
    
    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return 0;
#else

    SWISH_CROAK("Your version of libxml2 is too old");
    return 1;
#endif

}
