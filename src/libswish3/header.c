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

#ifndef LIBSWISH3_SINGLE_FILE
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/encoding.h>
#include <libxml/uri.h>
#include <ctype.h>
#include "libswish3.h"
#endif

extern int SWISH_DEBUG;

/* local struct to ease passing around flags/state */
typedef struct
{
    boolean isprops;
    boolean ismetas;
    boolean isindex;
    boolean isparser;
    boolean isalias;
    boolean ismime;
    const xmlChar *parent_name;
    xmlChar *conf_file;
    swish_Config *config;
    boolean is_valid;
    unsigned int prop_id;
    unsigned int meta_id;
} headmaker;

typedef struct
{
    void *thing1;
    void *thing2;
    void *thing3;
} temp_things;

static void read_metaname_aliases(
    xmlChar *str,
    headmaker * h,
    swish_MetaName *meta
);
static void read_metaname_attr(
    const xmlChar *attr,
    const xmlChar *attr_val,
    swish_MetaName *meta,
    headmaker * h
);
static void read_metaname(
    xmlTextReaderPtr reader,
    headmaker * h
);
static void read_property_aliases(
    xmlChar *str,
    headmaker * h,
    swish_Property *prop
);
static void read_property_attr(
    const xmlChar *attr,
    const xmlChar *attr_val,
    swish_Property *prop,
    headmaker * h
);
static void read_property(
    xmlTextReaderPtr reader,
    headmaker * h
);
static void process_node(
    xmlTextReaderPtr reader,
    headmaker * h
);
static void read_key_values_pair(
    xmlTextReaderPtr reader,
    xmlHashTablePtr hash,
    xmlChar *name
);
static void read_key_value_pair(
    xmlTextReaderPtr reader,
    xmlHashTablePtr hash,
    xmlChar *name
);
static void
read_key_value_stringlist(
    xmlTextReaderPtr reader,
    xmlHashTablePtr hash,
    xmlChar *name
);
static void read_header(
    char *filename,
    headmaker * h
);
static void test_meta_alias_for(
    swish_MetaName *meta,
    swish_Config *c,
    xmlChar *name
);
static void test_prop_alias_for(
    swish_Property *prop,
    swish_Config *c,
    xmlChar *name
);
static headmaker *init_headmaker(
);
static void
reset_headmaker(
    headmaker *h
);
static void write_open_tag(
    xmlTextWriterPtr writer,
    xmlChar *tag
);
static void write_close_tag(
    xmlTextWriterPtr writer
);
static void write_element_with_content(
    xmlTextWriterPtr writer,
    xmlChar *tag,
    xmlChar *content
);
static void write_metaname(
    swish_MetaName *meta,
    xmlTextWriterPtr writer,
    xmlChar *name
);
static void write_metanames(
    xmlTextWriterPtr writer,
    xmlHashTablePtr metanames
);
static void write_hash_entry(
    xmlChar *value,
    xmlTextWriterPtr writer,
    xmlChar *key
);
static void write_property(
    swish_Property *prop,
    xmlTextWriterPtr writer,
    xmlChar *name
);
static void write_properties(
    xmlTextWriterPtr writer,
    xmlHashTablePtr properties
);
static void write_parser(
    xmlChar *val,
    xmlTextWriterPtr writer,
    xmlChar *key
);
static void write_parsers(
    xmlTextWriterPtr writer,
    xmlHashTablePtr parsers
);
static void write_mime(
    xmlChar *type,
    temp_things *things,
    xmlChar *ext
);
static void write_mimes(
    xmlTextWriterPtr writer,
    xmlHashTablePtr mimes
);
static void write_index(
    xmlTextWriterPtr writer,
    xmlHashTablePtr index
);
static void write_tag_aliases(
    xmlTextWriterPtr writer,
    xmlHashTablePtr tag_aliases
);
static void write_misc(
    xmlTextWriterPtr writer,
    xmlHashTablePtr hash
);
static void handle_special_misc_flags(
    headmaker *h
);

