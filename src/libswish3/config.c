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

#ifndef LIBSWISH3_SINGLE_FILE
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"
#endif

extern int SWISH_DEBUG;

static void free_string(
    xmlChar *payload,
    xmlChar *key
);
static void free_props(
    swish_Property *prop,
    xmlChar *propname
);
static void free_metas(
    swish_MetaName *meta,
    xmlChar *metaname
);
static void config_printer(
    xmlChar *val,
    xmlChar *str,
    xmlChar *key
);
static void stringlist_printer(
    swish_StringList *strlist,
    xmlChar *str,
    xmlChar *key
);
static void property_printer(
    swish_Property *prop,
    xmlChar *str,
    xmlChar *propname
);
static void metaname_printer(
    swish_MetaName *meta,
    xmlChar *str,
    xmlChar *metaname
);
static void copy_property(
    swish_Property *prop2,
    xmlHashTablePtr props1,
    xmlChar *prop2name
);
static void merge_properties(
    xmlHashTablePtr props1,
    xmlHashTablePtr props2
);
static void copy_metaname(
    swish_MetaName *meta2,
    xmlHashTablePtr metas1,
    xmlChar *meta2name
);
static void merge_metanames(
    xmlHashTablePtr metas1,
    xmlHashTablePtr metas2
);
static void
free_stringlist(
    swish_StringList *strlist,
    xmlChar *key
);

static void
free_string(
    xmlChar *payload,
    xmlChar *key
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("   freeing config %s => %s", key, payload);

    swish_xfree(payload);
}

static void
free_stringlist(
    swish_StringList *strlist,
    xmlChar *key
)
{
    int i;
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("   freeing config->stringlists %s [%d strings]", key, strlist->n);
        for(i=0; i<strlist->n; i++) {
            SWISH_DEBUG_MSG("     string: %s", strlist->word[i]);
        }
    }

    swish_stringlist_free(strlist);
}

static void
free_props(
    swish_Property *prop,
    xmlChar *propname
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("   freeing config->prop %s", propname);
        swish_property_debug((swish_Property *)prop);
    }
    prop->ref_cnt--;
    if (prop->ref_cnt < 1) {
        swish_property_free(prop);
    }
}

static void
free_metas(
    swish_MetaName *meta,
    xmlChar *metaname
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG(" freeing config->meta %s", metaname);
        swish_metaname_debug((swish_MetaName *)meta);
    }
    meta->ref_cnt--;
    if (meta->ref_cnt < 1) {
        swish_metaname_free(meta);
    }
}

void
swish_config_free(
    swish_Config *config
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("freeing config");
        SWISH_DEBUG_MSG("ptr addr: 0x%x  %d", (long int)config, (long int)config);
        swish_mem_debug();
    }

    xmlHashFree(config->misc, (xmlHashDeallocator)free_string);
    xmlHashFree(config->properties, (xmlHashDeallocator)free_props);
    xmlHashFree(config->metanames, (xmlHashDeallocator)free_metas);
    xmlHashFree(config->tag_aliases, (xmlHashDeallocator)free_string);
    xmlHashFree(config->parsers, (xmlHashDeallocator)free_string);
    xmlHashFree(config->mimes, (xmlHashDeallocator)free_string);
    xmlHashFree(config->index, (xmlHashDeallocator)free_string);
    xmlHashFree(config->stringlists, (xmlHashDeallocator)free_stringlist);
    swish_config_flags_free(config->flags);

    if (config->ref_cnt != 0) {
        SWISH_WARN("config ref_cnt != 0: %d", config->ref_cnt);
    }

    if (config->stash != NULL) {
        SWISH_WARN("possible memory leak: config->stash was not freed");
    }

    swish_xfree(config);
}

swish_ConfigFlags *
swish_config_init_flags(
)
{
    swish_ConfigFlags *flags;
    flags = swish_xmalloc(sizeof(swish_ConfigFlags));
    flags->tokenize = SWISH_TRUE;
    flags->cascade_meta_context = SWISH_FALSE;  /* add tokens to every metaname in the stack */
    flags->ignore_xmlns = SWISH_TRUE;
    flags->follow_xinclude = SWISH_TRUE;
    flags->undef_metas = SWISH_UNDEF_METAS_INDEX;
    flags->undef_attrs = SWISH_UNDEF_ATTRS_DISABLE;
    flags->max_meta_id = -1;
    flags->max_prop_id = -1;
    flags->meta_ids = swish_hash_init(8);
    flags->prop_ids = swish_hash_init(8);
    //flags->contexts = swish_hash_init(8); // TODO cache these to save malloc/frees

    return flags;
}

