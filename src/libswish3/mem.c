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

/* mem.c -- graceful memory handling */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlstring.h>
#include <err.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

static long int memcount = 0;

void
swish_init_memory(
)
{
    memcount = 0;

}

/* PUBLIC */
/* realloc a block of memory */
void *
swish_xrealloc(
    void *ptr,
    size_t size
)
{
    void *new_ptr = realloc(ptr, size);

    if (new_ptr == NULL)
        SWISH_CROAK("Out of memory (could not reallocate %lu more bytes)!",
                    (unsigned long)size);

    return new_ptr;
}

/* PUBLIC */
void *
swish_xmalloc(
    size_t size
)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        SWISH_CROAK("Out of memory! Can't malloc %lu bytes",
                    (unsigned long)size);

    memcount++;
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("memcount = %ld", memcount);
        SWISH_DEBUG_MSG("xmalloc address: 0x%x", (int)ptr);
    }

    return ptr;
}

xmlChar *
swish_xstrdup(
    const xmlChar *ptr
)
{
    xmlChar *copy;
    memcount++;
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("memcount = %ld", memcount);
    copy = xmlStrdup(ptr);
    if (copy == NULL) 
        SWISH_CROAK("strdup returned NULL for %s", ptr);

    return copy;
}

xmlChar *
swish_xstrndup(
    const xmlChar *ptr,
    int len
)
{
    memcount++;
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("memcount = %ld", memcount);
    return (xmlStrndup(ptr, len));
}

void
swish_xfree(
    void *ptr
)
{
    if (ptr == NULL) {
        SWISH_WARN
            (" >>>>>>>>>>>>>> attempt to free NULL pointer <<<<<<<<<<<<<<");
        return;
    }
    
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing %s 0x%x", (char*)ptr, (int)ptr);

    xmlFree(ptr);

    memcount--;

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("memcount = %ld", memcount);
}

void
swish_mem_debug(
)
{
/* SWISH_DEBUG_MSG("memcount = %ld", memcount); */
    if (memcount > 0)
        SWISH_WARN
            ("memory error: %ld more swish_xmalloc()s or swish_xstrdup()s than swish_xfree()s",
             memcount);

    if (memcount < 0)
        SWISH_WARN("memory error: too many swish_xfree()s %d", memcount);
}
