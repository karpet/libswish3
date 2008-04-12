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
#include <getopt.h>

#include "libswish3.h"

int debug = 0;

int main(
    int argc,
    char **argv
);
int usage(
);
void handler(
    swish_ParserData *parser_data
);
void libxml2_version(
);
void swish_version(
);

int twords = 0;

extern int SWISH_DEBUG;

static struct option longopts[] = {
    {"config", required_argument, 0, 'c'},
    {"debug", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

void
libxml2_version(
)
{
    printf("libxml2 version: %s\n", LIBXML_DOTTED_VERSION);
}

void
swish_version(
)
{
    printf("libswish3 version %s\n", SWISH_LIB_VERSION);
    printf("swish version %s\n", SWISH_VERSION);
}

int
usage(
)
{

    char *descr = "swish_lint is an example program for using libswish3\n";
    printf("swish_lint [opts] [- | file(s)]\n");
    printf("opts:\n --config conf_file.xml\n --debug [lvl]\n --help\n");
    printf("\n%s\n", descr);
    libxml2_version();
    swish_version();
    exit(0);
}

void
handler(
    swish_ParserData *parser_data
)
{

    /*
       return; 
     */

    printf("nwords: %d\n", parser_data->docinfo->nwords);

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        swish_mem_debug();

    twords += parser_data->docinfo->nwords;

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO)
        swish_debug_docinfo(parser_data->docinfo);

    if (SWISH_DEBUG & SWISH_DEBUG_WORDLIST)
        swish_debug_wordlist(parser_data->wordlist);

    if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
        swish_debug_nb(parser_data->properties, (xmlChar *)"Property");
        swish_debug_nb(parser_data->metanames, (xmlChar *)"MetaName");
    }
}

int
main(
    int argc,
    char **argv
)
{
    int i, ch;
    extern char *optarg;
    extern int optind;
    int option_index;
    int files;
    int overwrite;
    char *etime;
    double start_time;
    xmlChar *config_file = NULL;
    swish_3 *s3;

    option_index = 0;
    files = 0;
    overwrite = 0;
    start_time = swish_time_elapsed();
    s3 = swish_init_swish3(&handler, NULL);

    while ((ch = getopt_long(argc, argv, "c:d:f:h", longopts, &option_index)) != -1) {

        switch (ch) {
        case 0:                /* If this option set a flag, do nothing else now. */
            if (longopts[option_index].flag != 0)
                break;
            printf("option %s", longopts[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'c':              /* should we set up default config first ? then override
                                 * here ? */

            //printf("optarg = %s\n", optarg);
            config_file = swish_xstrdup((xmlChar *)optarg);
            break;

        case 'd':
            printf("turning on debug mode: %s\n", optarg);

            if (!isdigit(optarg[0]))
                err(1, "-d option requires a positive integer as argument\n");

            SWISH_DEBUG = (int)strtol(optarg, (char **)NULL, 10);
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

    if (config_file != NULL) {
        s3->config = swish_add_config(config_file, s3->config);
    }

    i = optind;

    /*
       die with no args 
     */
    if (!i || i >= argc) {
        swish_free_swish3(s3);
        usage();

    }

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        swish_debug_config(s3->config);
    }

    for (; i < argc; i++) {

        if (argv[i][0] != '-') {

            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            printf("parse_file for %s\n", argv[i]);
            if (!swish_parse_file(s3, (unsigned char *)argv[i]))
                files++;

        }
        else if (argv[i][0] == '-' && !argv[i][1]) {

            printf("reading from stdin\n");
            files = swish_parse_fh(s3, NULL);

        }

    }

    printf("\n\n%d files indexed\n", files);
    printf("total words: %d\n", twords);

    etime = swish_print_time(swish_time_elapsed() - start_time);
    printf("%s total time\n\n", etime);
    swish_xfree(etime);

    swish_free_swish3(s3);

    if (config_file != NULL)
        swish_xfree(config_file);

    return (0);
}