static void
handle_special_misc_flags(
    headmaker *h
)
{
    if (swish_hash_exists(h->config->misc, BAD_CAST SWISH_TOKENIZE)) {
        /*
        SWISH_DEBUG_MSG("tokenize in config == %s", 
            swish_hash_fetch(h->config->misc, BAD_CAST SWISH_TOKENIZE));
        */
        h->config->flags->tokenize = 
            swish_string_to_boolean(swish_hash_fetch(h->config->misc, BAD_CAST SWISH_TOKENIZE));
    }
    if (swish_hash_exists(h->config->misc, BAD_CAST SWISH_CASCADE_META_CONTEXT)) {
        /*
        SWISH_DEBUG_MSG("cascade_meta_context in config == %s", 
            swish_hash_fetch(h->config->misc, BAD_CAST SWISH_CASCADE_META_CONTEXT));
        */
        h->config->flags->cascade_meta_context = 
            swish_string_to_boolean(swish_hash_fetch(h->config->misc, BAD_CAST SWISH_CASCADE_META_CONTEXT));
    }
    if (swish_hash_exists(h->config->misc, BAD_CAST SWISH_IGNORE_XMLNS)) {
        /*
        SWISH_DEBUG_MSG("ignore_xmlns in config == %s", 
            swish_hash_fetch(h->config->misc, BAD_CAST SWISH_IGNORE_XMLNS));
        */
        h->config->flags->ignore_xmlns = 
            swish_string_to_boolean(swish_hash_fetch(h->config->misc, BAD_CAST SWISH_IGNORE_XMLNS));
    }

}

static void
read_metaname_aliases(
    xmlChar *str,
    headmaker * h,
    swish_MetaName *meta
)
{
    swish_StringList *strlist;
    int i;

    strlist = swish_stringlist_build(str);

/* loop over each alias and create a MetaName for each,
       setting alias_for to meta->name
*/
    for (i = 0; i < strlist->n; i++) {

        if (!swish_hash_exists(h->config->metanames, strlist->word[i])) {
            swish_MetaName *newmeta;
            xmlChar *newname;
            newname = swish_str_tolower(strlist->word[i]);

/* is this an existing metaname? pull it from hash and update */
            if (swish_hash_exists(h->config->metanames, newname)) {
                newmeta = swish_hash_fetch(h->config->metanames, newname);
            }
/* else new metaname */
            else {
                newmeta = swish_metaname_init(newname);
                newmeta->ref_cnt++;
                newmeta->id = h->meta_id++;
                newmeta->bias = meta->bias;
                swish_hash_add(h->config->metanames, newmeta->name, newmeta);
            }

            newmeta->alias_for = swish_xstrdup(meta->name);

/* swish_metaname_debug(newmeta); */
        }
        else {
            SWISH_CROAK
                ("Cannot alias MetaName %s to %s because %s is already a real MetaName",
                 strlist->word[i], meta->name, strlist->word[i]);
        }

    }

    swish_stringlist_free(strlist);
}

static void
read_metaname_attr(
    const xmlChar *attr,
    const xmlChar *attr_val,
    swish_MetaName *meta,
    headmaker * h
)
{
    swish_MetaName *dupe;
        
    if (xmlStrEqual(attr, (xmlChar *)"bias")) {
        meta->bias = swish_string_to_int((char*)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"id")) {
        // make sure id is not already assigned
        if (swish_hash_exists(h->config->flags->meta_ids, (xmlChar*)attr_val)) {
            dupe = swish_hash_fetch(h->config->flags->meta_ids, (xmlChar*)attr_val);
            SWISH_CROAK("duplicate id %s on MetaName %s (already assigned to %s)",
                attr_val, meta->name, dupe->name);
        }
        meta->id = swish_string_to_int((char*)attr_val);
        // cache for id lookup
        swish_hash_add(h->config->flags->meta_ids, (xmlChar*)attr_val, meta);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"alias_for")) {
        meta->alias_for = swish_str_tolower(BAD_CAST attr_val);
    }
    else {
        SWISH_CROAK("Unknown MetaName attribute: %s", attr);
    }
}

static void
read_metaname(
    xmlTextReaderPtr reader,
    headmaker * h
)
{
    const xmlChar *nodename;
    swish_MetaName *meta;
    
    nodename = xmlTextReaderConstName(reader);

    meta = swish_metaname_init(swish_str_tolower((xmlChar *)nodename));
    meta->ref_cnt++;

    if (xmlTextReaderHasValue(reader)
        && xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) 
    {
        if (!h->parent_name) {
            SWISH_CROAK("Illegal text in MetaNames section: '%s'",
                xmlTextReaderValue(reader));
        }   
        
        swish_xfree(meta->name);
        meta->name = swish_str_tolower((xmlChar *)h->parent_name);
        read_metaname_aliases(xmlTextReaderValue(reader), h, meta);
        meta->ref_cnt--;
        swish_metaname_free(meta);
        return;
    }

    if (xmlTextReaderHasAttributes(reader)) {

        xmlTextReaderMoveToFirstAttribute(reader);
        if (xmlStrEqual(xmlTextReaderConstPrefix(reader),(xmlChar*)"xmlns")) {
            if (xmlTextReaderMoveToNextAttribute(reader) == 1) {
                read_metaname_attr(xmlTextReaderConstName(reader),
                           xmlTextReaderConstValue(reader), meta, h);
            }
        }
        else {
            read_metaname_attr(xmlTextReaderConstName(reader),
                           xmlTextReaderConstValue(reader), meta, h);
        }
        
        while (
            xmlTextReaderMoveToNextAttribute(reader) == 1
            &&
            !xmlStrEqual(xmlTextReaderConstPrefix(reader),(xmlChar*)"xmlns")
        ) {
            read_metaname_attr(xmlTextReaderConstName(reader),
                               xmlTextReaderConstValue(reader), meta, h);
        }

    }

/*  must have an id */
    if (meta->id == -1) {
        meta->id = h->meta_id++;
    }

    if (!swish_hash_exists(h->config->metanames, meta->name)) {
        swish_hash_add(h->config->metanames, meta->name, meta);
    }
    else {
        SWISH_WARN("MetaName %s is already defined", meta->name);
/*  TODO could be alias. how to check? */
    }

/* swish_metaname_debug(meta); */

    h->parent_name = nodename;

}

