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

void
swish_init()
{

    /* global var that scripts can check to determine what version of Swish they are
     * using. the second 0 indicates that it will not override it if already set */
    setenv("SWISH3", "1", 0);
    
    /* global debug flag */
    setenv("SWISH_DEBUG", "0", 0);
    if (!SWISH_DEBUG)
        SWISH_DEBUG = (int)strtol(getenv("SWISH_DEBUG"), (char**)NULL, 10);

    swish_init_parser();
    swish_init_memory();
    swish_init_words();
    swish_verify_utf8_locale();

}

void
swish_cleanup()
{
    swish_free_parser();
    swish_mem_debug();
}

