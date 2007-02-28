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


/* libswish3 properties
 * 
 * a property is a xmlBuffer taken as-is from a doc.
 * these can be stored in an index and
 * sorted & returned as part of results 
 */


#include <libxml/hash.h>
#include <libxml/xmlstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

static void     free_prop_from_hash(void *payload, xmlChar * propName);
static void     add_prop_to_hash(xmlChar * val, xmlHashTablePtr propHash, xmlChar * propName);
static void     print_prop(xmlBufferPtr prop, xmlHashTablePtr propHash, xmlChar * propName);

static void 
add_prop_to_hash(xmlChar * val, xmlHashTablePtr propHash, xmlChar * propName)
{    
    /* make sure we don't already have it */
    if (xmlHashLookup(propHash,propName))
    {
        swish_warn_err("%s is already in Property list -- ignoring", propName);
        return;
    }


    if (SWISH_DEBUG > 2)
        printf("  adding %s to propHash\n", propName);
    
    swish_hash_add(propHash, propName, xmlBufferCreateSize((size_t)SWISH_BUFFER_CHUNK_SIZE));
}

static void 
free_prop_from_hash(void *payload, xmlChar * propName)
{
    if (SWISH_DEBUG > 10)
        printf(" freeing property %s\n", propName);

    xmlBufferFree(payload);
}

xmlHashTablePtr
swish_init_PropHash(swish_Config * config)
{    
    xmlHashTablePtr propHash = xmlHashCreate(8);    /* will grow as needed */
    xmlHashTablePtr propsInConfig;
            
    propsInConfig = swish_subconfig_hash( config, (xmlChar *)SWISH_PROP );
                          
    /* add each one to our hash */
    xmlHashScan(propsInConfig, (xmlHashScanner)add_prop_to_hash, propHash);

    return propHash;
}

void
swish_free_PropHash(xmlHashTablePtr propHash)
{
    xmlHashFree(propHash, (xmlHashDeallocator)free_prop_from_hash);
}

static 
void
print_prop(xmlBufferPtr prop, xmlHashTablePtr propHash, xmlChar * propName)
{
    fprintf(stderr,"Property:\n<%s>%s</%s>\n", propName, xmlBufferContent(prop), propName);
}


void swish_debug_PropHash(xmlHashTablePtr propHash)
{
    xmlHashScan(propHash, (xmlHashScanner)print_prop, propHash);
}
