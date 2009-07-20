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

/* report on the isw* functions for any decimal character value.
 * 
 * example:
 * 
 * $ ./swish_isw 100
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>
#include <locale.h>

#include "libswish3.h"

void report(
    char *locale,
    int n
);
void usage(
);
int main(
    int argc,
    char **argv
);

char *types[] = {
    "alnum", "cntrl", "ideogram", "print", "special",
    "alpha", "digit", "lower", "punct", "upper",
    "blank", "graph", "phonogram", "space", "xdigit"
};

int ntypes = 15;

int
main(
    int argc,
    char **argv
)
{
    int i, n;
    char *curlocale, *locale;

    if (argc == 1)
        usage();

    locale = getenv("LC_CTYPE");
    printf("getenv locale = %s\n", locale);
    if (!locale) {
        locale = "C";
    }
    setlocale(LC_CTYPE, locale);
    curlocale = strdup(locale);

    for (i = 1; i < argc; i++) {

        if (!iswdigit(argv[i][0]))
            err(1, "arg %s is not a positive integer\n", argv[i]);

        n = swish_string_to_int(argv[i]);

        report(curlocale, n);
        report("en_US.UTF-8", n);

    }
    return 1;
}

void
usage(
)
{
    printf("usage: swish_isw N\n\n");
    printf("swish_isw is for testing locale and character property values.\n");
    printf("pass one or more decimal character values as arguments.\n");
    printf("Example: swish_isw 100\n");
    exit(0);
}

void
report(
    char *locale,
    int n
)
{
    int j;

    setlocale(LC_ALL, locale);
    printf("locale: %s\n", setlocale(LC_ALL, NULL));

    printf("%lc  %d  \\x%04x\n", n, n, n);

    for (j = 0; j < ntypes; j++) {
        printf("%10s => %d\n", types[j], iswctype(n, wctype(types[j])));
    }
}
