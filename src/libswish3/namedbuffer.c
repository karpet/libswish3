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

/* named buffers are just a hash where each key is a text buffer
*/

#ifndef LIBSWISH3_SINGLE_FILE
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"
#endif

extern int SWISH_DEBUG;

static void free_name_from_hash(
    void *buffer,
    xmlChar *name
);
static void add_name_to_hash(
    void *ignored,
    xmlHashTablePtr nbhash,
    xmlChar *name
);
static void print_buffer(
    xmlBufferPtr buffer,
    xmlChar *label,
    xmlChar *name
);
static void
cat_buffer(
    xmlBufferPtr buffer,
    swish_NamedBuffer *nb2,
    xmlChar *name
);

static void
add_name_to_hash(
    void *ignored,
    xmlHashTablePtr nbhash,
    xmlChar *name
)
{
/* make sure we don't already have it */
    if (swish_hash_exists(nbhash, name)) {
        SWISH_WARN("%s is already in NamedBuffer hash -- ignoring", name);
        return;
    }

    if (SWISH_DEBUG == SWISH_DEBUG_NAMEDBUFFER)
        SWISH_DEBUG_MSG("  adding %s to NamedBuffer\n", name);

    swish_hash_add(nbhash, name, xmlBufferCreateSize((size_t) SWISH_BUFFER_CHUNK_SIZE));
}

static void
free_name_from_hash(
    void *buffer,
    xmlChar *name
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG(" freeing NamedBuffer %s\n", name);

    xmlBufferFree(buffer);
}

swish_NamedBuffer *
swish_nb_init(
    xmlHashTablePtr confhash
)
{
    swish_NamedBuffer *nb = swish_xmalloc(sizeof(swish_NamedBuffer));
    nb->stash = NULL;
    nb->ref_cnt = 0;
    nb->hash = xmlHashCreate(8);        /* will grow as needed */

/* init a buffer for each key in confhash. Note that this inits hashes for alias_for
   PropertyNames and MetaNames, which we will never use. But it's easier this way.
*/
    xmlHashScan(confhash, (xmlHashScanner)add_name_to_hash, nb->hash);

    return nb;
}

void
swish_nb_free(
    swish_NamedBuffer * nb
)
{
    xmlHashFree(nb->hash, (xmlHashDeallocator)free_name_from_hash);

    if (nb->ref_cnt != 0) {
        SWISH_WARN("freeing NamedBuffer with ref_cnt != 0 (%d)", nb->ref_cnt);
    }

    if (nb->stash != NULL)
        SWISH_WARN("freeing NamedBuffer with non-null stash");

    swish_xfree(nb);
}

static void
print_buffer(
    xmlBufferPtr buffer,
    xmlChar *label,
    xmlChar *name
)
{
    const xmlChar *substr;
    const xmlChar *buf;
    int sub_len;

    SWISH_DEBUG_MSG("len=%d %s:<%s>%s</%s>", xmlBufferLength(buffer), 
                        label, name, xmlBufferContent(buffer), name);
    
    buf = xmlBufferContent(buffer);
    while ((substr = xmlStrstr(buf, (const xmlChar *)SWISH_TOKENPOS_BUMPER)) != NULL) {
        sub_len = substr - buf;
        SWISH_DEBUG_MSG("  len=%d <%s>%s</%s>", sub_len, name, xmlStrsub(buf, 0, sub_len), name );
        buf = substr + 1;
    }
    if (buf != NULL) {
        SWISH_DEBUG_MSG("  len=%d <%s>%s</%s>", xmlStrlen(buf), name, buf, name );
    }
}

void
swish_nb_debug(
    swish_NamedBuffer * nb,
    xmlChar *label
)
{
    xmlHashScan(nb->hash, (xmlHashScanner)print_buffer, label);
}

void
swish_nb_add_buf(
    swish_NamedBuffer * nb,
    xmlChar *name,
    xmlBufferPtr buf,
    xmlChar *joiner,
    boolean cleanwsp,
    boolean autovivify
)
{
    swish_nb_add_str(nb, name, (xmlChar *)xmlBufferContent(buf), xmlBufferLength(buf),
                        joiner, cleanwsp, autovivify);
}

void
swish_nb_add_str(
    swish_NamedBuffer * nb,
    xmlChar *name,
    xmlChar *str,
    unsigned int len,
    xmlChar *joiner,
    boolean cleanwsp,
    boolean autovivify
)
{
    xmlChar *nowhitesp;
    xmlBufferPtr buf = swish_hash_fetch(nb->hash, name);

/* if the str is nothing but whitespace, skip it */
    if (swish_str_all_ws(str)) {
        if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER)
            SWISH_DEBUG_MSG("skipping all whitespace string '%s'", str);

        return;
    }

    if (!buf) {
        if (autovivify) {
/* spring to life */
            add_name_to_hash(NULL, nb->hash, name);
            buf = swish_hash_fetch(nb->hash, name);
        }

        if (!buf)
            SWISH_CROAK("%s is not a named buffer", name);

    }

/* if the buf already exists and we're about to add more, append the joiner */
    if (xmlBufferLength(buf)) {
        if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
            SWISH_DEBUG_MSG("appending string joiner to '%s' buffer", name);
        }
        swish_buffer_append(buf, joiner, xmlStrlen(joiner));
    }

    if (cleanwsp) {
        if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
            SWISH_DEBUG_MSG("before cleanwsp: '%s'", str);
        }
        swish_str_ctrl_to_ws(str);
        nowhitesp = swish_str_skip_ws(str);
        swish_str_trim_ws(nowhitesp);
        if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
            SWISH_DEBUG_MSG("after  cleanwsp: adding '%s' to buffer '%s'", nowhitesp, name);
        }
        swish_buffer_append(buf, nowhitesp, xmlStrlen(nowhitesp));
    }
    else {
        if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
            SWISH_DEBUG_MSG("adding '%s' to buffer '%s'", str, name);
        }
        swish_buffer_append(buf, str, len);
    }

}

void
swish_buffer_append(
    xmlBufferPtr buf,
    xmlChar *txt,
    int txtlen
)
{
    int ret;

/* shouldn't happen */
    if (txtlen == 0) {
        return;
    }
    
    if (buf == NULL) {
        SWISH_CROAK("Can't append NULL pointer to buffer.");
    }

    ret = xmlBufferAdd(buf, (const xmlChar *)txt, txtlen);
    if (ret) {
        SWISH_CROAK("Problem adding \n>>%s<<\n length %d to buffer. Err: %d", txt, txtlen,
                    ret);
    }
}

static void
cat_buffer(
    xmlBufferPtr buffer,
    swish_NamedBuffer *nb2,
    xmlChar *name
)
{
    xmlChar *buf2;
    buf2 = swish_nb_get_value(nb2, name);
    if (xmlStrlen(buf2)) {
        if (xmlBufferLength(buffer)) {
            xmlBufferCat(buffer, (xmlChar *)SWISH_TOKENPOS_BUMPER);
        }
        xmlBufferCat(buffer, buf2);
    }
}

void
swish_buffer_concat(
    swish_NamedBuffer *nb1,
    swish_NamedBuffer *nb2
)
{
    xmlHashScan(nb1->hash, (xmlHashScanner)cat_buffer, nb2);
}

xmlChar *
swish_nb_get_value(
    swish_NamedBuffer *nb,
    xmlChar *key
)
{
    xmlBufferPtr buf;
    buf = swish_hash_fetch(nb->hash, key);
    return (xmlChar *)xmlBufferContent(buf);
}