static void
read_property_aliases(
    xmlChar *str,
    headmaker * h,
    swish_Property *prop
)
{
    swish_StringList *strlist;
    int i;

    strlist = swish_stringlist_build(str);

/* loop over each alias and create a Property for each,
   setting alias_for to prop->name
*/
    for (i = 0; i < strlist->n; i++) {

        if (!swish_hash_exists(h->config->properties, strlist->word[i])) {
            swish_Property *newprop =
                swish_property_init(swish_str_tolower(strlist->word[i]));
            newprop->ref_cnt++;
            newprop->alias_for = swish_xstrdup(prop->name);
            newprop->id = h->prop_id++;
            newprop->ignore_case = prop->ignore_case;
            newprop->type = prop->type;
            newprop->verbatim = prop->verbatim;
            newprop->max = prop->max;
            newprop->sort = prop->sort;
            swish_hash_add(h->config->properties, newprop->name, newprop);
            /* swish_property_debug(newprop); */
        }
        else {
            SWISH_CROAK
                ("Cannot alias Property %s to %s because %s is already a real Property",
                 strlist->word[i], prop->name, strlist->word[i]);
        }

    }

    swish_stringlist_free(strlist);
}

static void
read_property_attr(
    const xmlChar *attr,
    const xmlChar *attr_val,
    swish_Property *prop,
    headmaker * h
)
{
    swish_Property *dupe;
    
    if (xmlStrEqual(attr, (xmlChar *)"ignore_case")) {
        prop->ignore_case = swish_string_to_boolean((char *)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"max")) {
        prop->max = swish_string_to_int((char *)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"verbatim")) {
        prop->verbatim = swish_string_to_boolean((char *)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"sort")) {
        prop->sort = swish_string_to_boolean((char *)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"presort")) {
        prop->presort = swish_string_to_boolean((char *)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"sort_length")) {
        prop->sort_length = swish_string_to_int((char *)attr_val);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"id")) {
        // make sure id is not already assigned
        if (swish_hash_exists(h->config->flags->prop_ids, (xmlChar*)attr_val)) {
            dupe = swish_hash_fetch(h->config->flags->prop_ids, (xmlChar*)attr_val);
            SWISH_CROAK("duplicate id %s on MetaName %s (already assigned to %s)",
                attr_val, prop->name, dupe->name);
        }
        prop->id = swish_string_to_int((char*)attr_val);
        // cache for id lookup
        swish_hash_add(h->config->flags->prop_ids, (xmlChar*)attr_val, prop);
    }
    else if (xmlStrEqual(attr, (xmlChar *)"type")) {
        if (xmlStrEqual(attr_val, (xmlChar *)"int")) {
            prop->type = SWISH_PROP_INT;
        }
        else if (xmlStrEqual(attr_val, (xmlChar *)"date")) {
            prop->type = SWISH_PROP_DATE;
        }
        else if (xmlStrEqual(attr_val, (xmlChar*)"string")
                ||
                 xmlStrEqual(attr_val, (xmlChar*)"text")
        ) {
            prop->type = SWISH_PROP_STRING;
        }
        else if (isdigit(attr_val[0])) {
            prop->type = swish_string_to_int((char*)attr_val);
        }
        else {
            SWISH_CROAK("Invalid value for PropertyName '%s' type: %s",
                prop->name, attr_val);
        }
    }
    else if (xmlStrEqual(attr, (xmlChar *)"alias_for")) {
        prop->alias_for = swish_str_tolower(BAD_CAST attr_val);
    }
    else {
        SWISH_CROAK("unknown Property attribute: %s", attr);
    }

}

