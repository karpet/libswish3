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
    {0, 0, 0, 0}
};

int 
usage()
{

    char * descr = "swish_header reads and writes swish3 header/config files\n";
    printf("swish_header [opts] file\n");
    printf("opts:\n --debug [lvl]\n --help\n");
    printf("\n%s\n", descr);
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
    swish_Config   *config;
    
    option_index = 0;
    
    swish_init();    

    while ((ch = getopt_long(argc, argv, "d:h", longopts, &option_index)) != -1)
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

            SWISH_DEBUG = (int) strtol(optarg, (char **) NULL, 10);
            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }
    
    i = optind;

    for (; i < argc; i++) {
        printf("config file %s\n", argv[i]);
        config = swish_init_config();
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("init_config");
        }
        swish_config_set_default(config);
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("set default");
        }
        if (!swish_merge_config_with_header( (char*)argv[i], config )) {
            SWISH_CROAK("failed to merge header %s with defaulf config", argv[i]);
        }                
        swish_write_header("swish_header.xml", config);
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            SWISH_DEBUG_MSG("header written");
        }
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
