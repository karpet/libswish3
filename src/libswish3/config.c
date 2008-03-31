
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

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

void swish_free_config(
    swish_Config *config
);
swish_Config *swish_init_config(
);
void swish_config_set_default(
    swish_Config *config
);
swish_Config *swish_add_config(
    xmlChar *conf,
    swish_Config *config
);
swish_Config *swish_parse_config(
    xmlChar *conf,
    swish_Config *config
);
void swish_debug_config(
    swish_Config *config
);
void swish_config_merge(
    swish_Config *config1,
    swish_Config *config2
);
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
free_props(
    swish_Property *prop,
    xmlChar *propname
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("   freeing config->prop %s", propname);
        swish_debug_property((swish_Property *)prop);
    }
    prop->ref_cnt--;
    if (prop->ref_cnt < 1) {
        swish_free_property(prop);
    }
}

static void
free_metas(
    swish_MetaName *meta,
    xmlChar *metaname
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("   freeing config->meta %s", metaname);
        swish_debug_metaname((swish_MetaName *)meta);
    }
    meta->ref_cnt--;
    if (meta->ref_cnt < 1) {
        swish_free_metaname(meta);
    }
}

void
swish_free_config(
    swish_Config *config
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("freeing config");
        SWISH_DEBUG_MSG("ptr addr: 0x%x  %d", (int)config, (int)config);
        swish_mem_debug();
    }

    xmlHashFree(config->misc, (xmlHashDeallocator) free_string);
    xmlHashFree(config->properties, (xmlHashDeallocator) free_props);
    xmlHashFree(config->metanames, (xmlHashDeallocator) free_metas);
    xmlHashFree(config->tag_aliases, (xmlHashDeallocator) free_string);
    xmlHashFree(config->parsers, (xmlHashDeallocator) free_string);
    xmlHashFree(config->mimes, (xmlHashDeallocator) free_string);
    xmlHashFree(config->index, (xmlHashDeallocator) free_string);
    swish_xfree(config->flags);

    if (config->ref_cnt != 0) {
        SWISH_WARN("config ref_cnt != 0: %d", config->ref_cnt);
    }

    if (config->stash != NULL) {
        SWISH_WARN("possible memory leak: config->stash was not freed");
    }

    swish_xfree(config);
}

swish_ConfigFlags *
swish_init_config_flags(
)
{
    swish_ConfigFlags   *flags;
    int                  i;
    
    flags               = swish_xmalloc(sizeof(swish_ConfigFlags));
    flags->tokenize     = 1;
       
    /* set all defaults to 0 */
    for(i=0; i<SWISH_MAX_IDS; i++) {
        flags->meta_ids[i] = 0;
    }
    for(i=0; i<SWISH_MAX_IDS; i++) {
        flags->prop_ids[i] = 0;
    }
    
    return flags;
}

    

/* init config object */
swish_Config *
swish_init_config(
)
{
    swish_Config *config;

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("init config");
    }

/* the hashes will automatically grow as needed so we init with sane starting size */
    config = swish_xmalloc(sizeof(swish_Config));
    config->flags = swish_init_config_flags();
    config->misc = swish_init_hash(8);
    config->metanames = swish_init_hash(8);
    config->properties = swish_init_hash(8);
    config->parsers = swish_init_hash(8);
    config->index = swish_init_hash(8);
    config->tag_aliases = swish_init_hash(8);
    config->mimes = NULL;
    config->ref_cnt = 0;
    config->stash = NULL;
    
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("config ptr 0x%x", (int)config);
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

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        SWISH_DEBUG_MSG("setting default config");

/* we xstrdup a lot in order to consistently free in swish_free_config() */

/* MIME types */
    config->mimes = swish_mime_hash();

/* metanames */
    swish_hash_add(config->metanames, (xmlChar *)SWISH_DEFAULT_METANAME,
                   swish_init_metaname(swish_xstrdup
                                       ((xmlChar *)SWISH_DEFAULT_METANAME))
        );
    swish_hash_add(config->metanames, (xmlChar *)SWISH_TITLE_METANAME,
                   swish_init_metaname(swish_xstrdup
                                       ((xmlChar *)SWISH_TITLE_METANAME))
        );

/* alter swish_MetaName objects after they've been stashed.
       a little awkward, but saves var names.
*/
    tmpmeta =
        swish_hash_fetch(config->metanames, (xmlChar *)SWISH_DEFAULT_METANAME);
    tmpmeta->ref_cnt++;
    tmpmeta->id = SWISH_META_DEFAULT_ID;
    tmpmeta =
        swish_hash_fetch(config->metanames, (xmlChar *)SWISH_TITLE_METANAME);
    tmpmeta->ref_cnt++;
    tmpmeta->id = SWISH_META_TITLE_ID;

/* parsers */
    swish_hash_add(config->parsers, (xmlChar *)"text/plain",
                   swish_xstrdup((xmlChar *)SWISH_PARSER_TXT));
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
                   swish_xstrdup((xmlChar *)setlocale(LC_ALL, "")));

/* properties */
    swish_hash_add(config->properties, (xmlChar *)SWISH_PROP_DESCRIPTION,
                   swish_init_property(swish_xstrdup
                                       ((xmlChar *)SWISH_PROP_DESCRIPTION))
        );
    swish_hash_add(config->properties, (xmlChar *)SWISH_PROP_TITLE,
                   swish_init_property(swish_xstrdup
                                       ((xmlChar *)SWISH_PROP_TITLE))
        );