void
swish_config_flags_free(
    swish_ConfigFlags * flags
)
{
    /*
       these hashes are for convenience and are really freed in swish_config_free() 
     */
    xmlHashFree(flags->meta_ids, NULL);
    xmlHashFree(flags->prop_ids, NULL);
    if (SWISH_DEBUG) {
        swish_config_flags_debug(flags);
    }
    swish_xfree(flags);
}

void
swish_config_flags_debug(
    swish_ConfigFlags *flags
)
{
    SWISH_DEBUG_MSG("config->tokenize == %d", flags->tokenize);
    SWISH_DEBUG_MSG("config->cascade_meta_context == %d", flags->cascade_meta_context);
    SWISH_DEBUG_MSG("config->ignore_xmlns == %d", flags->ignore_xmlns);
    SWISH_DEBUG_MSG("config->follow_xinclude == %d", flags->follow_xinclude);
    SWISH_DEBUG_MSG("config->undef_metas == %d", flags->undef_metas);
    SWISH_DEBUG_MSG("config->undef_attrs == %d", flags->undef_attrs);
    SWISH_DEBUG_MSG("config->max_meta_id == %d", flags->max_meta_id);
    SWISH_DEBUG_MSG("config->max_prop_id == %d", flags->max_prop_id);
}

/* init config object */
swish_Config *
swish_config_init(
)
{
    swish_Config *config;

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("init config");
    }

/* the hashes will automatically grow as needed so we init with sane starting size */
    config = swish_xmalloc(sizeof(swish_Config));
    config->flags = swish_config_init_flags();
    config->misc = swish_hash_init(8);
    config->metanames = swish_hash_init(8);
    config->properties = swish_hash_init(8);
    config->parsers = swish_hash_init(8);
    config->index = swish_hash_init(8);
    config->tag_aliases = swish_hash_init(8);   /* alias => real */
    config->stringlists = swish_hash_init(8);
    config->mimes = NULL;
    config->ref_cnt = 0;
    config->stash = NULL;

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("config ptr 0x%x", (long int)config);
    }

    return config;

}

void
swish_config_set_default(
    swish_Config *config
)
{
    swish_Property *tmpprop;
    swish_MetaName *tmpmeta;
    xmlChar *tmpbuf;

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("setting default config");

/* we xstrdup a lot in order to consistently free in swish_config_free() */

/* MIME types */
    config->mimes = swish_mime_defaults();

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("mime hash set");

/* metanames */
    // default
    tmpmeta = swish_metaname_init(swish_xstrdup((xmlChar *)SWISH_DEFAULT_METANAME));
    tmpmeta->ref_cnt++;
    tmpmeta->id = SWISH_META_DEFAULT_ID;
    tmpbuf = swish_int_to_string(SWISH_META_DEFAULT_ID);
    swish_hash_add(config->flags->meta_ids, tmpbuf, tmpmeta);
    swish_hash_add(config->metanames, (xmlChar *)SWISH_DEFAULT_METANAME, tmpmeta);
    swish_xfree(tmpbuf);
    config->flags->max_meta_id = tmpmeta->id;
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("swishdefault metaname set");
    }
    
    // title
    tmpmeta = swish_metaname_init(swish_xstrdup((xmlChar *)SWISH_TITLE_METANAME));
    tmpmeta->ref_cnt++;
    tmpmeta->id = SWISH_META_TITLE_ID;
    tmpbuf = swish_int_to_string(SWISH_META_TITLE_ID);
    swish_hash_add(config->flags->meta_ids, tmpbuf, tmpmeta);
    swish_hash_add(config->metanames, (xmlChar *)SWISH_TITLE_METANAME, tmpmeta);
    swish_xfree(tmpbuf);
    config->flags->max_meta_id = tmpmeta->id;
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("swishtitle metaname set");
    }
    
/* properties */
    // description
    tmpprop = swish_property_init(swish_xstrdup((xmlChar *)SWISH_PROP_DESCRIPTION));
    tmpprop->ref_cnt++;
    tmpprop->id = SWISH_PROP_DESCRIPTION_ID;
    tmpprop->sort = SWISH_FALSE;
    swish_hash_add(config->properties, (xmlChar *)SWISH_PROP_DESCRIPTION, tmpprop);
    tmpbuf = swish_int_to_string(SWISH_PROP_DESCRIPTION_ID);
    swish_hash_add(config->flags->prop_ids, tmpbuf, tmpprop);
    swish_xfree(tmpbuf);
    config->flags->max_prop_id = tmpprop->id;
    
    // title
    tmpprop = swish_property_init(swish_xstrdup((xmlChar *)SWISH_PROP_TITLE));
    tmpprop->ref_cnt++;
    tmpprop->id = SWISH_PROP_TITLE_ID;
    swish_hash_add(config->properties, (xmlChar *)SWISH_PROP_TITLE, tmpprop);
    tmpbuf = swish_int_to_string(SWISH_PROP_TITLE_ID);
    swish_hash_add(config->flags->prop_ids, tmpbuf, tmpprop);
    swish_xfree(tmpbuf);
    config->flags->max_prop_id = tmpprop->id;

