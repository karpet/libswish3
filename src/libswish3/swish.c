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

#include <stdlib.h>
#include "libswish3.h"


/* global stuff */

int SWISH_DEBUG = 0;

void static swish_init();

swish_3*
swish_init_swish3( void (*handler) (swish_ParseData *), void *stash )
{
    swish_3 *s3;
    swish_init();
    s3              = swish_xmalloc(sizeof(swish_3));
    s3->ref_cnt++;
    s3->config      = swish_init_config();
    s3->analyzer    = swish_init_analyzer(s3->config);
    s3->parser      = swish_init_parser(handler);
    s3->stash       = stash;
    return s3;
}

void
swish_free_swish3(swish_3* s3)
{
    swish_free_parser(s3->parser);
    swish_free_analyzer(s3->analyzer);
    swish_free_config(s3->config);
    swish_xfree(s3);
    swish_mem_debug();
}


void static
swish_init()
{
            
    /* global var that scripts can check to determine what version of Swish they are
     * using. the second 0 indicates that it will not override it if already set */
    setenv("SWISH3", "1", 0);
    
    /* global debug flag */
    setenv("SWISH_DEBUG", "0", 0);
    if (!SWISH_DEBUG)
        SWISH_DEBUG = (int)strtol(getenv("SWISH_DEBUG"), (char**)NULL, 10);

    swish_init_memory();
    swish_init_words();
    swish_verify_utf8_locale();

}
