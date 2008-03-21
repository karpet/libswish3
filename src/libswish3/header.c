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

/* read/write the swish.xml header file */

#include <libxml/xmlreader.h>
#include "libswish3.h"

extern int SWISH_DEBUG;

/* local struct to ease passing around flags/state */
typedef struct {
    boolean         isprops;
    boolean         ismetas;
    boolean         isindex;
    boolean         isparser;
    boolean         isalias;
    boolean         ismime;
    const xmlChar   *parent_name;
    swish_Config    *config;
    boolean         is_valid;
    unsigned int    prop_id;
    unsigned int    meta_id;
} headmaker;

static void
do_index(xmlTextReaderPtr reader, headmaker *h)
{
    SWISH_DEBUG_MSG("TODO index");
}

static void
do_parser(xmlTextReaderPtr reader, headmaker *h)
{
    SWISH_DEBUG_MSG("TODO parser");
}

static void
do_alias(xmlTextReaderPtr reader, headmaker *h)
{
    SWISH_DEBUG_MSG("TODO alias");
}

static void
do_mime(xmlTextReaderPtr reader, headmaker *h)
{
    SWISH_DEBUG_MSG("TODO mime");
}

static void
do_metaname_aliases(
    xmlChar *str,
    headmaker *h,
    swish_MetaName *meta
)
{
    swish_StringList * strlist;
    int i;

    strlist = swish_make_stringlist(str);

    /* loop over each alias and create a Property for each,
       setting alias_for to prop->name
    */
    for (i=0; i < strlist->n; i++) {
        
        if (! swish_hash_exists( h->config->metanames, strlist->word[i]) ) {
            swish_MetaName *newmeta;
            xmlChar *newname;
            newname = swish_str_tolower(strlist->word[i]);
            
            /* is this an existing metaname? pull it from hash and update */
            if (swish_hash_exists( h->config->metanames, newname )) {
                newmeta = xmlHashLookup( h->config->metanames, newname );
            }
            else {
                newmeta = swish_init_metaname( newname );
                newmeta->ref_cnt++;
                newmeta->id = h->meta_id++;
                swish_hash_add( h->config->metanames, newmeta->name, newmeta );
            }
            
            newmeta->alias_for = swish_xstrdup( meta->name );
            
            //swish_debug_metaname(newmeta);
        } 
        else {
            SWISH_CROAK("Cannot alias MetaName %s to %s because %s is already a real MetaName",
                strlist->word[i], meta->name, strlist->word[i]);
        }
   

    }
    
    swish_free_stringlist(strlist);
}


static void
do_metaname_attr(
    const xmlChar *attr, 
    const xmlChar *attr_val, 
    swish_MetaName *meta,
    headmaker *h
)
{

    if (xmlStrEqual(attr, (xmlChar*)"bias")) {
        meta->bias = (boolean)strtol((char*)attr_val, (char**)NULL, 10);
    }
    else if (xmlStrEqual(attr, (xmlChar*)"id")) {
        meta->id   = (int)strtol((char*)attr_val, (char**)NULL, 10);
    }
    else if (xmlStrEqual(attr, (xmlChar*)"alias")) {
        do_metaname_aliases( (xmlChar*)attr_val, h, meta );
    }
    else {
        SWISH_CROAK("Unknown MetaName attribute: %s", attr );
    }
}

static void
do_metaname(xmlTextReaderPtr reader, headmaker *h) 
{
    xmlChar *value;
    swish_MetaName *meta;
    
    meta  = swish_init_metaname( swish_str_tolower( (xmlChar*)xmlTextReaderConstName(reader) ) );
    meta->ref_cnt++;
    value = NULL;
        
    if (    xmlTextReaderHasValue(reader) 
        &&  xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT ) {
        meta->name = swish_str_tolower( (xmlChar*)h->parent_name );
        do_metaname_aliases( xmlTextReaderValue(reader), h, meta );
        return;
    }
    
    
    if ( xmlTextReaderHasAttributes(reader) ) {
    
        xmlTextReaderMoveToFirstAttribute(reader);
        do_metaname_attr(
                xmlTextReaderConstName(reader),
                xmlTextReaderConstValue(reader),
                meta,
                h
                );
        
        while(xmlTextReaderMoveToNextAttribute(reader) == 1) {
            do_metaname_attr(
                xmlTextReaderConstName(reader),
                xmlTextReaderConstValue(reader),
                meta,
                h
                );
        }
            
    }
    
    // must have an id
    if (!meta->id) {
        meta->id = h->meta_id++;
    }

        
    if (!swish_hash_exists( h->config->metanames, meta->name )) {
        swish_hash_add( h->config->metanames, meta->name, meta );
    } 
    else {
        SWISH_WARN("MetaName %s is already defined", meta->name); // TODO could be alias. how to check?
    }
    
    //swish_debug_metaname(meta);
    
    if (value != NULL) {
        xmlFree(value);
    }
    
    h->parent_name = xmlTextReaderConstName(reader);

}