static void
read_property(
    xmlTextReaderPtr reader,
    headmaker * h
)
{
    const xmlChar *nodename;
    swish_Property *prop;

    nodename = xmlTextReaderConstName(reader);
    prop = swish_property_init(swish_str_tolower((xmlChar *)nodename));
    prop->ref_cnt++;

    if (xmlTextReaderHasValue(reader)
        && xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
        
        if (!h->parent_name) {
            SWISH_CROAK("Illegal text in PropertyNames section: '%s'",
                xmlTextReaderValue(reader));
        }
        
        swish_xfree(prop->name);
        prop->name = swish_str_tolower((xmlChar *)h->parent_name);
        read_property_aliases(xmlTextReaderValue(reader), h, prop);
        prop->ref_cnt--;
        swish_property_free(prop);
        return;
    }

    if (xmlTextReaderHasAttributes(reader)) {

        xmlTextReaderMoveToFirstAttribute(reader);
        if (xmlStrEqual(xmlTextReaderConstPrefix(reader),(xmlChar*)"xmlns")) {
            if (xmlTextReaderMoveToNextAttribute(reader) == 1) {
                read_property_attr(xmlTextReaderConstName(reader),
                           xmlTextReaderConstValue(reader), prop, h);
            }
        }
        else {
            read_property_attr(xmlTextReaderConstName(reader),
                           xmlTextReaderConstValue(reader), prop, h);
        }

        while (
            xmlTextReaderMoveToNextAttribute(reader) == 1
            &&
            !xmlStrEqual(xmlTextReaderConstPrefix(reader),(xmlChar*)"xmlns")
        ) {
            read_property_attr(xmlTextReaderConstName(reader),
                               xmlTextReaderConstValue(reader), prop, h);
        }

    }

    if (prop->id == -1) {
        prop->id = h->prop_id++;
    }

    if (!swish_hash_exists(h->config->properties, prop->name)) {
        swish_hash_add(h->config->properties, prop->name, prop);
    }
    else {
/* swish_config_debug( h->config ); */
        SWISH_CROAK("Property %s is already defined", prop->name);
    }

/* swish_property_debug(prop); */

    h->parent_name = nodename;

}

static void
process_node(
    xmlTextReaderPtr reader,
    headmaker * h
)
{
    const xmlChar *name, *value;
    int type;

    type = xmlTextReaderNodeType(reader);
    name = xmlTextReaderConstLocalName(reader);
    value = xmlTextReaderConstValue(reader);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("name %s  type %d  value %s", name, type, value);

    if (name == NULL)
        name = BAD_CAST "--";

    if (type == XML_READER_TYPE_COMMENT)
        return;

    if (xmlStrEqual(name, (const xmlChar *)SWISH_HEADER_ROOT)) {
        h->is_valid = 1;
        return;
    }
    if (!h->is_valid) {
        SWISH_CROAK("invalid header file");
    }

    if (swish_str_all_ws((xmlChar *)value)
        && xmlStrEqual(name, (xmlChar *)"#text")) {
        return;
    }            

    if (type == XML_READER_TYPE_END_ELEMENT) {
        if (xmlStrEqual(name, (const xmlChar *)SWISH_PROP)) {
            reset_headmaker(h);
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_META)) {
            reset_headmaker(h);
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_INDEX)) {
            reset_headmaker(h);
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_PARSERS)) {
            reset_headmaker(h);
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_MIME)) {
            reset_headmaker(h);
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_ALIAS)) {
            reset_headmaker(h);
            return;
        }

        //SWISH_DEBUG_MSG("END ELEMENT name %s  type %d  value %s", name, type, value);

        return;

    }
    else {
    
              
    /* the special include directive means we stop and process
     * that config file immediately instead of storing the value
     * in the hash.
     */
        if (xmlStrEqual(name, BAD_CAST SWISH_INCLUDE_FILE)) {
            if (xmlTextReaderRead(reader) == 1) {
                if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
                    value = xmlTextReaderConstValue(reader);
                    xmlChar *conf_file = swish_xstrdup(value);
                    if (conf_file[0] != SWISH_PATH_SEP) {
                        xmlChar *path, *xuri;
                        path = swish_fs_get_path(h->conf_file);
                        if (path == NULL) {
                            SWISH_CROAK("Unable to resolve config file path %s relative to %s", 
                                conf_file, h->conf_file);
                        }
                        xuri = xmlBuildURI(conf_file, path);
                        if (xuri == NULL) {
                            SWISH_CROAK("Unable to build URI for %s and %s", conf_file, path);
                        }
                        swish_xfree(conf_file);
                        conf_file = swish_xstrdup(xuri);
                        xmlFree(xuri); /* because we did not malloc it */
                        xmlFree(path); /* because we did not malloc it */
                    }
                    swish_header_merge((char*)conf_file, h->config);
                    swish_xfree(conf_file);
                    return;
                }
            }
            SWISH_CROAK("Invalid value for %s", name);
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_PROP)) {
            reset_headmaker(h);
            h->isprops = 1;
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_META)) {
            reset_headmaker(h);
            h->ismetas = 1;
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_INDEX)) {
            reset_headmaker(h);
            h->isindex = 1;
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_PARSERS)) {
            reset_headmaker(h);
            h->isparser = 1;
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_MIME)) {
            reset_headmaker(h);
            h->ismime = 1;
            return;
        }
        else if (xmlStrEqual(name, (const xmlChar *)SWISH_ALIAS)) {
            reset_headmaker(h);
            h->isalias = 1;
            return;
        }

        //SWISH_DEBUG_MSG("NOT END ELEMENT name %s  type %d  value %s", name, type, value);
    }

    if (type != XML_READER_TYPE_END_ELEMENT) {

        if (h->isprops) {
            read_property(reader, h);
            return;
        }
        else if (h->ismetas) {
            read_metaname(reader, h);
            return;
        }
        else if (h->isindex) {
            read_key_value_pair(reader, h->config->index, (xmlChar *)name);
            return;
        }
        else if (h->isparser) {
            read_key_values_pair(reader, h->config->parsers, (xmlChar *)name);
            return;
        }
        else if (h->ismime) {
            read_key_value_pair(reader, h->config->mimes, (xmlChar *)name);
            return;
        }
        else if (h->isalias) {
            read_key_values_pair(reader, h->config->tag_aliases, (xmlChar *)name);
            return;
        }
        else if (xmlStrEqual((xmlChar *)SWISH_CLASS_ATTRIBUTES, (xmlChar *)name)) {
            read_key_value_stringlist(reader, h->config->stringlists, (xmlChar *)name);
            return;
        }
        else if (type == XML_READER_TYPE_ELEMENT) {
            read_key_value_pair(reader, h->config->misc, (xmlChar *)name);
            handle_special_misc_flags(h);
            return;
        }

        /*
           SWISH_DEBUG_MSG("STILL NOT END ELEMENT name %s  type %d  value %s", name, type, value); 
         */

    }

}

