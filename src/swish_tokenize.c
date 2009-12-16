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

/* test utf8 tokenizer */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>
#include <getopt.h>

#include "libswish3.h"

static struct option longopts[] = {
    {"file", required_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(
    int argc,
    char **argv
);
int usage(
);

extern int SWISH_DEBUG;

int
usage(
)
{

    char *descr =
        "swish_tokenize is an example program for testing the libswish3 tokenizer\n";
    printf("swish_tokenize [opts] [string(s)]\n");
    printf("opts:\n --file file.txt\n");
    printf("\n%s\n\n", descr);
    exit(1);
}

int
main(
    int argc,
    char **argv
)
{
    int i, ch;
    int option_index;
    int ntokens;
    extern char *optarg;
    extern int optind;
    xmlChar *string;
    swish_TokenIterator *iterator;
    xmlChar *meta;
    swish_3 *s3;

    swish_setup();   // always call first
    meta = (xmlChar *)SWISH_DEFAULT_METANAME;
    option_index = 0;
    string = NULL;

    s3 = swish_3_init(NULL, NULL);
    iterator = swish_token_iterator_init(s3->analyzer);

    while ((ch = getopt_long(argc, argv, "f:h", longopts, &option_index)) != -1) {

        switch (ch) {
        case 0:                /* If this option set a flag, do nothing else now. */
            if (longopts[option_index].flag != 0)
                break;
            printf("option %s", longopts[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'f':
            printf("reading %s\n", optarg);
            string = swish_io_slurp_file( (xmlChar *)optarg, 0, swish_fs_looks_like_gz((xmlChar*)optarg), SWISH_FALSE );
            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }

    i = optind;

    for (; i < argc; i++) {
        ntokens =
            swish_tokenize(iterator, (xmlChar *)argv[i],
                            swish_hash_fetch(s3->config->metanames, meta), meta);
        printf("parsed %d tokens: %s\n", ntokens, argv[i]);
        swish_token_list_debug(iterator);
    }

    if (string != NULL) {
        ntokens =
            swish_tokenize(iterator, string,  
                            swish_hash_fetch(s3->config->metanames, meta), meta);
        printf("parsed %d tokens\n", ntokens);
        swish_token_list_debug(iterator);
        swish_xfree(string);
    }

    swish_token_iterator_free(iterator);
    swish_3_free(s3);

    return (0);
}