static void
do_property_aliases(
    xmlChar *str,
    headmaker *h,
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
        
        if (! swish_hash_exists( h->config->properties, strlist->word[i]) ) {
            swish_Property *newprop = swish_init_property( swish_str_tolower(strlist->word[i]) );
            newprop->ref_cnt++;
            newprop->alias_for = swish_xstrdup( prop->name );
            newprop->id = h->prop_id++;
            swish_hash_add( h->config->properties, newprop->name, newprop );
            //swish_debug_property(newprop);
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
    headmaker *h
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
    else if (xmlStrEqual(attr, (xmlChar*)"id")) {
        prop->id = (boolean)strtol((char*)attr_val, (char**)NULL, 10);
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
        do_property_aliases( (xmlChar*)attr_val, h, prop );
    }
    else {
        SWISH_CROAK("unknown Property attribute: %s", attr);
    }

}

static void
do_property(xmlTextReaderPtr reader, headmaker *h) 
{
    xmlChar *value;
    swish_Property *prop;
    
    prop  = swish_init_property( swish_str_tolower( (xmlChar*)xmlTextReaderConstName(reader) ) );
    prop->ref_cnt++;
    value = NULL;
        
    if (    xmlTextReaderHasValue(reader) 
        &&  xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT ) {
        prop->name = swish_str_tolower( (xmlChar*)h->parent_name );
        do_property_aliases( xmlTextReaderValue(reader), h, prop );
        return;
    }
    
    
    if ( xmlTextReaderHasAttributes(reader) ) {
    
        xmlTextReaderMoveToFirstAttribute(reader);
        do_property_attr(
                xmlTextReaderConstName(reader),
                xmlTextReaderConstValue(reader),
                prop,
                h
                );
        
        while(xmlTextReaderMoveToNextAttribute(reader) == 1) {
            do_property_attr(
                xmlTextReaderConstName(reader),
                xmlTextReaderConstValue(reader),
                prop,
                h
                );
        }
    
    }
    
    if (!prop->id) {
        prop->id = h->prop_id++;
    }
        
    if (!swish_hash_exists( h->config->properties, prop->name )) {
        swish_hash_add( h->config->properties, prop->name, prop );
    } else {
        //swish_debug_config( h->config );
        SWISH_CROAK("Property %s is already defined", prop->name);
    }
    
    //swish_debug_property(prop);
    
    if (value != NULL) {
        xmlFree(value);
    }
    
    h->parent_name = xmlTextReaderConstName(reader);

}

static void
process_node(xmlTextReaderPtr reader, headmaker *h) 
{
    const xmlChar *name, *value;
    int type;

    type    = xmlTextReaderNodeType(reader);
    name    = xmlTextReaderConstName(reader);
    value   = xmlTextReaderConstValue(reader);
    
    //SWISH_DEBUG_MSG("name %s  type %d  value %s", name, type, value);
    
    if (name == NULL)
	    name = BAD_CAST "--";
        
    if (type == XML_READER_TYPE_COMMENT)
        return;
        
    if (xmlStrEqual(name, (const xmlChar*)"swish")) {
        h->is_valid = 1;
        return;
    }
    if (!h->is_valid) {
        SWISH_CROAK("invalid header file");
    }

    
    
    if (    swish_str_all_ws((xmlChar*)value) 
        &&  xmlStrEqual(name, (xmlChar*)"#text")) 
    {
        return;
    }
   
    if (type == XML_READER_TYPE_END_ELEMENT) {
        if ( xmlStrEqual(name, (const xmlChar*)SWISH_PROP) ) {
            h->isprops = 0;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_META) ) {
            h->ismetas = 0;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_INDEX) ) {
            h->isindex = 0;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_PARSERS) ) {
            h->isparser = 0;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_MIME) ) {
            h->ismime = 0;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_ALIAS) ) {
            h->isalias = 0;
            return;
        }
        
        return;
        
    }
    else {
        if ( xmlStrEqual(name, (const xmlChar*)SWISH_PROP) ) {
            h->isprops = 1;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_META) ) {
            h->ismetas = 1;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_INDEX) ) {
            h->isindex = 1;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_PARSERS) ) {
            h->isparser = 1;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_MIME) ) {
            h->ismime = 1;
            return;
        }
        else if ( xmlStrEqual(name, (const xmlChar*)SWISH_ALIAS) ) {
            h->isalias = 1;
            return;
        }
    }
    
    if (type != XML_READER_TYPE_END_ELEMENT) {
    
        if (h->isprops) {
            do_property(reader, h);
            return;
        }
        else if (h->ismetas) {
            do_metaname(reader, h);
            return;
        }
        else if (h->isindex) {
            do_index(reader, h);
            return;
        }    
        else if (h->isparser) {
            do_parser(reader, h);
            return;
        }    
        else if (h->ismime) {
            do_mime(reader, h);
            return;
        }    
        else if (h->isalias) {
            do_alias(reader, h);
            return;
        }    
        else if (type == XML_READER_TYPE_ELEMENT) {
        
            //SWISH_DEBUG_MSG("misc header value");
            
            /* element. get text and add to misc */
            if (xmlTextReaderRead(reader) == 1) {
                if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
                    value = xmlTextReaderConstValue(reader);
                    if (swish_hash_exists(h->config->misc, (xmlChar*)name)) {
                        swish_hash_replace(h->config->misc, (xmlChar*)name, swish_xstrdup(value));
                    }
                    else {
                        swish_hash_add(h->config->misc, (xmlChar*)name, swish_xstrdup(value));
                    }
                }
                else {
                    SWISH_CROAK("header line missing value: %s", name);
                }
            }
            else {
                SWISH_CROAK("error reading value for header element %s", name);
            }
            
        }   
        
    }
    

}


