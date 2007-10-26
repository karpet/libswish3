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

#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

extern int errno;

#include "libswish3.h"

extern int SWISH_DEBUG;

/* PUBLIC */
swish_DocInfo *
swish_init_docinfo()
{

    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("init'ing docinfo");
    
    swish_DocInfo *docinfo = swish_xmalloc( sizeof(swish_DocInfo) );
    docinfo->nwords         = 0;
    docinfo->mtime          = 0;
    docinfo->size           = 0;
    docinfo->encoding       = swish_xstrdup( (xmlChar*)SWISH_DEFAULT_ENCODING );
    docinfo->uri            = NULL;
    docinfo->mime           = NULL;
    docinfo->parser         = NULL;
    docinfo->ext            = NULL;
    docinfo->update         = NULL;
   
    if (SWISH_DEBUG > 9)
    {
        SWISH_DEBUG_MSG("docinfo all ready");
        swish_debug_docinfo( docinfo );
    }

    return docinfo;
}

/* PUBLIC */
void
swish_free_docinfo( swish_DocInfo * ptr )
{    
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing swish_DocInfo");

    if (SWISH_DEBUG > 9)
        swish_debug_docinfo( ptr );


    ptr->nwords = 0; /* why is this required? */
    ptr->mtime  = 0;
    ptr->size   = 0;
    
    /* encoding and mime are malloced via xmlstrdup elsewhere */
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing docinfo->encoding");
    swish_xfree(ptr->encoding);
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing docinfo->mime");
    swish_xfree(ptr->mime);
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing docinfo->uri");
    swish_xfree(ptr->uri);
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing docinfo->ext");
    swish_xfree(ptr->ext);
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing docinfo->parser");
    swish_xfree(ptr->parser);
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("freeing docinfo ptr");
    swish_xfree(ptr);
    
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("docinfo ptr is all freed");
}

int
swish_check_docinfo(swish_DocInfo * docinfo, swish_Config * config)
{
    int     ok;
    xmlChar *ext;
    
    ok = 1;

    if (SWISH_DEBUG > 3)
        swish_debug_docinfo(docinfo);

    if (!docinfo->uri)
        SWISH_CROAK("Failed to return required header Content-Location:");

    if (docinfo->size == -1)
        SWISH_CROAK("Failed to return required header Content-Length: for doc '%s'",
                         docinfo->uri);

/* might make this conditional on verbose level */
    if (docinfo->size == 0)    
        SWISH_CROAK("Found zero Content-Length for doc '%s'", docinfo->uri);

    ext = swish_get_file_ext(docinfo->uri);
    /* this fails with non-filenames like db ids, etc. */

    if (docinfo->ext == NULL)
    {
        if (ext != NULL)
            docinfo->ext = swish_xstrdup( ext );
        else
            docinfo->ext = swish_xstrdup((xmlChar*)"none");
            
    }

    swish_xfree(ext);

    if (!docinfo->mime) {
        if (SWISH_DEBUG > 5)
            SWISH_DEBUG_MSG( "no MIME known. guessing based on uri extension '%s'", docinfo->ext);
        docinfo->mime = swish_get_mime_type( config, docinfo->ext );
    }
    else
    {
        if ( SWISH_DEBUG > 9 )
            SWISH_DEBUG_MSG( "found MIME type in headers: '%s'", docinfo->mime);
            
    }
    
    if (!docinfo->parser) {
        if (SWISH_DEBUG > 5)
            SWISH_DEBUG_MSG( "no parser defined in headers -- deducing from content type '%s'", docinfo->mime);
            
        docinfo->parser = swish_get_parser( config, docinfo->mime );
    }
    else
    {
        if (SWISH_DEBUG > 5)
            SWISH_DEBUG_MSG( "found parser in headers: '%s'", docinfo->parser);
            
    }
    
    if (SWISH_DEBUG > 9)
        swish_debug_docinfo(docinfo);

    return ok;

}

/* PUBLIC */
int
swish_docinfo_from_filesystem( xmlChar *filename, swish_DocInfo * i, swish_ParseData *parse_data )
{
    struct  stat info;
    int     stat_res;
    
    if (i->ext != NULL)
        swish_xfree( i->ext );
        
    i->ext = swish_get_file_ext( filename );
            
    stat_res = stat((char *)filename, &info);
        
    if ( stat_res == -1)
    {
        SWISH_WARN("Can't stat '%s': %s", filename, strerror(errno));
        return 0;
    }
                       
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("handling url %s", filename);
        
    if(i->uri != NULL)
        swish_xfree(i->uri);
            
    i->uri = swish_xstrdup( filename );       
    i->mtime = info.st_mtime;
    i->size  = info.st_size;
    
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("handling mime");
        
    if(i->mime != NULL)
        swish_xfree(i->mime);
        
    i->mime = swish_get_mime_type( parse_data->config, i->ext );
        
    if (SWISH_DEBUG > 9)
        SWISH_DEBUG_MSG("handling parser");
        
    if(i->parser != NULL)
        swish_xfree(i->parser);
        
    i->parser = swish_get_parser( parse_data->config, i->mime );
    
    return 1;
     
}

/* PUBLIC */
void
swish_debug_docinfo( swish_DocInfo * docinfo )
{
    xmlChar *h_mtime = swish_xmalloc( 30 );    
    strftime((char*)h_mtime, 
            (unsigned long)30, 
            SWISH_DATE_FORMAT_STRING, 
            (struct tm *) localtime((time_t *)&(docinfo->mtime) ));

    SWISH_DEBUG_MSG("DocInfo");
    SWISH_DEBUG_MSG("  docinfo ptr: %lu",                  (unsigned long)docinfo);
    //SWISH_DEBUG_MSG("  size of swish_DocInfo struct: %d", (int)sizeof(swish_DocInfo));
    //SWISH_DEBUG_MSG("  size of docinfo ptr: %d",           (int)sizeof(*docinfo));
    SWISH_DEBUG_MSG("  uri: %s (%d)", docinfo->uri,        (int)sizeof(docinfo->uri));
    SWISH_DEBUG_MSG("  doc size: %lu bytes (%d)",          (unsigned long)docinfo->size, (int)sizeof(docinfo->size));
    SWISH_DEBUG_MSG("  doc mtime: %lu (%d)",               (unsigned long)docinfo->mtime, (int)sizeof(docinfo->mtime));
    //SWISH_DEBUG_MSG("  size of mime: %d",                  (int)sizeof(docinfo->mime));
    //SWISH_DEBUG_MSG("  size of encoding: %d",              (int)sizeof(docinfo->encoding));
    SWISH_DEBUG_MSG("  mtime str: %s",                     h_mtime);
    SWISH_DEBUG_MSG("  mime type: %s",                     docinfo->mime);
    SWISH_DEBUG_MSG("  encoding: %s",                      docinfo->encoding); /* only known after parsing has started ... */
    SWISH_DEBUG_MSG("  file ext: %s",                      docinfo->ext);
    SWISH_DEBUG_MSG("  parser: %s",                        docinfo->parser);
    SWISH_DEBUG_MSG("  nwords: %d",                        docinfo->nwords);
    
    swish_xfree( h_mtime );    

}

