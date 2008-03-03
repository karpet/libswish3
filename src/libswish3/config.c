/*
 * This file is part of libswish3
 * Copyright (C) 2007 Peter Karman
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

/* parse XML-style config files
 * 
 * based on http://www.yolinux.com/TUTORIALS/GnomeLibXml2.html
 * 
 */

#include <libxml/xmlstring.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"

extern int      SWISH_DEBUG;


static void     config_printer(xmlChar * val, xmlChar * str, xmlChar * key);

static xmlDocPtr parse_xml_config(xmlChar * conf);
static xmlChar *get_node_name(xmlNode * node);
static xmlNode *get_dom_root(xmlDocPtr doc);

static void     free_string(xmlChar *payload, xmlChar *key);
static void     free_props(swish_Property *prop, xmlChar *propname);
static void     free_metas(swish_MetaName *meta, xmlChar *metaname);

static void
free_string(xmlChar *payload, xmlChar * key)
{
    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("   freeing config %s => %s", key, payload);

    swish_xfree(payload);
}

static void
free_props(swish_Property *prop, xmlChar *propname)
{
    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("   freeing config->prop %s", propname);
        swish_debug_property((swish_Property*)prop);
    }
    prop->ref_cnt--;
    swish_free_property(prop);
}

static void
free_metas(swish_MetaName *meta, xmlChar *metaname)
{
    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("   freeing config->meta %s", metaname);
        swish_debug_metaname((swish_MetaName*)meta);
    }
    meta->ref_cnt--;
    swish_free_metaname(meta);
}

void
swish_free_config(swish_Config * config)
{
    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
    {
        SWISH_DEBUG_MSG("freeing config");
        SWISH_DEBUG_MSG("ptr addr: 0x%x  %d", (int) config, (int) config);
    }

    xmlHashFree(config->misc,           (xmlHashDeallocator)free_string);
    xmlHashFree(config->properties,     (xmlHashDeallocator)free_props);
    xmlHashFree(config->metanames,      (xmlHashDeallocator)free_metas);
    xmlHashFree(config->tag_aliases,    (xmlHashDeallocator)free_string);
    xmlHashFree(config->parsers,        (xmlHashDeallocator)free_string);
    xmlHashFree(config->mimes,          (xmlHashDeallocator)free_string);
    xmlHashFree(config->index,          (xmlHashDeallocator)free_string);
    swish_xfree(config->flags);

    if (config->ref_cnt != 0) {
        SWISH_WARN("config ref_cnt != 0: %d", config->ref_cnt);
    }

    if (config->stash != NULL) {
        SWISH_WARN("possible memory leak: config->stash was not freed");
    }

    swish_xfree(config);
}



/* init config object */

swish_Config  *
swish_init_config()
{
    swish_Config  *config;
    
    /* the hashes will automatically grow as needed so we init with sane starting size */
    config              = swish_xmalloc(sizeof(swish_Config));
    config->flags       = swish_xmalloc(sizeof(swish_ConfigFlags));
    config->misc        = swish_new_hash(8);
    config->metanames   = swish_new_hash(8);
    config->properties  = swish_new_hash(8);
    config->parsers     = swish_new_hash(8);
    config->index       = swish_new_hash(8);
    config->tag_aliases = swish_new_hash(8);
    config->mimes       = NULL;
    config->ref_cnt     = 0;
    config->stash       = NULL;
    
    return config;

}

