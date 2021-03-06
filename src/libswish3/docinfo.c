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

/* docinfo.c -- stat and time of files */

#ifndef LIBSWISH3_SINGLE_FILE
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "libswish3.h"
#endif

extern int errno;
extern int SWISH_DEBUG;

/* PUBLIC */
swish_DocInfo *
swish_docinfo_init(
)
{

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("init'ing docinfo");

    swish_DocInfo *docinfo = swish_xmalloc(sizeof(swish_DocInfo));
    docinfo->ref_cnt = 0;
    docinfo->nwords = 0;
    docinfo->mtime = 0;
    docinfo->size = 0;
    docinfo->encoding = swish_xstrdup((xmlChar *)SWISH_DEFAULT_ENCODING);
    docinfo->uri = NULL;
    docinfo->mime = NULL;
    docinfo->parser = NULL;
    docinfo->ext = NULL;
    docinfo->action = NULL;
    docinfo->is_gzipped = SWISH_FALSE;

    /*
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("docinfo all ready");
        swish_docinfo_debug(docinfo);
    }
    */

    return docinfo;
}

/* PUBLIC */
void
swish_docinfo_free(
    swish_DocInfo *ptr
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("freeing swish_DocInfo");
    }
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        swish_docinfo_debug(ptr);
    }
    if (ptr->ref_cnt != 0) {
        SWISH_WARN("docinfo ref_cnt != 0: %d", ptr->ref_cnt);
    }

    ptr->nwords = 0;            /* why is this required? */
    ptr->mtime = 0;
    ptr->size = 0;
    ptr->is_gzipped = SWISH_FALSE;

/* encoding and mime are malloced via xmlstrdup elsewhere */
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing docinfo->encoding");
    swish_xfree(ptr->encoding);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing docinfo->mime");
    if (ptr->mime != NULL)
        swish_xfree(ptr->mime);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing docinfo->uri");
    if (ptr->uri != NULL)
        swish_xfree(ptr->uri);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing docinfo->ext");
    if (ptr->ext != NULL)
        swish_xfree(ptr->ext);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing docinfo->parser");
    if (ptr->parser != NULL)
        swish_xfree(ptr->parser);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("freeing docinfo ptr");
    swish_xfree(ptr);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        SWISH_DEBUG_MSG("swish_DocInfo all freed");
}

int
swish_docinfo_check(
    swish_DocInfo *docinfo,
    swish_Config *config
)
{
    int ok;
    xmlChar *ext;

    ok = 1;

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO)
        swish_docinfo_debug(docinfo);

    if (!docinfo->uri)
        SWISH_CROAK("Failed to return required header Content-Location:");

    if (docinfo->size == -1)
        SWISH_CROAK
            ("Failed to return required header Content-Length: for doc '%s'",
             docinfo->uri);

/* might make this conditional on verbose level */
    if (docinfo->size == 0)
        SWISH_CROAK("Found zero Content-Length for doc '%s'", docinfo->uri);

    ext = swish_fs_get_file_ext(docinfo->uri);
    
/* this fails with non-filenames like db ids, etc. */

    if (docinfo->ext == NULL) {
        if (ext != NULL) {
            docinfo->ext = swish_xstrdup(ext);
        }
        else {
            docinfo->ext = swish_xstrdup((xmlChar *)"none");
        }
    }

    if (ext != NULL) {
        swish_xfree(ext);
    }
    
    if (!docinfo->mime) {
        if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
            SWISH_DEBUG_MSG
                ("no MIME known. guessing based on uri extension '%s'",
                 docinfo->ext);
        }
        docinfo->mime = swish_mime_get_type(config, docinfo->ext);
    }
    else {
        if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
            SWISH_DEBUG_MSG("found MIME type in headers: '%s'", docinfo->mime);
        }
    }

    if (!docinfo->parser) {
        if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
            SWISH_DEBUG_MSG
                ("no parser defined in headers -- deducing from content type '%s'",
                 docinfo->mime);
        }
        docinfo->parser = swish_mime_get_parser(config, docinfo->mime);
    }
    else {
        if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
            SWISH_DEBUG_MSG("found parser in headers: '%s'", docinfo->parser);
        }
    }

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        swish_docinfo_debug(docinfo);
    }
    return ok;

}