/* parsers */
    swish_hash_add(config->parsers, (xmlChar *)"text/plain",
                   swish_xstrdup((xmlChar *)SWISH_PARSER_TXT));
    swish_hash_add(config->parsers, (xmlChar *)"application/xml",
                   swish_xstrdup((xmlChar *)SWISH_PARSER_XML));
    swish_hash_add(config->parsers, (xmlChar *)"text/xml",
                   swish_xstrdup((xmlChar *)SWISH_PARSER_XML));
    swish_hash_add(config->parsers, (xmlChar *)"text/html",
                   swish_xstrdup((xmlChar *)SWISH_PARSER_HTML));
    swish_hash_add(config->parsers, (xmlChar *)SWISH_DEFAULT_PARSER,
                   swish_xstrdup((xmlChar *)SWISH_DEFAULT_PARSER_TYPE));

/* index */
    swish_hash_add(config->index, (xmlChar *)SWISH_INDEX_FORMAT,
                   swish_xstrdup((xmlChar *)SWISH_INDEX_FILEFORMAT));
    swish_hash_add(config->index, (xmlChar *)SWISH_INDEX_NAME,
                   swish_xstrdup((xmlChar *)SWISH_INDEX_FILENAME));
    swish_hash_add(config->index, (xmlChar *)SWISH_INDEX_LOCALE,
                   swish_xstrdup((xmlChar *)setlocale(LC_CTYPE, NULL)));

/* aliases: other names a tag might be known as, for matching properties and
     * metanames */
    swish_hash_add(config->tag_aliases, (xmlChar *)SWISH_TITLE_TAG,
                   swish_xstrdup((xmlChar *)SWISH_TITLE_METANAME));
    swish_hash_add(config->tag_aliases, (xmlChar *)SWISH_BODY_TAG,
                   swish_xstrdup((xmlChar *)SWISH_PROP_DESCRIPTION));

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("config_set_default done");
        swish_config_debug(config);
    }

}

swish_Config *
swish_config_add(
    swish_Config *config,
    xmlChar *conf
)
{
    config = swish_config_parse(config, conf);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        swish_config_debug(config);

    return config;
}

swish_Config *
swish_config_parse(
    swish_Config *config,
    xmlChar *conf
)
{
    swish_header_merge((char *)conf, config);
    return config;
}

static void
config_printer(
    xmlChar *val,
    xmlChar *str,
    xmlChar *key
)
{
    SWISH_DEBUG_MSG(" %s:  %s => %s", str, key, val);
}

static void 
stringlist_printer(
    swish_StringList *strlist,
    xmlChar *str,
    xmlChar *key
)
{
    int i;
    for(i=0; i<strlist->n; i++) {
        SWISH_DEBUG_MSG(" %s: %s => %s", str, key, strlist->word[i]);
    }
}

static void
property_printer(
    swish_Property *prop,
    xmlChar *str,
    xmlChar *propname
)
{
    SWISH_DEBUG_MSG(" %s:  %s =>", str, propname);
    swish_property_debug(prop);
}

static void
metaname_printer(
    swish_MetaName *meta,
    xmlChar *str,
    xmlChar *metaname
)
{
    SWISH_DEBUG_MSG(" %s:  %s =>", str, metaname);
    swish_metaname_debug(meta);
}

/* PUBLIC */
void
swish_config_debug(
    swish_Config *config
)
{
    SWISH_DEBUG_MSG("config->ref_cnt = %d", config->ref_cnt);
    SWISH_DEBUG_MSG("config->stash address = 0x%x  %d", (long int)config->stash,
                    (long int)config->stash);
    SWISH_DEBUG_MSG("ptr addr: 0x%x  %d", (long int)config, (long int)config);

    xmlHashScan(config->misc, (xmlHashScanner)config_printer, "misc conf");
    xmlHashScan(config->stringlists, (xmlHashScanner)stringlist_printer, "stringlists");
    xmlHashScan(config->properties, (xmlHashScanner)property_printer, "properties");
    xmlHashScan(config->metanames, (xmlHashScanner)metaname_printer, "metanames");
    xmlHashScan(config->parsers, (xmlHashScanner)config_printer, "parsers");
    xmlHashScan(config->mimes, (xmlHashScanner)config_printer, "mimes");
    xmlHashScan(config->index, (xmlHashScanner)config_printer, "index");
    xmlHashScan(config->tag_aliases, (xmlHashScanner)config_printer, "tag_aliases");
    swish_config_flags_debug(config->flags);
}

