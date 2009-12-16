/*
 * This file is part of libswish3
 * Copyright (C) 2009 Peter Karman
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

/* test utf8 functions */

#include <stdio.h>
#include <assert.h>
#include <libxml/hash.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <err.h>
#include <getopt.h>
#include "libswish3.h"

static struct option longopts[] = {
    {"file", required_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"loop", required_argument, 0, 'l'},
    {0, 0, 0, 0}
};

int main(
    int argc,
    char **argv
);
int usage(
);
void iterate(
    xmlChar *utf8
);
int char_report(
    xmlChar *ptr
);
void seq_by_seq(
    xmlChar *ptr
);
char *types[] = {
    "alnum", "cntrl", "ideogram", "print", "special",
    "alpha", "digit", "lower", "punct", "upper",
    "blank", "graph", "phonogram", "space", "xdigit"
};

int ntypes = 15;

int
usage(
)
{

    char *descr = "swish_utf8 is an example program for testing the libswish3 utf8 functions\n";
    printf("swish_utf8 [opts] [string(s)]\n");
    printf("opts:\n --file file.txt\n --debug\n");
    printf("opts:\n --loop N (perform tests N times for profiling)\n");
    printf("\n%s\n\n", descr);
    exit(1);
}

int
main(
    int argc,
    char **argv
)
{
    int i, ch, loop;
    int option_index = 0;
    extern char *optarg;
    extern int optind;
    xmlChar *string;
    string = NULL;
    loop = 1;

    swish_setup();   // always call first

    while ((ch = getopt_long(argc, argv, "d:f:h", longopts, &option_index)) != -1) {

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

        case 'l':
            loop = swish_string_to_int(optarg);

        case '?':
        case 'h':
        default:
            usage();

        }

    }

    i = optind;

    for (; i < argc; i++) {
        printf("utf8: %s\n", argv[i]);
        iterate((xmlChar *)argv[i]);
    }

    if (string != NULL) {
        printf("parsing UTF-8 string\n");
        while (loop--)
            iterate(string);
        swish_xfree(string);
    }

    return (0);
}

void
iterate(
    xmlChar *utf8
)
{
    int n_bytes;
    xmlChar *ptr, *escaped, *unescaped;

    ptr = utf8;
    n_bytes = xmlStrlen(utf8);
    escaped = swish_str_escape_utf8(utf8);
    unescaped = swish_str_unescape_utf8(escaped);
    
    printf("%s\n", utf8);
    printf("escaped:   '%s'\n", escaped);
    printf("unescaped: '%s'\n", unescaped);
    
    swish_xfree(escaped);
    swish_xfree(unescaped);
     
    //printf("iterate over %d characters %d bytes\n", n_chars, n_bytes);

    if (utf8 == NULL) {
        printf("first byte in utf8 string is NULL\n");
        return;
    }

    seq_by_seq(utf8);

    printf("----------------------------------------------------------\n");

    /*
       get first seq, then loop until done 
     */
    ptr += char_report(ptr);

    while (xmlStrlen(ptr)) {
        ptr += char_report(ptr);
    }

}

int
char_report(
    xmlChar *ptr
)
{
    xmlChar buf[5];             /* max length of ucs32 char plus NULL */
    int sl, i, j;
    uint32_t cp;
    xmlChar *escaped, *unescaped;
    
    cp = swish_utf8_codepoint(ptr);
    sl = swish_utf8_chr_len(ptr);
    printf("clen = %d ", sl);
    for (i = 0; i < sl; i++) {
        buf[i] = ptr[i];
        printf("0x%02x %d ", buf[i], buf[i]);
    }
    buf[i] = '\0';              /* terminate */
    printf(" -> %s ", buf);

    // get codepoint val
    printf("[0x%x] [%d]\n", cp, cp);
    escaped = swish_str_escape_utf8(buf);
    unescaped = swish_str_unescape_utf8(escaped);
    printf("escaped:   '%s'\n", escaped);
    printf("unescaped: '%s'\n", unescaped);
    swish_xfree(escaped);
    swish_xfree(unescaped);
    
    printf("   %lc\n", cp);

    for (j = 0; j < ntypes; j++) {
        printf(" %10s => %d\n", types[j], iswctype(cp, wctype(types[j])));
    }

    printf("\n");
    return sl;
}

void
seq_by_seq(
    xmlChar *ptr
)
{
    xmlChar buf[5];             /* max length of ucs32 char plus NULL */
    int byte_pos = 0;
    int prev_pos = 0;
    int clen, i;

    /*
       forward 
     */
    for (byte_pos = 0; ptr[prev_pos] != '\0'; swish_utf8_next_chr(ptr, &byte_pos)) {
        clen = byte_pos - prev_pos;
        if (!clen) {
            prev_pos = byte_pos;
            continue;
        }

        printf("clen = %d ", clen);
        for (i = 0; i < clen; i++) {
            buf[i] = ptr[prev_pos + i];
            printf("0x%02x ", buf[i]);
        }
        buf[i] = '\0';
        printf(" -> %s ", buf);

        // get codepoint val
        printf("[%d]", swish_utf8_codepoint(buf));
        printf("\n");
        prev_pos = byte_pos;
    }

    return;

    // the rest is optional

    /*
       reverse 
     */
    byte_pos -= 2;              /* back past NULL */
    for (; byte_pos >= 0; swish_utf8_prev_chr(ptr, &byte_pos)) {
        clen = prev_pos - byte_pos;
        if (!clen) {
            prev_pos = byte_pos;
            continue;
        }
        printf("clen = %d ", clen);
        for (i = 0; i < clen; i++) {
            buf[i] = ptr[byte_pos + i];
            printf("0x%02x ", buf[i]);
        }
        buf[i] = '\0';
        printf(" -> %s ", buf);

        // get codepoint val
        printf("[%d]", swish_utf8_codepoint(buf));
        printf("\n");
        prev_pos = byte_pos;
    }

}
