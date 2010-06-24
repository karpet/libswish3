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

#ifndef LIBSWISH3_SINGLE_FILE
#include <stdlib.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include "acconfig.h"
#include "libswish3.h"
#endif

extern int errno;
int SWISH_DEBUG = 0;            /* global var */
int SWISH_WARNINGS = 1;         /* global var */

const char *
swish_lib_version(
)
{
    return (const char*)SWISH_LIB_VERSION;
}

const char *
swish_libxml2_version(
)
{
    return (const char*)LIBXML_DOTTED_VERSION;
}

swish_3 *
swish_3_init(
    void (*handler) (swish_ParserData *),
    void *stash
)
{
    swish_3 *s3;
    s3 = swish_xmalloc(sizeof(swish_3));
    s3->ref_cnt = 0;
    s3->config = swish_config_init();
    s3->config->ref_cnt++;
    swish_config_set_default(s3->config);
    s3->analyzer = swish_analyzer_init(s3->config);
    s3->analyzer->ref_cnt++;
    s3->parser = swish_parser_init(handler);
    s3->parser->ref_cnt++;
    s3->stash = stash;
    
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("s3 ptr 0x%lx", s3);
    }
    
    return s3;
}

void
swish_3_free(
    swish_3 *s3
)
{    
    s3->parser->ref_cnt--;
    if (s3->parser->ref_cnt < 1) {
        swish_parser_free(s3->parser);
    }

    s3->analyzer->ref_cnt--;
    if (s3->analyzer->ref_cnt < 1) {
        swish_analyzer_free(s3->analyzer);
    }

    s3->config->ref_cnt--;
    if (s3->config->ref_cnt < 1) {
        swish_config_free(s3->config);
    }

    if (s3->ref_cnt != 0) {
        SWISH_WARN("s3 ref_cnt != 0: %d\n", s3->ref_cnt);
    }
    swish_xfree(s3);
}

void
swish_setenv(
    char * name,
    char * value,
    int override
)
{
    int ret;
    ret = setenv(name, value, override);
    if (ret != 0) {
        SWISH_CROAK("setenv failed with %d: %s", errno, strerror(errno));
    }
}

/* MUST call this before instantiating any swish_3 objects */
void
swish_setup(
)
{

/* global var that scripts can check to determine what version of Swish they are
 * using. the second 0 indicates that it will not override it if already set */
    swish_setenv("SWISH3", "1", 0);

/* global debug flag */
    swish_setenv("SWISH_DEBUG", "0", 0);
    swish_setenv("SWISH_DEBUG_MEMORY", "0", 0);
    swish_setenv("SWISH_DEBUG_CONFIG", "0", 0);
    swish_setenv("SWISH_DEBUG_DOCINFO", "0", 0);
    swish_setenv("SWISH_DEBUG_IO", "0", 0);
    swish_setenv("SWISH_DEBUG_TOKENLIST", "0", 0);
    swish_setenv("SWISH_DEBUG_TOKENIZER", "0", 0);
    swish_setenv("SWISH_DEBUG_PARSER", "0", 0);
    swish_setenv("SWISH_DEBUG_NAMEDBUFFER", "0", 0);
    swish_setenv("SWISH_WARNINGS", "1", 0);
    if (!SWISH_DEBUG) {

        SWISH_DEBUG += swish_string_to_int(getenv("SWISH_DEBUG"));

/* additional env vars just increase the global var value */

        if (swish_string_to_int(getenv("SWISH_DEBUG_MEMORY"))) {
            SWISH_DEBUG += SWISH_DEBUG_MEMORY;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_CONFIG"))) {
            SWISH_DEBUG += SWISH_DEBUG_CONFIG;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_DOCINFO"))) {
            SWISH_DEBUG += SWISH_DEBUG_DOCINFO;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_TOKENLIST"))) {
            SWISH_DEBUG += SWISH_DEBUG_TOKENLIST;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_TOKENIZER"))) {
            SWISH_DEBUG += SWISH_DEBUG_TOKENIZER;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_PARSER"))) {
            SWISH_DEBUG += SWISH_DEBUG_PARSER;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_NAMEDBUFFER"))) {
            SWISH_DEBUG += SWISH_DEBUG_NAMEDBUFFER;
        }
        if (swish_string_to_int(getenv("SWISH_DEBUG_IO"))) {
            SWISH_DEBUG += SWISH_DEBUG_IO;
        }
        
        /* special value to turn on all debugging */
        if (SWISH_DEBUG == -1) {
            SWISH_DEBUG += SWISH_DEBUG_MEMORY;
            SWISH_DEBUG += SWISH_DEBUG_CONFIG;
            SWISH_DEBUG += SWISH_DEBUG_DOCINFO;
            SWISH_DEBUG += SWISH_DEBUG_TOKENLIST;
            SWISH_DEBUG += SWISH_DEBUG_TOKENIZER;
            SWISH_DEBUG += SWISH_DEBUG_PARSER;
            SWISH_DEBUG += SWISH_DEBUG_NAMEDBUFFER;
            SWISH_DEBUG += SWISH_DEBUG_IO;        
        }

        if (SWISH_DEBUG) {
            SWISH_DEBUG_MSG("SWISH_DEBUG set to %d", SWISH_DEBUG);
        }
    }
    
    SWISH_WARNINGS = swish_string_to_int(getenv("SWISH_WARNINGS"));

/*
     * initialize the library and check potential API mismatches
     * between the version it was compiled for and the actual shared
     * library used.
*/
    LIBXML_TEST_VERSION 
    swish_mem_init();
    swish_verify_utf8_locale();

}