void
swish_config_set_default( swish_Config *config )
{
    swish_Property *tmpprop;
    swish_MetaName *tmpmeta;

    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("setting default config");
        
    /* we xstrdup a lot in order to consistently free in swish_free_config() */

    /* MIME types */
    config->mimes = swish_mime_hash();

    /* metanames */
    swish_hash_add(
            config->metanames, 
            (xmlChar*)SWISH_DEFAULT_METANAME,
            swish_init_metaname( swish_xstrdup((xmlChar*)SWISH_DEFAULT_METANAME) ) 
            );
    swish_hash_add(
            config->metanames,
            (xmlChar*)SWISH_TITLE_METANAME,
            swish_init_metaname( swish_xstrdup((xmlChar*)SWISH_TITLE_METANAME) )
            );
            
    /* increm ref counts after they've been stashed. a little awkward, but saves var names... */
    tmpmeta = xmlHashLookup(config->metanames, (xmlChar*)SWISH_DEFAULT_METANAME);
    tmpmeta->ref_cnt++;
    tmpmeta = xmlHashLookup(config->metanames, (xmlChar*)SWISH_TITLE_METANAME);
    tmpmeta->ref_cnt++;
    

    /* parsers */
    swish_hash_add(
            config->parsers,
            (xmlChar *) "text/plain",
            swish_xstrdup((xmlChar *) SWISH_PARSER_TXT));
    swish_hash_add(
            config->parsers,
            (xmlChar *) "text/xml",
            swish_xstrdup((xmlChar *) SWISH_PARSER_XML));
    swish_hash_add(
            config->parsers,
            (xmlChar *) "text/html",
            swish_xstrdup((xmlChar *) SWISH_PARSER_HTML));
    swish_hash_add(
            config->parsers,
            (xmlChar *) SWISH_DEFAULT_PARSER,
            swish_xstrdup((xmlChar *) SWISH_DEFAULT_PARSER_TYPE));


    /* index */
    swish_hash_add(
            config->index,
            (xmlChar *) SWISH_INDEX_FORMAT,
            swish_xstrdup((xmlChar *) SWISH_INDEX_FILEFORMAT));
    swish_hash_add(
            config->index,
            (xmlChar *) SWISH_INDEX_NAME,
            swish_xstrdup((xmlChar *) SWISH_INDEX_FILENAME));
    swish_hash_add(
            config->index,
            (xmlChar *) SWISH_INDEX_LOCALE,
            swish_xstrdup((xmlChar *) setlocale(LC_ALL, "")));


    /* properties */
    swish_hash_add(
            config->properties,
            (xmlChar*)SWISH_PROP_DESCRIPTION,
            swish_init_property(swish_xstrdup((xmlChar*)SWISH_PROP_DESCRIPTION))
            );
    swish_hash_add(
            config->properties,
            (xmlChar*)SWISH_PROP_TITLE,
            swish_init_property(swish_xstrdup((xmlChar*)SWISH_PROP_TITLE))
            );

    /* same deal as metanames above */
    tmpprop = xmlHashLookup(config->properties, (xmlChar*)SWISH_PROP_DESCRIPTION);
    tmpprop->ref_cnt++;
    tmpprop = xmlHashLookup(config->properties, (xmlChar*)SWISH_PROP_TITLE);
    tmpprop->ref_cnt++;
    

    /* aliases: other names a tag might be known as, for matching properties and
     * metanames */
    swish_hash_add(
            config->tag_aliases,
            (xmlChar *) SWISH_TITLE_TAG,
            swish_xstrdup((xmlChar *) SWISH_TITLE_METANAME));
    swish_hash_add(
            config->tag_aliases,
            (xmlChar *) SWISH_BODY_TAG,
            swish_xstrdup((xmlChar *) SWISH_PROP_DESCRIPTION));

    /* misc default flags */
    config->flags->tokenize = 1;
    
    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("config_set_default done");
        swish_debug_config(config);
    }

}

/* PUBLIC */
swish_Config  *
swish_add_config(xmlChar * conf, swish_Config * config)
{

    config = swish_parse_config(conf, config);

    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
        swish_debug_config(config);


    return config;

}

static xmlChar *
get_node_name(xmlNode * node)
{
    return swish_xstrdup(node->name);
}

static xmlDocPtr 
parse_xml_config(xmlChar * conf)
{
    xmlDocPtr       doc;
    struct stat     fileinfo;

    /* parse either a filename, or, if we can't stat it, assume conf is a XML string */
    if (!stat((char *) conf, &fileinfo))
    {
        doc = xmlParseFile((const char *) conf);
        if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
            SWISH_DEBUG_MSG("Parsing configuration file: %s", conf);
    }
    else
    {
        doc = xmlParseMemory((const char *) conf, xmlStrlen(conf));
        if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
            SWISH_DEBUG_MSG("Parsing configuration from memory");
    }

    if (doc == NULL)
        SWISH_CROAK("error: could not parse XML: %s", conf);

    return doc;

}


static xmlNode *
get_dom_root(xmlDocPtr doc)
{
    xmlNode        *root = NULL;
    xmlChar        *toptag = (xmlChar*)"swishconfig";

    root = xmlDocGetRootElement(doc);

    /* --------------------------------------------------------------------------
     * Must have root element, a name and the name must be "swishconfig"
     * -------------------------------------------------------------------------- */
    if (!root ||
        !root->name ||
        !xmlStrEqual(root->name, toptag))
    {
        xmlFreeDoc(doc);
        SWISH_CROAK("bad config format: malformed or missing '%s' toplevel tag", toptag);
        return 0;
    }

    return root;
}