static void
read_header(char *filename, headmaker *h) 
{
    xmlTextReaderPtr reader;
    int ret;
    struct stat fileinfo;
    
    /* parse either a filename, or, if we can't stat it, 
       assume conf is a XML string 
    */
    if (stat((char *)filename, &fileinfo)) {
        reader = xmlReaderForMemory(
                    (const char*)filename, 
                    xmlStrlen((xmlChar*)filename), 
                    "[ swish.xml ]", 
                    NULL,
                    0);
                    
        //SWISH_DEBUG_MSG("header parsed in-memory");
    }
    else {
        reader = xmlReaderForFile(filename, NULL, 0);
        
        //SWISH_DEBUG_MSG("header parsed from file");
    }
    
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            process_node(reader, h);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) {
            SWISH_CROAK("%s : failed to parse\n", filename);
        }
    } else {
        SWISH_CROAK("Unable to open %s\n", filename);
    }
    
    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
}

static headmaker *
init_headmaker()
{
    headmaker *h;
    h           = swish_xmalloc(sizeof(headmaker));
    h->config   = swish_init_config();
    h->isprops  = 0;
    h->ismetas  = 0;
    h->parent_name = NULL;
    h->prop_id  = SWISH_PROP_THIS_MUST_COME_LAST_ID;
    h->meta_id  = SWISH_META_THIS_MUST_COME_LAST_ID;
    return h;
}

boolean
swish_validate_header(char *filename)
{
    headmaker *h;
    h = init_headmaker();
    read_header( filename, h );
    swish_debug_config( h->config );
    swish_free_config( h->config );
    swish_xfree( h );
    return 1; // how to test ?
}

boolean
swish_merge_config_with_header(char *filename, swish_Config *c)
{
    headmaker *h;
    h = init_headmaker();
    read_header( filename, h );
    swish_config_merge( c, h->config );
    swish_free_config( h->config );
    swish_xfree( h );
    return 1;
}

swish_Config *
swish_read_header(char *filename)
{
    headmaker *h;
    swish_Config *c;
    h = init_headmaker();
    read_header( filename, h );
    c = h->config;
    swish_xfree( h );
    return c;
}