static void
copy_property(
    swish_Property *prop2,
    xmlHashTablePtr props1,
    xmlChar *prop2name
)
{
    swish_Property *prop1;

    if (swish_hash_exists(props1, prop2name)) {
        prop1 = swish_hash_fetch(props1, prop2name);
        if (prop1->name != NULL) {
            swish_xfree(prop1->name);
            prop1->name = swish_xstrdup(prop2->name);
        }
    }
    else {
        prop1 = swish_property_init(swish_xstrdup(prop2name));
        prop1->ref_cnt++;
        swish_hash_add(props1, prop1->name, prop1);
    }
/* 
    SWISH_DEBUG_MSG("%s prop1->id = %d    %s prop2->id = %d",
                    prop1->name, prop1->id, prop2->name, prop2->id);
 */
    prop1->id = prop2->id;        
    prop1->ignore_case = prop2->ignore_case;
    prop1->type = prop2->type;
    prop1->verbatim = prop2->verbatim;
    if (prop1->alias_for != NULL) {
        swish_xfree(prop1->alias_for);
    }
    if (prop2->alias_for != NULL) {
        prop1->alias_for = swish_xstrdup(prop2->alias_for);
    }
    prop1->max = prop2->max;
    prop1->sort = prop2->sort;

}

static void
merge_properties(
    xmlHashTablePtr props1,
    xmlHashTablePtr props2
)
{
    xmlHashScan(props2, (xmlHashScanner)copy_property, props1);
}

static void
copy_metaname(
    swish_MetaName *meta2,
    xmlHashTablePtr metas1,
    xmlChar *meta2name
)
{
    swish_MetaName *meta1;
    
    if (swish_hash_exists(metas1, meta2name)) {
        meta1 = swish_hash_fetch(metas1, meta2name);
        if (meta1->name != NULL) {
            swish_xfree(meta1->name);
            meta1->name = swish_xstrdup(meta2->name);
        }    
    }
    else {
        meta1 = swish_metaname_init(swish_xstrdup(meta2name));
        meta1->ref_cnt++;
        swish_hash_add(metas1, meta1->name, meta1);
    }
/*     
    SWISH_DEBUG_MSG("%s meta1->id = %d    %s meta2->id = %d",
                    meta1->name, meta1->id, meta2->name, meta2->id);
 */
    // only change id if meta2->id is not already spoken for.
    meta1->id = meta2->id;
    meta1->bias = meta2->bias;
    if (meta1->alias_for != NULL) {
        swish_xfree(meta1->alias_for);
    }
    if (meta2->alias_for != NULL) {
        meta1->alias_for = swish_xstrdup(meta2->alias_for);
    }

}

static void
merge_metanames(
    xmlHashTablePtr metas1,
    xmlHashTablePtr metas2
)
{
    xmlHashScan(metas2, (xmlHashScanner)copy_metaname, metas1);
}

static void
copy_strlist(
    swish_StringList *strlist2,
    xmlHashTablePtr strlists1,
    xmlChar *key
)
{
    swish_StringList *strlist1;
    if (swish_hash_exists(strlists1, key)) {
        strlist1 = swish_hash_fetch(strlists1, key);
        swish_stringlist_merge(strlist2, strlist1);
    }
    else {
        strlist1 = swish_stringlist_copy(strlist2);
        swish_hash_add(strlists1, key, strlist1);
    }
}

static void
merge_stringlists(
    xmlHashTablePtr strlists1,
    xmlHashTablePtr strlists2
)
{
    xmlHashScan(strlists2, (xmlHashScanner)copy_strlist, strlists1);
}

void
swish_config_merge(
    swish_Config *config1,
    swish_Config *config2
)
{

    xmlChar *v;

/* values in config2 override and are set in config1 */

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("Merging config2 0x%lx into config1 0x%lx",
            config2, config1);
        swish_config_debug(config2);
        swish_config_debug(config1);
    }


    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge properties");
    }
    merge_properties(config1->properties, config2->properties);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge metanames");
    }
    merge_metanames(config1->metanames, config2->metanames);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge parsers");
    }
    swish_hash_merge(config1->parsers, config2->parsers);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge mimes");
    }
    swish_hash_merge(config1->mimes, config2->mimes);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge index");
    }
    swish_hash_merge(config1->index, config2->index);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge tag_aliases");
    }
    swish_hash_merge(config1->tag_aliases, config2->tag_aliases);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge misc");
    }
    swish_hash_merge(config1->misc, config2->misc);
    
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge stringlists");
    }
    merge_stringlists(config1->stringlists, config2->stringlists);

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("merge complete");
    }

