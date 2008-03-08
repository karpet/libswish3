/*
 * This file is part of libswish3
 * Copyright (C) 2008 Peter Karman
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

/* check a swish3 header file for correct syntax */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"


int 
main(int argc, char **argv)
{
#ifdef LIBXML_READER_ENABLED
    int i;
    swish_Config *config;
    
    swish_init();    

    for (i=1; i < argc; i++) {
        printf("config file %s\n", argv[i]);
        config = swish_read_header( (char*)argv[i] );
        swish_debug_config(config);
        swish_free_config(config);
    }
        
    
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return 0;
#else

    SWISH_CROAK("Your version of libxml2 is too old");
    return 1;
#endif

}
