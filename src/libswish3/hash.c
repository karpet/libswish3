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


/* wrappers to common functions in libxml2 hash */

#include <libxml/hash.h>
#include <stdlib.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

static void free_hashval( void *val, xmlChar *key )
{
    if (SWISH_DEBUG>10)
        printf("freeing %s from hash key %s\n", (xmlChar *)val, key);
        
    swish_xfree( val );
}

/* PUBLIC */
int swish_hash_add( xmlHashTablePtr hash, xmlChar *key, void * value )
{
    int ret;
    ret = xmlHashAddEntry( hash, key, value );
    if (ret == -1)
        swish_fatal_err("xmlHashAddEntry for %s failed", key);
                
    return ret;
}

/* PUBLIC */
int swish_hash_replace( xmlHashTablePtr hash, xmlChar *key, void *value )
{
    int ret;
    ret = xmlHashUpdateEntry(hash, key, value, (xmlHashDeallocator)free_hashval );
    if (ret == -1)
        swish_fatal_err("xmlHashUpdateEntry for %s failed", key);
    
    return ret;
}

/* PUBLIC */
int swish_hash_delete( xmlHashTablePtr hash, xmlChar *key )
{
    int ret;
    ret = xmlHashRemoveEntry(hash, key, (xmlHashDeallocator)free_hashval );
    if (ret == -1)
        swish_fatal_err("xmlHashRemoveEntry for %s failed", key);
        
    return ret;
}

xmlHashTablePtr swish_new_hash(int size)
{
    xmlHashTablePtr h = xmlHashCreate(size);
    if (h == NULL)
    {
        swish_fatal_err("error creating hash of size %d", size);
        return NULL;
    }
       
    return h;
}