/* set flags */
    if (swish_hash_exists(config2->misc, BAD_CAST SWISH_TOKENIZE)) {
        config2->flags->tokenize = swish_string_to_boolean(swish_hash_fetch(config2->misc, BAD_CAST SWISH_TOKENIZE));
    }
    config1->flags->tokenize = config2->flags->tokenize;
    if (swish_hash_exists(config2->misc, BAD_CAST SWISH_CASCADE_META_CONTEXT)) {
        config2->flags->cascade_meta_context = 
            swish_string_to_boolean(swish_hash_fetch(config2->misc, BAD_CAST SWISH_CASCADE_META_CONTEXT));
    }
    config1->flags->cascade_meta_context = config2->flags->cascade_meta_context;
    if (swish_hash_exists(config2->misc, BAD_CAST SWISH_IGNORE_XMLNS)) {
        config2->flags->ignore_xmlns = 
            swish_string_to_boolean(swish_hash_fetch(config2->misc, BAD_CAST SWISH_IGNORE_XMLNS));
    }
    config1->flags->ignore_xmlns = config2->flags->ignore_xmlns;
    if (swish_hash_exists(config2->misc, BAD_CAST SWISH_FOLLOW_XINCLUDE)) {
        config2->flags->follow_xinclude =
            swish_string_to_boolean(swish_hash_fetch(config2->misc, BAD_CAST SWISH_FOLLOW_XINCLUDE));
    }
    config1->flags->follow_xinclude = config2->flags->follow_xinclude;
    if (swish_hash_exists(config2->misc, BAD_CAST SWISH_UNDEFINED_METATAGS)) {
        v = swish_hash_fetch(config2->misc, BAD_CAST SWISH_UNDEFINED_METATAGS);
        if (xmlStrEqual(v, BAD_CAST "error")) {
            config2->flags->undef_metas = SWISH_UNDEF_METAS_ERROR;
        }
        else if (xmlStrEqual(v, BAD_CAST "ignore")) {
            config2->flags->undef_metas = SWISH_UNDEF_METAS_IGNORE;
        }
        else if (xmlStrEqual(v, BAD_CAST "index")) {
            config2->flags->undef_metas = SWISH_UNDEF_METAS_INDEX;
        }
        else if (xmlStrEqual(v, BAD_CAST "auto")) {
            config2->flags->undef_metas = SWISH_UNDEF_METAS_AUTO;
        }
        else {
            SWISH_CROAK("Unknown value for %s: %s", SWISH_UNDEFINED_METATAGS, v);
        }
    }
    config1->flags->undef_metas = config2->flags->undef_metas;
    if (swish_hash_exists(config2->misc, BAD_CAST SWISH_UNDEFINED_XML_ATTRIBUTES)) {
        v = swish_hash_fetch(config2->misc, BAD_CAST SWISH_UNDEFINED_XML_ATTRIBUTES);
        if (xmlStrEqual(v, BAD_CAST "error")) {
            config2->flags->undef_attrs = SWISH_UNDEF_ATTRS_ERROR;
        }
        else if (xmlStrEqual(v, BAD_CAST "ignore")) {
            config2->flags->undef_attrs = SWISH_UNDEF_ATTRS_IGNORE;
        }
        else if (xmlStrEqual(v, BAD_CAST "index")) {
            config2->flags->undef_attrs = SWISH_UNDEF_ATTRS_INDEX;
        }
        else if (xmlStrEqual(v, BAD_CAST "auto")) {
            config2->flags->undef_attrs = SWISH_UNDEF_ATTRS_AUTO;
        }
        else if (xmlStrEqual(v, BAD_CAST "disable")) {
            config2->flags->undef_attrs = SWISH_UNDEF_ATTRS_DISABLE;
        }
        else {
            SWISH_CROAK("Unknown value for %s: %s", SWISH_UNDEFINED_XML_ATTRIBUTES, v);
        }
    }
    config1->flags->undef_attrs = config2->flags->undef_attrs;
    
    if (config1->flags->max_meta_id < config2->flags->max_meta_id) {
        config1->flags->max_meta_id = config2->flags->max_meta_id;
    }
    if (config1->flags->max_prop_id < config2->flags->max_prop_id) {
        config1->flags->max_prop_id = config2->flags->max_prop_id;
    }


    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("flags set");
    }

}
