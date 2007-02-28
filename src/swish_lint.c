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
 
/* swish_lint.c -- test libswish3 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>
#include <libxml/hash.h>
#include <getopt.h>

#include "libswish3.h"

int             debug = 0;

int             main(int argc, char **argv);
int             usage();
void            handler(swish_ParseData * parse_data);
void            libxml2_version();
void            swish_version();

int             twords = 0;

static struct option longopts[] =
{
    {"config", required_argument, 0, 'c'},
    {"debug",  required_argument, 0, 'd'},
    {"help",   no_argument, 0, 'h'},
    {"version", no_argument, 0, 'v'},
    {0, 0, 0, 0}
};


void libxml2_version()
{
    printf("libxml2 version: %s\n",  LIBXML_DOTTED_VERSION);   
}

void swish_version()
{
    printf("libswishp version %s\n", SWISH_VERSION);
}

int 
usage()
{

    char * descr = "swish_lint is an example program for using SwishParser\n";
    printf("swish_lint [opts] [- | file(s)]\n");
    printf("opts:\n --config conf_file.xml\n --debug\n --help\n --version\n");
    printf("\n%s\n", descr);
    libxml2_version();
    swish_version();
    exit(0);
}

void 
handler(swish_ParseData * parse_data)
{

    /* return; */

    printf("nwords: %d\n", parse_data->docinfo->nwords);
    
    twords += parse_data->docinfo->nwords;

    if (debug)
    {
        swish_debug_docinfo(parse_data->docinfo);
        swish_debug_wordlist(parse_data->wordlist);
        swish_debug_PropHash(parse_data->propHash);
    }
}

int 
main(int argc, char **argv)
{
    int             i, ch;
    extern char    *optarg;
    extern int      optind;
    int             option_index = 0;
    int             files = 0;
    int             overwrite = 0;
    char           *etime;
    double          startTime = swish_time_elapsed();
    
    swish_init_parser();

    swish_Config * config = swish_init_config();

    /* setting this \after\ make_char_tables() causes weird error...
     * 
     * xmlSubstituteEntitiesDefault(1); resolve text entities */


    while ((ch = getopt_long(argc, argv, "c:d:f:h", longopts, &option_index)) != -1)
    {
        /* printf("switch is %c\n",   ch); */
        /* printf("optarg is %s\n", optarg); */
        /* printf("optind = %d\n",  optind); */

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

        case 'c':    /* should we set up default config first ? then override
                 * here ? */

            //printf("optarg = %s\n", optarg);
            config = swish_add_config((xmlChar *) optarg, config);
            break;
            
        case 'v':
            libxml2_version();
            swish_version();
            break;


        case 'd':
            printf("turning on debug mode: %s\n", optarg);

            if (!isdigit(optarg[0]))
                err(1, "-d option requires a positive integer as argument\n");

            setenv("SWISH_DEBUG", optarg, 1);
            debug = (int) strtol(getenv("SWISH_DEBUG"), (char **) NULL, 10);
            /* printf("debug at level %d\n", debug); */

            break;


        case 'o':
            overwrite = 1;
            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }

    i = optind;
    
    /* die with no args */
    if (!i || i >= argc)
    {
        swish_free_config(config);
        usage();

    }
    
    if (debug == 20)
    {
        swish_debug_config(config);   
    }
        
    
    /* if (optind < argc) { printf("non-option ARGV-elements:\n"); while (optind <
     * argc) printf("%s ", argv [optind++]); putchar('\n'); } */

    for (; i < argc; i++)
    {

        if (argv[i][0] != '-')
        {

            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            printf("parse_file for %s\n", argv[i]);
            if (! swish_parse_file((unsigned char *) argv[i], config, &handler, NULL, NULL))
                files++;

        }
        else if (argv[i][0] == '-' && !argv[i][1])
        {

            printf("reading from stdin\n");
            files = swish_parse_stdin(config, &handler, NULL, NULL);

        }

    }

    printf("\n\n%d files indexed\n", files);
    printf("total words: %d\n", twords);

    etime = swish_print_time(swish_time_elapsed() - startTime);
    printf("%s total time\n\n", etime);
    swish_xfree(etime);
    
    swish_free_config( config );
    swish_free_parser();
    swish_mem_debug();

    return (0);
}