static void
read_key_value_stringlist(
    xmlTextReaderPtr reader,
    xmlHashTablePtr hash,
    xmlChar *name
)
{
    swish_StringList *strlist;
    xmlChar *str;
    const xmlChar *value;

/* element. get text and add to misc */
    if (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {

            value = xmlTextReaderConstValue(reader);
            str = swish_str_tolower((xmlChar *)value);
            strlist = swish_stringlist_build(str);
            if (swish_hash_exists(hash, name)) {
                swish_stringlist_merge(strlist, swish_hash_fetch(hash, name));
            }
            else {
                swish_hash_add(hash, name, strlist);
            }
            swish_xfree(str);
        }
        else {
            SWISH_CROAK("Top-level XML element missing value: %s", name);
        }
    }
    else {
        SWISH_CROAK("Error reading value for top-level XML element %s", name);
    }
}

static void
read_key_values_pair(
    xmlTextReaderPtr reader,
    xmlHashTablePtr hash,
    xmlChar *name
)
{
    swish_StringList *strlist;
    xmlChar *str;
    const xmlChar *value;
    int i;

/* element. get text and add to misc */
    if (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {

            value = xmlTextReaderConstValue(reader);
            str = swish_str_tolower((xmlChar *)value);
            strlist = swish_stringlist_build(str);

            for (i = 0; i < strlist->n; i++) {
/*  SWISH_DEBUG_MSG("key_values pair: %s -> %s", strlist->word[i], name);  */
                if (swish_hash_exists(hash, strlist->word[i])) {
                    swish_hash_replace(hash, strlist->word[i], swish_xstrdup(name));
                }
                else {
                    swish_hash_add(hash, strlist->word[i], swish_xstrdup(name));
                }
            }

            swish_stringlist_free(strlist);
            swish_xfree(str);

        }
        else {
            SWISH_CROAK("Top-level XML element missing value: %s", name);
        }
    }
    else {
        SWISH_CROAK("Error reading value for top-level XML element %s", name);
    }

}

static void
read_key_value_pair(
    xmlTextReaderPtr reader,
    xmlHashTablePtr hash,
    xmlChar *name
)
{
    const xmlChar *value;

/* element. get text and add to misc */
    if (xmlTextReaderRead(reader) == 1) {
        if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
            value = xmlTextReaderConstValue(reader);            
            
            if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
                SWISH_DEBUG_MSG("read key %s for value %s", name, value);
            }
            
            if (swish_hash_exists(hash, name)) {
            
                if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
                    SWISH_DEBUG_MSG("replacing %s => %s in hash", name, value);
                }
                swish_hash_replace(hash, name, swish_xstrdup(value));
            }
            else {
            
                if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
                    SWISH_DEBUG_MSG("adding %s => %s to hash", name, value);
                }
                swish_hash_add(hash, name, swish_xstrdup(value));
            }
        }
        else {
            SWISH_CROAK("Top-level XML element missing value: %s", name);
        }
    }
    else {
        SWISH_CROAK("Error reading value for top-level XML element %s", name);
    }

}