swish_Config  *
swish_parse_config(xmlChar * conf, swish_Config * config)
{
    xmlNode        *cur_node, *root;
    xmlChar        *opt_name, *opt_type, *opt_arg, *tmp_arg, *tmp_value;
    int             name_seen, i, free_tmp;
    swish_StringList *arg_list;
    xmlHashTablePtr vhash;
    xmlDocPtr       doc;

    doc = parse_xml_config(conf);
    root = get_dom_root(doc);

    /* init all strings */

    opt_name = NULL;
    opt_type = NULL;
    opt_arg = NULL;


    /* -------------------------------------------------------------------------- get
     * options/values format is:
     * 
     * <directive type="someval">some arg</directive>
     * 
     * where type="someval" attr is optional -- defaults to whatever 'some arg' is
     * -------------------------------------------------------------------------- */

    for (cur_node = root->children; cur_node != NULL; cur_node = cur_node->next)
    {
        if (cur_node->type == XML_ELEMENT_NODE)
        {
            /* fprintf(stderr, "Element: %s \n", cur_node->name); */

            opt_name = get_node_name(cur_node);

            opt_type = xmlGetProp(cur_node, (xmlChar *) "type");

            opt_arg = xmlNodeGetContent(cur_node);

            if (!opt_arg)
                SWISH_CROAK("no value for option tag '%s'", opt_name);

            /* append value/args to any existing names in config */
            if (xmlHashLookup(config->misc, opt_name))
            {
                /* err(202,"Bad 'name' tag contents in config: already
                 * seen '%s'\n", opt_name); */
                vhash = xmlHashLookup(config->misc, opt_name);
                if (vhash == NULL)
                {
                    SWISH_CROAK("error with existing name in config: %s", opt_name);
                }
                else
                {
                    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
                        SWISH_DEBUG_MSG(" >>> found existing name in config: %s", opt_name);

                    name_seen = 1;
                }

            }
            else
            {
                name_seen = 0;

                vhash = swish_new_hash(16);    /* values => args */
                if (vhash == NULL)
                    SWISH_CROAK("error creating vhash");

            }


            /* if we reference another config file, load it immediately but
             * don't load into hash, as we might have multiple config files
             * nested */
            if (xmlStrEqual(opt_name, (xmlChar *) SWISH_INCLUDE_FILE))
            {

                /* TODO deduce relative paths */
                swish_parse_config(opt_arg, config);
                swish_xfree(opt_name);
                xmlFree(opt_arg);
                continue;

            }

            /* split arg and value into words/phrases and add each one to
             * hash. This allows for multiple values in a single opt_name */


            arg_list = swish_make_stringlist(opt_arg);

            for (i = 0; i < arg_list->n; i++)
            {

                free_tmp = 0;
                tmp_arg = arg_list->word[i];

                if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
                    SWISH_DEBUG_MSG("config %s tmp_arg = %s  opt_type = %s", opt_name, tmp_arg, opt_type);

                tmp_value = opt_type ? swish_xstrdup(opt_type) : swish_xstrdup(tmp_arg);

                /* normalize all metanames and properties since we
                 * compare against lc tag names */
                if (xmlStrEqual(opt_name, (xmlChar *) SWISH_META)
                    ||
                    xmlStrEqual(opt_name, (xmlChar *) SWISH_PROP)

                    )
                {
                    free_tmp = 1;
                    if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
                        SWISH_DEBUG_MSG("tolower str: >%s<", tmp_arg);

                    tmp_arg = swish_str_tolower(tmp_arg); // TODO mem leak !! ??

                }

                if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
                    SWISH_DEBUG_MSG("config %s tmp_arg = %s  tmp_value = %s", opt_name, tmp_arg, tmp_value);

                if (xmlHashLookup(vhash, tmp_arg))
                    swish_hash_replace(vhash, tmp_arg, tmp_value);
                else
                    swish_hash_add(vhash, tmp_arg, tmp_value);

                if (free_tmp)
                    swish_xfree(tmp_arg);


            }

            swish_free_stringlist(arg_list);

            /* don't use our swish_xfree since it throws off memcount */
            xmlFree(opt_arg);

            if (opt_type != NULL)
                xmlFree(opt_type);


            /* add vhash to config hash unless we already have or it's just
             * calling another config file */
            if (!name_seen && xmlStrcmp(opt_name, (xmlChar *) "IncludeConfigFile"))
            {
                if (SWISH_DEBUG >= SWISH_DEBUG_CONFIG)
                    SWISH_DEBUG_MSG(" >>> adding %s to config hash ( name_seen = %d )", opt_name, name_seen);

                swish_hash_add(config->misc, opt_name, vhash);
            }


            swish_xfree(opt_name);

        }        /* end XML_ELEMENT_NODE */

    }            /* end cur_node */

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return config;
}


static void
config_printer(xmlChar *val, xmlChar *str, xmlChar *key)
{
    SWISH_DEBUG_MSG(" %s:  %s => %s", str, key, val);
}

