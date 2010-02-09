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

#include <ctype.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include <err.h>
#include <getopt.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

static struct option longopts[] =
{
    {"debug",           required_argument,  0, 'd'},
    {"help",            no_argument,        0, 'h'},
    {"test",            no_argument,        0, 't'},
    {0, 0, 0, 0}
};

int 
usage()
{

    char * descr = "swish_header validates and merges libswish3 header/config files\n";
    printf("swish_header [opts] file [...fileN]\n");
    printf("opts:\n --debug [lvl]\n --test\n --help\n");
    printf("\n%s\n", descr);
    printf("libswish3 version: %s\n", swish_lib_version());
    printf("  libxml2 version: %s\n", swish_libxml2_version());
    printf("    swish version: %s\n", SWISH_VERSION);
    exit(0);
}


int 
main(int argc, char **argv)
{
#ifdef LIBXML_READER_ENABLED
    int i, ch;
    int             option_index;
    extern char    *optarg;
    extern int      optind;
    boolean         test_mode;
    swish_Config   *config, *test_config;
    
    option_index = 0;
    test_mode = SWISH_FALSE;
    
    swish_setup();    

    while ((ch = getopt_long(argc, argv, "d:ht", longopts, &option_index)) != -1)
    {
        switch (ch)
        {
        case 0:    /* If this option set a flag, do nothing else now. */
            if (longopts[option_index].flag != 0)
                break;
            printf("option %s", longopts[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;            

        case 'd':
            printf("turning on debug mode: %s\n", optarg);

            if (!isdigit(optarg[0]))
                err(1, "-d option requires a positive integer as argument\n");

            SWISH_DEBUG = swish_string_to_int(optarg);
            break;
            
        case 't':
            test_mode = SWISH_TRUE;
            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }
    
    i = optind;

    if (!i || i >= argc) {
        usage();
    }

    config = swish_config_init();
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("init_config");
    }
    swish_config_set_default(config);
    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        SWISH_DEBUG_MSG("set default");
    }

    for (; i < argc; i++) {
        if (test_mode) {
            printf("testing %s ... ", argv[i]);
            test_config = swish_header_read(argv[i]);
            swish_config_free(test_config);
            printf("ok\n");
            continue;
        }
        printf("merging %s ... ", argv[i]);
        if (!swish_header_merge( (char*)argv[i], config )) {
            SWISH_CROAK("failed to merge config %s", argv[i]);
        }
        else {
            printf("ok\n");
        }       
    }

    if (!test_mode) {
        swish_header_write("swish_header.xml", config);
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("header written");
        }
    }

    swish_config_free(config);
    swish_mem_debug();
    return 0;
#else

    SWISH_CROAK("Your version of libxml2 is too old");
    return 1;
#endif

}