static void
read_header(
    char *filename,
    headmaker * h
)
{
    xmlTextReaderPtr reader;
    int ret;

/* parse either a filename, or, if we can't stat it,
 * assume conf is a XML string.
 */
    if (!swish_fs_file_exists((xmlChar*)filename)) {
        reader =
            xmlReaderForMemory((const char *)filename, xmlStrlen((xmlChar *)filename),
                               "[ swish.xml ]", NULL, 0);

        h->conf_file = swish_xstrdup(BAD_CAST "in-memory");
        
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("header parsed in-memory");
        }
    }
    else {
        reader = xmlReaderForFile(filename, NULL, 0);
        h->conf_file = swish_xstrdup(BAD_CAST filename);

        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("header parsed from file");
        }
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
        swish_xfree(h->conf_file);
        h->conf_file = NULL;
    }
    else {
        SWISH_CROAK("Unable to open %s\n", filename);
    }

/*
 * Cleanup function for the XML library.
 */
    xmlCleanupParser();
}

static void
test_meta_alias_for(
    swish_MetaName *meta,
    swish_Config *c,
    xmlChar *name
)
{
    if (meta->alias_for != NULL && !swish_hash_exists(c->metanames, meta->alias_for)
        ) {
        SWISH_CROAK
            ("MetaName '%s' has alias_for value of '%s' but no such MetaName defined",
             name, meta->alias_for);
    }
}

static void
test_prop_alias_for(
    swish_Property *prop,
    swish_Config *c,
    xmlChar *name
)
{
    if (prop->alias_for != NULL && !swish_hash_exists(c->properties, prop->alias_for)) {
        SWISH_CROAK("Property '%s' has alias_for value of '%s' but no such Property defined",
             name, prop->alias_for);
    }
}

void
swish_config_test_alias_fors(
    swish_Config *c
)
{
    xmlHashScan(c->metanames, (xmlHashScanner)test_meta_alias_for, c);
    xmlHashScan(c->properties, (xmlHashScanner)test_prop_alias_for, c);
}


static headmaker *
init_headmaker(
)
{
    headmaker *h;
    h = swish_xmalloc(sizeof(headmaker));
    h->config = swish_config_init();
/*  mimes is set to NULL in default config but we need it to be a hash here. */
    h->config->mimes = swish_hash_init(8);
    reset_headmaker(h);
    h->prop_id = SWISH_PROP_THIS_MUST_COME_LAST_ID;
    h->meta_id = SWISH_META_THIS_MUST_COME_LAST_ID;
    h->conf_file = NULL;
    return h;
}

static void
reset_headmaker(
    headmaker *h
)
{
    h->isprops = 0;
    h->ismetas = 0;
    h->isindex = 0;
    h->isalias = 0;
    h->isparser = 0;
    h->ismime = 0;
    h->parent_name = NULL;
}

boolean
swish_header_validate(
    char *filename
)
{
    headmaker *h;
    h = init_headmaker();
    read_header(filename, h);

/*  test that all the alias_for links resolve ok */
    swish_config_test_alias_fors(h->config);

    swish_config_debug(h->config);
    swish_config_free(h->config);
    if (h->conf_file != NULL) {
        swish_xfree(h->conf_file);
    }
    swish_xfree(h);
    return 1;                   /* how to test ? */
}

boolean
swish_header_merge(
    char *filename,
    swish_Config *c
)
{
    headmaker *h;
    h = init_headmaker();
    read_header(filename, h);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("read_header complete");
    }
    swish_config_merge(c, h->config);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("config_merge complete");
    }
    swish_config_free(h->config);
    if (h->conf_file != NULL) {
        swish_xfree(h->conf_file);
    }
    swish_xfree(h);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("temp head struct freed");
    }

/*  test that all the alias_for links resolve ok */
    swish_config_test_alias_fors(c);

    return 1;
}

swish_Config *
swish_header_read(
    char *filename
)
{
    headmaker *h;
    swish_Config *c;
    h = init_headmaker();
    read_header(filename, h);
    c = h->config;
    if (h->conf_file != NULL) {
        swish_xfree(h->conf_file);
    }
    swish_xfree(h);
    return c;
}

static void
write_open_tag(
    xmlTextWriterPtr writer,
    xmlChar *tag
)
{
    int rc;
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("writing open tag <%s>", tag);
    }
    rc = xmlTextWriterStartElement(writer, tag);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("wrote open tag <%s>", tag);
    }

    if (rc < 0) {
        SWISH_CROAK("Error writing element %s", tag);
    }
}