static void
property_printer(swish_Property *prop, xmlChar *str, xmlChar *propname)
{
    SWISH_DEBUG_MSG(" %s:  %s =>", str, propname);
    swish_debug_property(prop);
}

static void
metaname_printer(swish_MetaName *meta, xmlChar *str, xmlChar *metaname)
{
    SWISH_DEBUG_MSG(" %s:  %s =>", str, metaname);
    swish_debug_metaname(meta);
}

/* PUBLIC */
void
swish_debug_config(swish_Config * config)
{
    SWISH_DEBUG_MSG("config->ref_cnt = %d", config->ref_cnt);
    SWISH_DEBUG_MSG("config->stash address = 0x%x  %d", (int) config->stash, (int) config->stash);
    SWISH_DEBUG_MSG("ptr addr: 0x%x  %d", (int) config, (int) config);

    xmlHashScan(config->misc,       (xmlHashScanner)config_printer,     "misc conf");
    xmlHashScan(config->properties, (xmlHashScanner)property_printer,   "properties");
    xmlHashScan(config->metanames,  (xmlHashScanner)metaname_printer,   "metanames");
    xmlHashScan(config->parsers,    (xmlHashScanner)config_printer,     "parsers");
    xmlHashScan(config->mimes,      (xmlHashScanner)config_printer,     "mimes");
    xmlHashScan(config->index,      (xmlHashScanner)config_printer,     "index");
    xmlHashScan(config->tag_aliases,(xmlHashScanner)config_printer,     "tag_aliases");
}

static void
copy_property( xmlHashTablePtr props1, swish_Property *prop2, xmlChar *prop2name )
{
    swish_Property *prop1;
    boolean in_hash;
    if (swish_hash_exists(props1, prop2name)) {
        prop1 = swish_hash_fetch(props1, prop2name);
        in_hash = 1;
    }
    else {
        prop1 = swish_init_property(swish_xstrdup(prop2name));
        in_hash = 0;
    }
    
    prop1->id            = prop2->id;
    if (prop1->name != NULL) {
        swish_xfree( prop1->name );
    }
    prop1->name          = swish_xstrdup( prop2->name );
    prop1->ignore_case   = prop2->ignore_case;
    prop1->type          = prop2->type;
    prop1->verbatim      = prop2->verbatim;
    if (prop1->alias_for != NULL) {
        swish_xfree( prop2->alias_for );
    }
    prop1->alias_for     = swish_xstrdup( prop2->alias_for );
    prop1->max           = prop2->max;
    prop1->sort          = prop2->sort;
    
    if (!in_hash) {
        swish_hash_add(props1, prop1->name, prop1);
    }
}

static void
merge_properties(xmlHashTablePtr props1, xmlHashTablePtr props2)
{
    xmlHashScan(props2, (xmlHashScanner)copy_property, props1);
}

static void
copy_metaname( xmlHashTablePtr metas1, swish_MetaName *meta2, xmlChar *meta2name )
{
    swish_MetaName *meta1;
    boolean in_hash;
    if (swish_hash_exists(metas1, meta2name)) {
        meta1 = swish_hash_fetch(metas1, meta2name);
        in_hash = 1;
    }
    else {
        meta1 = swish_init_metaname(swish_xstrdup(meta2name));
        in_hash = 0;
    }

    meta1->id           = meta2->id;
    if (meta1->name != NULL) {
        swish_xfree(meta1->name);
    }
    meta1->name         = swish_xstrdup( meta2->name );
    meta1->bias         = meta2->bias;
    if (meta1->alias_for != NULL) {
        swish_xfree(meta1->alias_for);
    }
    meta1->alias_for    = swish_xstrdup( meta2->alias_for );
    
    if (!in_hash) {
        swish_hash_add(metas1, meta1->name, meta1);
    }
}

static void
merge_metanames(xmlHashTablePtr metas1, xmlHashTablePtr metas2)
{
    xmlHashScan(metas2, (xmlHashScanner)copy_metaname, metas1);
}

void
swish_config_merge(swish_Config *config1, swish_Config *config2)
{
    /* values in config2 override and are set in config1 */
    merge_properties(config1->properties,   config2->properties);
    merge_metanames(config1->metanames,     config2->metanames);
    swish_hash_merge(config1->parsers,      config2->parsers);
    swish_hash_merge(config1->mimes,        config2->mimes);
    swish_hash_merge(config1->index,        config2->index);
    swish_hash_merge(config1->tag_aliases,  config2->tag_aliases);
    swish_hash_merge(config1->misc,         config2->misc);

    /* set flags */
    config1->flags->tokenize = config2->flags->tokenize;
    
}

