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

#include <stdlib.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

int swish_hash_add(
    xmlHashTablePtr hash,
    xmlChar *key,
    void *value
);
int swish_hash_replace(
    xmlHashTablePtr hash,
    xmlChar *key,
    void *value
);
int swish_hash_delete(
    xmlHashTablePtr hash,
    xmlChar *key
);
boolean swish_hash_exists(
    xmlHashTablePtr hash,
    xmlChar *key
);
void *swish_hash_fetch(
    xmlHashTablePtr hash,
    xmlChar *key
);
xmlHashTablePtr swish_init_hash(
    int size
);
void swish_hash_merge(
    xmlHashTablePtr hash1,
    xmlHashTablePtr hash2
);
static void free_hashval(
    void *val,
    xmlChar *key
);
static void merge_hashes(
    xmlChar *value,
    xmlHashTablePtr hash1,
    xmlChar *key
);

static void
free_hashval(
    void *val,
    xmlChar *key
)
{
    if (SWISH_DEBUG > 10)
        printf("freeing %s from hash key %s\n", (xmlChar *)val, key);

    swish_xfree(val);
}

int
swish_hash_add(
    xmlHashTablePtr hash,
    xmlChar *key,
    void *value
)
{
    int ret;
    ret = xmlHashAddEntry(hash, key, value);
    if (ret == -1)
        SWISH_CROAK("xmlHashAddEntry for %s failed", key);

    return ret;
}

int
swish_hash_replace(
    xmlHashTablePtr hash,
    xmlChar *key,
    void *value
)
{
    int ret;
    ret =
        xmlHashUpdateEntry(hash, key, value, (xmlHashDeallocator) free_hashval);
    if (ret == -1)
        SWISH_CROAK("xmlHashUpdateEntry for %s failed", key);

    return ret;
}

int
swish_hash_delete(
    xmlHashTablePtr hash,
    xmlChar *key
)
{
    int ret;
    ret = xmlHashRemoveEntry(hash, key, (xmlHashDeallocator) free_hashval);
    if (ret == -1)
        SWISH_CROAK("xmlHashRemoveEntry for %s failed", key);

    return ret;
}

boolean
swish_hash_exists(
    xmlHashTablePtr hash,
    xmlChar *key
)
{
    return xmlHashLookup(hash, key) ? 1 : 0;
}

void *
swish_hash_fetch(
    xmlHashTablePtr hash,
    xmlChar *key
)
{
    return xmlHashLookup(hash, key);
}

xmlHashTablePtr
swish_init_hash(
    int size
)
{
    xmlHashTablePtr h;

    h = xmlHashCreate(size);
    if (h == NULL) {
        SWISH_CROAK("error creating hash of size %d", size);
        return NULL;
    }

    return h;
}

static void
merge_hashes(
    xmlChar *value,
    xmlHashTablePtr hash1,
    xmlChar *key
)
{
    if (swish_hash_exists(hash1, key)) {
        swish_hash_replace(hash1, key, swish_xstrdup(value));
    }
    else {
        swish_hash_add(hash1, key, swish_xstrdup(value));
    }
}

void
swish_hash_merge(
    xmlHashTablePtr hash1,
    xmlHashTablePtr hash2
)
{
    xmlHashScan(hash2, (xmlHashScanner) merge_hashes, hash1);
}