static void
write_close_tag(
    xmlTextWriterPtr writer
)
{
    int rc;
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("writing close tag");
    }
    rc = xmlTextWriterEndElement(writer);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("wrote close tag");
    }
    if (rc < 0) {
        SWISH_CROAK("Error at xmlTextWriterEndElement");
    }
}

static void
write_element_with_content(
    xmlTextWriterPtr writer,
    xmlChar *tag,
    xmlChar *content
)
{
    int rc;
    rc = xmlTextWriterWriteElement(writer, tag, content);
    if (rc < 0) {
        SWISH_CROAK("Error writing element %s with content %s", tag, content);
    }
}

static void
write_metaname(
    swish_MetaName *meta,
    xmlTextWriterPtr writer,
    xmlChar *name
)
{
    int rc;
    write_open_tag(writer, name);
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id", "%d", meta->id);
    if (rc < 0) {
        SWISH_CROAK("Error writing metaname id attribute for %s", name);
    }

    if (meta->alias_for != NULL) {
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "alias_for", meta->alias_for);
        if (rc < 0) {
            SWISH_CROAK("Error writing metaname alias_for attribute for %s", name);
        }

    }
    else {
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "bias", "%d", meta->bias);
        if (rc < 0) {
            SWISH_CROAK("Error writing metaname bias attribute for %s", name);
        }
    }

    write_close_tag(writer);
}

static void
write_metanames(
    xmlTextWriterPtr writer,
    xmlHashTablePtr metanames
)
{
    xmlHashScan(metanames, (xmlHashScanner)write_metaname, writer);
}

static void
write_hash_entry(
    xmlChar *value,
    xmlTextWriterPtr writer,
    xmlChar *key
)
{
    write_element_with_content(writer, key, value);
}

static void
write_reverse_hash_entry(
    xmlChar *value,
    xmlTextWriterPtr writer,
    xmlChar *key
)
{
    write_element_with_content(writer, value, key);
}

static void
write_property(
    swish_Property *prop,
    xmlTextWriterPtr writer,
    xmlChar *name
)
{
    int rc;
    write_open_tag(writer, name);
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "id", "%d", prop->id);
    if (rc < 0) {
        SWISH_CROAK("Error writing property id attribute for %s", name);
    }

    if (prop->alias_for != NULL) {
        rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "alias_for", prop->alias_for);
        if (rc < 0) {
            SWISH_CROAK("Error writing property alias_for attribute for %s", name);
        }
    }
    else {

/* all other attrs are irrelevant if this is an alias */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ignore_case", "%d",
                                               prop->ignore_case);
        if (rc < 0) {
            SWISH_CROAK("Error writing property ignore_case attribute for %s", name);
        }
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "verbatim", "%d",
                                               prop->verbatim);
        if (rc < 0) {
            SWISH_CROAK("Error writing property verbatim attribute for %s", name);
        }
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "type", "%d", prop->type);
        if (rc < 0) {
            SWISH_CROAK("Error writing property type attribute for %s", name);
        }
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "max", "%d", prop->max);
        if (rc < 0) {
            SWISH_CROAK("Error writing property max attribute for %s", name);
        }
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "sort", "%d", prop->sort);
        if (rc < 0) {
            SWISH_CROAK("Error writing property sort attribute for %s", name);
        }
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "sort_length", "%d", prop->sort_length);
        if (rc < 0) {
            SWISH_CROAK("Error writing property sort attribute for %s", name);
        }
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "presort", "%d", prop->presort);
        if (rc < 0) {
            SWISH_CROAK("Error writing property sort attribute for %s", name);
        }
    }
    write_close_tag(writer);
}

static void
write_properties(
    xmlTextWriterPtr writer,
    xmlHashTablePtr properties
)
{
    xmlHashScan(properties, (xmlHashScanner)write_property, writer);
}

static void
write_parser(
    xmlChar *val,
    xmlTextWriterPtr writer,
    xmlChar *key
)
{
    write_element_with_content(writer, val, key);
}

static void
write_parsers(
    xmlTextWriterPtr writer,
    xmlHashTablePtr parsers
)
{
    xmlHashScan(parsers, (xmlHashScanner)write_parser, writer);
}

static void
write_mime(
    xmlChar *type,
    temp_things  *things,
    xmlChar *ext
)
{
    if (   !swish_hash_exists((xmlHashTablePtr) things->thing1, ext)
        || !xmlStrEqual(swish_hash_fetch((xmlHashTablePtr) things->thing1, ext), type)
    ) {

/*
        if (!swish_hash_exists((xmlHashTablePtr) things->thing1, ext)) {
            SWISH_DEBUG_MSG("%s not in hash", ext);
        }
*/
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("writing unique MIME %s => %s", ext, type);
        }
        write_element_with_content((xmlTextWriterPtr) things->thing3, ext, type);
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("wrote unique MIME %s => %s", ext, type);
        }
    }
}