/* PUBLIC */
int
swish_docinfo_from_filesystem(
    xmlChar *filename,
    swish_DocInfo *i,
    swish_ParserData *parser_data
)
{
    if (i->ext != NULL)
        swish_xfree(i->ext);

    i->ext = swish_fs_get_file_ext(filename);
    if (xmlStrEqual(i->ext, BAD_CAST "gz")) {
        i->is_gzipped = SWISH_TRUE;
        /* get new ext */
        xmlChar *copy = swish_xstrdup(filename);
        unsigned int len = xmlStrlen(filename);
        copy[len-3] = '\0';
        swish_xfree(i->ext);
        i->ext = swish_fs_get_file_ext(copy);
        swish_xfree(copy);
    }
    
    if (!swish_fs_file_exists(filename)) {
        SWISH_WARN("Can't stat '%s': %s", filename, strerror(errno));
        return 0;
    }

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        SWISH_DEBUG_MSG("handling url %s", filename);
    }
    if (i->uri != NULL) {
        swish_xfree(i->uri);
    }
    i->uri = swish_xstrdup(filename);
    i->mtime = swish_fs_get_file_mtime(filename);
    i->size = swish_fs_get_file_size(filename);

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        SWISH_DEBUG_MSG("handling mime");
    }
    if (i->mime != NULL) {
        swish_xfree(i->mime);
    }
    i->mime = swish_mime_get_type(parser_data->s3->config, i->ext);

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        SWISH_DEBUG_MSG("handling parser");
    }
    if (i->parser != NULL) {
        swish_xfree(i->parser);
    }
    
    i->parser = swish_mime_get_parser(parser_data->s3->config, i->mime);

    return 1;

}

/* PUBLIC */
void
swish_docinfo_debug(
    swish_DocInfo *docinfo
)
{
    char *ts;
    ts = swish_time_format(docinfo->mtime);
    
    SWISH_DEBUG_MSG("DocInfo");
    SWISH_DEBUG_MSG("  docinfo ptr: %lu", (unsigned long)docinfo);
/* SWISH_DEBUG_MSG("  size of swish_DocInfo struct: %d", (int)sizeof(swish_DocInfo)); */
/* SWISH_DEBUG_MSG("  size of docinfo ptr: %d",           (int)sizeof(*docinfo)); */
    SWISH_DEBUG_MSG("  uri: %s (%d)", docinfo->uri, (int)sizeof(docinfo->uri));
    SWISH_DEBUG_MSG("  doc size: %lu bytes (%d)", (unsigned long)docinfo->size,
                    (int)sizeof(docinfo->size));
    SWISH_DEBUG_MSG("  doc mtime: %lu (%d)", (unsigned long)docinfo->mtime,
                    (int)sizeof(docinfo->mtime));
/* SWISH_DEBUG_MSG("  size of mime: %d",                  (int)sizeof(docinfo->mime)); */
/* SWISH_DEBUG_MSG("  size of encoding: %d",              (int)sizeof(docinfo->encoding)); */
    SWISH_DEBUG_MSG("  mtime str: %s", ts);
    SWISH_DEBUG_MSG("  mime type: %s", docinfo->mime);
    SWISH_DEBUG_MSG("  encoding: %s", docinfo->encoding);       /* only known after parsing has
                                                                   started ... */
    SWISH_DEBUG_MSG("  file ext: %s", docinfo->ext);
    SWISH_DEBUG_MSG("  parser: %s", docinfo->parser);
    SWISH_DEBUG_MSG("  nwords: %d", docinfo->nwords);
    SWISH_DEBUG_MSG("  is_gzipped: %d", docinfo->is_gzipped);
    
    swish_xfree(ts);
}
