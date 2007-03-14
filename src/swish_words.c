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


/* test word parser */

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

static struct option longopts[] =
{
    {"file", required_argument, 0, 'f'},
    {"debug", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

void            print_list(swish_WordList * list);
int             main(int argc, char **argv);
int             usage();

int             debug = 0;


int
usage()
{

    char           *descr = "swish_words is an example program for testing the word parser\n";
    printf("swish_words [opts] [string(s)]\n");
    printf("opts:\n --file file.txt\n --debug\n");
    printf("\n%s\n\n", descr);
    exit(1);
}

int
main(int argc, char **argv)
{
    int             i, ch;
    int             option_index = 0;
    extern char    *optarg;
    extern int      optind;
    xmlChar        *string;
    xmlChar        *meta = (xmlChar *) "swishdefault";
    int             max = 255;
    int             min = 1;

    swish_WordList *list;

    while ((ch = getopt_long(argc, argv, "d:f:h", longopts, &option_index)) != -1)
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

        case 'f':
            printf("reading %s\n", optarg);
                    
            string = swish_slurp_file((xmlChar *) optarg);
            list = swish_tokenize(string, meta, meta, max, min, 0, 0);
            swish_debug_wordlist(list);
            swish_free_WordList(list);
            swish_xfree(string);
            
            break;

        case 'd':
            printf("turning on debug mode: %s\n", optarg);

            if (!isdigit(optarg[0]))
                err(1, "-d option requires a positive integer as argument\n");

            setenv("SWISH_DEBUG", optarg, 1);
            debug = (int) strtol(getenv("SWISH_DEBUG"), (char **) NULL, 10);
            /* printf("debug at level %d\n", debug); */

            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }

    i = optind;
        
    for (; i < argc; i++)
    {

        list = swish_tokenize((xmlChar *) argv[i], meta, meta, max, min, 0, 0);
        printf("parsed: %s\n", argv[i]);
        swish_debug_wordlist(list);
        swish_free_WordList(list);

    }

    return (0);
}