static void
write_mimes(
    xmlTextWriterPtr writer,
    xmlHashTablePtr mimes
)
{
/*  only write what differs from the default */
    temp_things *t;
    t = swish_xmalloc(sizeof(temp_things));
    t->thing1 = swish_mime_defaults();
    t->thing2 = mimes;
    t->thing3 = writer;
    xmlHashScan(mimes, (xmlHashScanner)write_mime, t);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("done writing MIMEs");
    }
    swish_hash_free(t->thing1);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("freed thing1 hash");
    }
    swish_xfree(t);
}

static void
write_index(
    xmlTextWriterPtr writer,
    xmlHashTablePtr index
)
{
    xmlHashScan(index, (xmlHashScanner)write_hash_entry, writer);
}

static void
write_tag_aliases(
    xmlTextWriterPtr writer,
    xmlHashTablePtr tag_aliases
)
{
    xmlHashScan(tag_aliases, (xmlHashScanner)write_reverse_hash_entry, writer);
}

static void
write_misc(
    xmlTextWriterPtr writer,
    xmlHashTablePtr hash
)
{
    xmlHashScan(hash, (xmlHashScanner)write_hash_entry, writer);
}

void
swish_header_write(
    char *uri,
    swish_Config *config
)
{
#if !defined(LIBXML_WRITER_ENABLED) || !defined(LIBXML_OUTPUT_ENABLED)
    SWISH_CROAK("libxml2 writer not compiled in this version of libxml2");
#else
    int rc;
    xmlTextWriterPtr writer;

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        swish_config_debug(config);
    }

/* Create a new XmlWriter for uri, with no compression. */
    writer = xmlNewTextWriterFilename((const char *)uri, 0);
    if (writer == NULL) {
        SWISH_CROAK("Error creating the xml writer\n");
    }

/* set some basic formatting rules. these make it easier to debug headers */
    rc = xmlTextWriterSetIndent(writer, 1);
    if (rc < 0) {
        SWISH_CROAK("failed to set indent on XML writer");
    }

/* Start the document with the xml default for the version,
     * encoding UTF-8 (default) and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
    if (rc < 0) {
        SWISH_CROAK("Error at xmlTextWriterStartDocument\n");
    }

/* root element
    NOTE the BAD_CAST macro is xml2 shortcut for (xmlChar*)
*/
    write_open_tag(writer, BAD_CAST SWISH_HEADER_ROOT);

/* Write a comment indicating a computer wrote this file */
    rc = xmlTextWriterWriteComment(writer, BAD_CAST "written by libswish3 - DO NOT EDIT");
    if (rc < 0) {
        SWISH_CROAK("Error at xmlTextWriterWriteComment\n");
    }

    // TODO check for these in reader and croak if mismatch
    if (!swish_hash_exists(config->misc, BAD_CAST "swish_version")) {
        write_element_with_content(writer, BAD_CAST "swish_version",
                                   BAD_CAST SWISH_VERSION);
    }
    if (!swish_hash_exists(config->misc, BAD_CAST "swish_lib_version")) {
        write_element_with_content(writer, BAD_CAST "swish_lib_version",
                                   BAD_CAST swish_lib_version());
    }

/* write MetaNames */
    write_open_tag(writer, BAD_CAST SWISH_META);
    write_metanames(writer, config->metanames);
    write_close_tag(writer);

/* write PropertyNames */
    write_open_tag(writer, BAD_CAST SWISH_PROP);
    write_properties(writer, config->properties);
    write_close_tag(writer);

/* write Parsers */
    write_open_tag(writer, BAD_CAST SWISH_PARSERS);
    write_parsers(writer, config->parsers);
    write_close_tag(writer);

/* write MIMEs */

    write_open_tag(writer, BAD_CAST SWISH_MIME);
    write_mimes(writer, config->mimes);
    write_close_tag(writer);

/* write index */
    write_open_tag(writer, BAD_CAST SWISH_INDEX);
    write_index(writer, config->index);
    write_close_tag(writer);

    write_open_tag(writer, BAD_CAST SWISH_ALIAS);
    write_tag_aliases(writer, config->tag_aliases);
    write_close_tag(writer);

/* misc tags have no parent */
    write_misc(writer, config->misc);

/* this function will close any open tags */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0) {
        SWISH_CROAK("Error at xmlTextWriterEndDocument\n");
    }

    xmlFreeTextWriter(writer);
#endif
}
