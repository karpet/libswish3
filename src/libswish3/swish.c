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

int             SWISH_DEBUG = 0;/* global var */


swish_3        *
                swish_init_swish3(void (*handler) (swish_ParserData *), void *stash) {
    swish_3        *s3;
    swish_init();
    s3 = swish_xmalloc(sizeof(swish_3));
    s3->ref_cnt = 0;
    s3->config = swish_init_config();
    s3->config->ref_cnt++;
    swish_config_set_default(s3->config);
    s3->analyzer = swish_init_analyzer(s3->config);
    s3->analyzer->ref_cnt++;
    s3->parser = swish_init_parser(handler);
    s3->parser->ref_cnt++;
    s3->stash = stash;
    return s3;
}

void
swish_free_swish3(swish_3 *s3)
{
    s3->parser->ref_cnt--;
    if (s3->parser->ref_cnt < 1) {
/* SWISH_DEBUG_MSG("freeing parser"); */
        swish_free_parser(s3->parser);
    }

    s3->analyzer->ref_cnt--;
    if (s3->analyzer->ref_cnt < 1) {
/* SWISH_DEBUG_MSG("freeing analyzer"); */
        swish_free_analyzer(s3->analyzer);
    }

    s3->config->ref_cnt--;
    if (s3->config->ref_cnt < 1) {
/* SWISH_DEBUG_MSG("freeing config"); */
        swish_free_config(s3->config);
    }

/* SWISH_DEBUG_MSG("freeing s3"); */
    if (s3->ref_cnt != 0) {
        SWISH_WARN("s3 ref_cnt != 0: %d\n", s3->ref_cnt);
    }
    swish_xfree(s3);
    swish_mem_debug();
}


void
swish_init()
{

/* global var that scripts can check to determine what version of Swish they are
     * using. the second 0 indicates that it will not override it if already set */
    setenv("SWISH3", "1", 0);

/* global debug flag */
    setenv("SWISH_DEBUG", "0", 0);
    setenv("SWISH_DEBUG_MEMORY", "0", 0);
    setenv("SWISH_DEBUG_CONFIG", "0", 0);
    setenv("SWISH_DEBUG_DOCINFO", "0", 0);
    setenv("SWISH_DEBUG_WORDLIST", "0", 0);
    setenv("SWISH_DEBUG_TOKENIZER", "0", 0);
    setenv("SWISH_DEBUG_PARSER", "0", 0);
    setenv("SWISH_DEBUG_NAMEDBUFFER", "0", 0);
    if (!SWISH_DEBUG) {

        SWISH_DEBUG += (int) strtol(getenv("SWISH_DEBUG"), (char **) NULL, 10);

/* additional env vars just increase the global var value */

        if ((int) strtol(getenv("SWISH_DEBUG_MEMORY"), (char **) NULL, 10)) {
            SWISH_DEBUG += SWISH_DEBUG_MEMORY;
        }
        if ((int) strtol(getenv("SWISH_DEBUG_CONFIG"), (char **) NULL, 10)) {
            SWISH_DEBUG += SWISH_DEBUG_CONFIG;
        }
        if ((int) strtol(getenv("SWISH_DEBUG_DOCINFO"), (char **) NULL, 10)) {
            SWISH_DEBUG += SWISH_DEBUG_DOCINFO;
        }
        if ((int) strtol(getenv("SWISH_DEBUG_WORDLIST"), (char **) NULL, 10)) {
            SWISH_DEBUG += SWISH_DEBUG_WORDLIST;
        }
        if ((int) strtol(getenv("SWISH_DEBUG_PARSER"), (char **) NULL, 10)) {
            SWISH_DEBUG += SWISH_DEBUG_PARSER;
        }
        if ((int) strtol(getenv("SWISH_DEBUG_NAMEDBUFFER"), (char **) NULL, 10)) {
            SWISH_DEBUG += SWISH_DEBUG_NAMEDBUFFER;
        }
    }

/*
     * initialize the library and check potential API mismatches
     * between the version it was compiled for and the actual shared
     * library used.
*/
    LIBXML_TEST_VERSION

        swish_init_memory();
    swish_init_words();
    swish_verify_utf8_locale();

}
