/*
 * This file is part of libswish3
 * Copyright (C) 2009 Peter Karman
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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <sys/param.h>
#include <limits.h>
#include <assert.h>

#include "libswish3.h"

#endif

extern int errno;
extern int SWISH_DEBUG;

static xmlChar *findlast(
    xmlChar *str,
    xmlChar *set
);
static xmlChar *lastptr(
    xmlChar *str
);

boolean
swish_fs_file_exists(
    xmlChar *path
)
{
    struct stat info;
    if (stat((char *)path, &info)) {
        return 0;
    }
    return 1;
}

boolean
swish_fs_is_dir(
    xmlChar *path
)
{
    struct stat stbuf;

    if (stat((char *)path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}

boolean
swish_fs_is_file(
    xmlChar *path
)
{
    struct stat stbuf;

    if (stat((char *)path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
}

boolean
swish_fs_is_link(
    xmlChar *path
)
{
#ifdef HAVE_LSTAT
    struct stat stbuf;

    if (lstat((char *)path, &stbuf))
        return 0;
    return ((stbuf.st_mode & S_IFLNK) == S_IFLNK) ? 1 : 0;
#else
    return 0;
#endif
}

off_t
swish_fs_get_file_size(
    xmlChar *path
)
{
    struct stat stbuf;

    if (stat((char *)path, &stbuf))
        return -1;
    return stbuf.st_size;
}

time_t
swish_fs_get_file_mtime(
    xmlChar *path
)
{
    struct stat stbuf;

    if (stat((char *)path, &stbuf))
        return -1;
    return stbuf.st_mtime;
}

/* parse a URL to determine file ext */
/* inspired by http://www.tug.org/tex-archive/tools/zoo/ by Rahul Dhesi */
xmlChar *
swish_fs_get_file_ext(
    xmlChar *url
)
{
    xmlChar *p;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("parsing url %s for extension", url);

    p = findlast(url, (xmlChar *)SWISH_EXT_SEP);        /* look for . or /         */

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("p = %s", p);

    if (p == NULL)
        return p;

    if (p != NULL && *p != SWISH_EXT_CH)        /* found .?                     */
        return NULL;            /* ... if not, ignore / */

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("p = %s", p);

    if (*p == SWISH_EXT_CH)
        p++;                    /* skip to next char after . */

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("ext is %s", p);

    return swish_str_tolower(p);
}

boolean
swish_fs_looks_like_gz(
    xmlChar *file
)
{
    xmlChar *ext;
    boolean is_eq;
    ext = swish_fs_get_file_ext(file);
    is_eq = xmlStrEqual(ext, BAD_CAST "gz");
    //SWISH_DEBUG_MSG("looks like gz? %d", is_eq);
    swish_xfree(ext);
    return is_eq;
}

/*******************/
/*
findlast() finds last occurrence in provided string of any of the characters
except the null character in the provided set.

If found, return value is pointer to character found, else it is NULL.
*/

static xmlChar *
findlast(
    xmlChar *str,
    xmlChar *set
)
{
    xmlChar *p;

    if (str == NULL || set == NULL || *str == '\0' || *set == '\0')
        return (NULL);

    p = lastptr(str);           /* pointer to last char of string */
    assert(p != NULL);

    while (p != str && xmlStrchr(set, *p) == NULL) {
        --p;
    }

/* either p == str or we found a character or both */
    if (xmlStrchr(set, *p) == NULL)
        return (NULL);
    else
        return (p);
}

/*
lastptr() returns a pointer to the last non-null character in the string, if
any.  If the string is null it returns NULL
*/

static xmlChar *
lastptr(
    xmlChar *str
)
{
    xmlChar *p;
    if (str == NULL)
        SWISH_CROAK("received null pointer while looking for last NULL");
    if (*str == '\0')
        return (NULL);
    p = str;
    while (*p != '\0')          /* find trailing null char */
        ++p;
    --p;                        /* point to just before it */
    return (p);
}