/* same deal as metanames above */
    tmpprop =
        swish_hash_fetch(config->properties, (xmlChar *)SWISH_PROP_DESCRIPTION);
    tmpprop->ref_cnt++;
    tmpprop->id = SWISH_PROP_DESCRIPTION_ID;
    tmpprop = swish_hash_fetch(config->properties, (xmlChar *)SWISH_PROP_TITLE);
    tmpprop->ref_cnt++;
    tmpprop->id = SWISH_PROP_TITLE_ID;

/* aliases: other names a tag might be known as, for matching properties and
     * metanames */
    swish_hash_add(config->tag_aliases, (xmlChar *)SWISH_TITLE_TAG,
                   swish_xstrdup((xmlChar *)SWISH_TITLE_METANAME));
    swish_hash_add(config->tag_aliases, (xmlChar *)SWISH_BODY_TAG,
                   swish_xstrdup((xmlChar *)SWISH_PROP_DESCRIPTION));

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("config_set_default done");
        swish_debug_config(config);
    }

}

swish_Config *
swish_add_config(
    xmlChar *conf,
    swish_Config *config
)
{

    config = swish_parse_config(conf, config);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG)
        swish_debug_config(config);

    return config;

}

swish_Config *
swish_parse_config(
    xmlChar *conf,
    swish_Config *config
)
{
    swish_merge_config_with_header((char *)conf, config);
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
property_printer(
    swish_Property *prop,
    xmlChar *str,
    xmlChar *propname
)
{
    SWISH_DEBUG_MSG(" %s:  %s =>", str, propname);
    swish_debug_property(prop);
}

static void
metaname_printer(
    swish_MetaName *meta,
    xmlChar *str,
    xmlChar *metaname
)
{
    SWISH_DEBUG_MSG(" %s:  %s =>", str, metaname);
    swish_debug_metaname(meta);
}

/* PUBLIC */
void
swish_debug_config(
    swish_Config *config
)
{
    SWISH_DEBUG_MSG("config->ref_cnt = %d", config->ref_cnt);
    SWISH_DEBUG_MSG("config->stash address = 0x%x  %d", (int)config->stash,
                    (int)config->stash);
    SWISH_DEBUG_MSG("ptr addr: 0x%x  %d", (int)config, (int)config);

    xmlHashScan(config->misc, (xmlHashScanner) config_printer, "misc conf");
    xmlHashScan(config->properties, (xmlHashScanner) property_printer,
                "properties");
    xmlHashScan(config->metanames, (xmlHashScanner) metaname_printer,
                "metanames");
    xmlHashScan(config->parsers, (xmlHashScanner) config_printer, "parsers");
    xmlHashScan(config->mimes, (xmlHashScanner) config_printer, "mimes");
    xmlHashScan(config->index, (xmlHashScanner) config_printer, "index");
    xmlHashScan(config->tag_aliases, (xmlHashScanner) config_printer,
                "tag_aliases");
}

static void
copy_property(
    swish_Property *prop2,
    xmlHashTablePtr props1,
    xmlChar *prop2name
)
{
    swish_Property *prop1;
    boolean in_hash;

    if (swish_hash_exists(props1, prop2name)) {
        prop1 = swish_hash_fetch(props1, prop2name);
        in_hash = 1;
    }
    else {
        prop1 = swish_init_property(swish_xstrdup(prop2name));
        prop1->ref_cnt++;
        in_hash = 0;
    }

    prop1->id = prop2->id;
    if (in_hash && prop1->name != NULL) {
        swish_xfree(prop1->name);
        prop1->name = swish_xstrdup(prop2->name);
    }
    prop1->ignore_case = prop2->ignore_case;
    prop1->type = prop2->type;
    prop1->verbatim = prop2->verbatim;
    if (prop1->alias_for != NULL) {
        swish_xfree(prop2->alias_for);
    }
    if (prop2->alias_for != NULL) {
        prop1->alias_for = swish_xstrdup(prop2->alias_for);
    }
    prop1->max = prop2->max;
    prop1->sort = prop2->sort;

    if (!in_hash) {
        swish_hash_add(props1, prop1->name, prop1);
    }

}

static void
merge_properties(
    xmlHashTablePtr props1,
    xmlHashTablePtr props2
)
{
    xmlHashScan(props2, (xmlHashScanner) copy_property, props1);
}

static void
copy_metaname(
    swish_MetaName *meta2,
    xmlHashTablePtr metas1,
    xmlChar *meta2name
)
{
    swish_MetaName *meta1;
    boolean in_hash;

    if (swish_hash_exists(metas1, meta2name)) {
        meta1 = swish_hash_fetch(metas1, meta2name);
        in_hash = 1;
    }
    else {
        meta1 = swish_init_metaname(swish_xstrdup(meta2name));
        meta1->ref_cnt++;
        in_hash = 0;
    }

    meta1->id = meta2->id;
    if (in_hash && meta1->name != NULL) {
        swish_xfree(meta1->name);
        meta1->name = swish_xstrdup(meta2->name);
    }
    meta1->bias = meta2->bias;
    if (meta1->alias_for != NULL) {
        swish_xfree(meta1->alias_for);
    }
    if (meta2->alias_for != NULL) {
        meta1->alias_for = swish_xstrdup(meta2->alias_for);
    }

    if (!in_hash) {
        swish_hash_add(metas1, meta1->name, meta1);
    }
}

static void
merge_metanames(
    xmlHashTablePtr metas1,
    xmlHashTablePtr metas2
)
{
    xmlHashScan(metas2, (xmlHashScanner) copy_metaname, metas1);
}

void
swish_config_merge(
    swish_Config *config1,
    swish_Config *config2
)
{

/* values in config2 override and are set in config1 */
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
        SWISH_DEBUG_MSG("merge complete");
    }

/* set flags */
    config1->flags->tokenize = config2->flags->tokenize;

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("flags set");
    }

}
