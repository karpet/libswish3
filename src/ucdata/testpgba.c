/*
 * Copyright 2005 Computing Research Labs, New Mexico State University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COMPUTING RESEARCH LAB OR NEW MEXICO STATE UNIVERSITY BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef lint
#ifdef __GNUC__
static char rcsid[] __attribute__ ((unused)) = "$Id: testpgba.c,v 1.4 2005/03/24 22:41:05 mleisher Exp $";
#else
static char rcsid[] = "$Id: testpgba.c,v 1.4 2005/03/24 22:41:05 mleisher Exp $";
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ucdata.h"
#include "ucpgba.h"

#define ishex(cc) (('0' <= (cc) && (cc) <= '9') || \
                   ('A' <= (cc) && (cc) <= 'F') || \
                   ('a' <= (cc) && (cc) <= 'f'))

/*
 * This is a routine to read a line into a fixed size buffer. This is to
 * restrict any attempts to overflow buffers using external data.
 */
static int
getline(FILE *in, char *buf, int bufsize)
{
    int i, c, nc;

    if (buf == 0 || bufsize == 0)
      return -1;

    for (i = c = 0; c != EOF && c != '\n' && i < bufsize; i++) {
        if ((c = getc(in)) != EOF) {
            /*
             * Take care of Mac and Windows end-of-line characters.
             */
            if (c == '\r') {
                if ((nc = getc(in)) != '\n')
                  ungetc(nc, in);
                c = '\n';
            }
            if (c != '\n')
              buf[i] = c;
        }
    }
    if (i > 0 && i < bufsize)
      i--;
    buf[i] = 0;

    return c;
}

/*************************************************************************
 *
 * Character mapping section.
 *
 *************************************************************************/

typedef struct {
    char *name;
    unsigned long map[128];
} cmap_t;

static cmap_t *maps;
static int maps_count;

static int
loadmaps(char *filename)
{
    FILE *in;
    int c;
    unsigned long uc;
    cmap_t *mp;
    char *bp, buf[96];

    if ((in = fopen(filename, "r")) == 0)
      return 0;

    while (getline(in, buf, 128) != EOF) {
        if (buf[0] == '#' || buf[0] == 0)
          continue;

        if (memcmp(buf, "map ", 4) == 0) {
            /*
             * Start of a new map.
             */
            if (maps_count == 0)
              maps = (cmap_t *) malloc(sizeof(cmap_t));
            else
              maps = (cmap_t *)
                  realloc(maps, sizeof(cmap_t) * (maps_count + 1));
            mp = maps + maps_count++;

            /*
             * Initialize the new map.
             */
            mp->name = malloc(strlen(buf + 4) + 1);
            strcpy(mp->name, buf + 4);
            for (uc = 0; uc < 96; uc++)
              mp->map[uc] = uc + ' ';
            continue;
        }

        bp = buf;
        c = *bp++;
        if (c == '\\')
          /*
           * Catch the cases of \# and \\.
           */
          c = *bp++;
        while (*bp && (*bp == ' ' || *bp == '\t'))
          bp++;
        uc = 0;

        /*
         * Check the prefix on the hexadecimal value.
         */
        if ((*bp == 'U' || *bp == 'u') &&
            (*(bp+1) == '+' || *(bp+1) == '-'))
          /*
           * U+XXXX U-XXXX u+XXXX u-XXXX.
           */
          bp += 2;
        else if (*bp == '0' && (*(bp+1) == 'x' || *(bp+1) == 'X'))
          /*
           * 0xXXXX or 0XXXXX.
           */
          bp += 2;
        else if (*bp == '\\' && (*(bp+1) == 'u' || *(bp+1) == 'x')) {
            /*
             * Either \uXXXX or \xXXXX.
             */
            bp += 2;
            if (*bp == '{')
              /*
               * Perl style \x{XXXX}.
               */
              bp++;
        }

        /*
         * Collect the number until we get a non-hexadecimal character.
         */
        while (*bp && ishex(*bp)) {
            uc <<= 4;
            if ('0' <= *bp && *bp <= '9')
              uc += *bp - '0';
            else if ('A' <= *bp && *bp <= 'F')
              uc += 10 + (*bp - 'A');
            else if ('a' <= *bp && *bp <= 'f')
              uc += 10 + (*bp - 'a');
            bp++;
        }

        /*
         * Add the mapping.
         */
        mp->map[c - ' '] = uc;
    }
    fclose(in);

    return 1;
}

static unsigned long *
makeustr(char *str, unsigned long *outlen, cmap_t *map)
{
    char *sp;
    int j, len;
    unsigned long *out = 0;

    if (!str || (len = strlen(str)) == 0)
      return 0;

    out = (unsigned long *) malloc(sizeof(unsigned long) * (len + 1));

    for (sp = str, j = 0; *sp; sp++)
      /*
       * Subtract a space off the character to get the actual index in
       * the smaller map.
       */
      out[j++] = (map != 0) ? map->map[*sp - ' '] : *sp;

    out[j] = 0;
    *outlen = j;
    return out;
}

static void
disp(ucstring_t *s, char *os, int cursor, cmap_t *map)
{
    unsigned long i, j, c;
    ucrun_t *run;

    for (run = s->visual_first; run; run = run->visual_next) {
        for (i = run->start; i < run->end; i++) {
            if (cursor && run == s->cursor && i == run->cursor) {
                if (run->direction == UCPGBA_LTR)
                  putchar('>');
                else
                  putchar('<');
            }
            c = run->chars[i-run->start];
            if (map == 0) {
                if (c > 0x80)
                  printf("\\x{%lX}", c);
                else
                  putchar(c);
            } else if (c < 0x80)
              putchar(c);
            else {
                for (j = 0; j < 96 && c != 0; j++) {
                    if (c == map->map[j]) {
                        /*
                         * Add a space to the index to determine the
                         * original ASCII letter used.
                         */
                        putchar(j + ' ');
                        c = 0;
                    }
                }
            }
        }
        if (cursor && run == s->cursor && i == run->cursor) {
            if (run->direction == UCPGBA_LTR)
              putchar('>');
            else
              putchar('<');
        }
    }
    putchar('\n');
}

int
main(int argc, char *argv[])
{
    char *prog, **lines, buf[128];
    int lines_used = 0, lines_size = 0;

    ucstring_t *str;
    unsigned long *ustr;
    unsigned long len;
    int i, params = 1, docursor = 0, cdir = UCPGBA_CURSOR_VISUAL;
    FILE *in = NULL;
    cmap_t *mp = 0;

    if ((prog = strrchr(argv[0], '/')) == 0)
      prog = argv[0];

#if 0
    /*
     * All of the data files don't need to be loaded for bidi reordering.
     */
    ucdata_load(".:data", UCDATA_ALL);
#endif

    /*
     * We only need the ctype data (about 27K) to do bidi reordering.
     */
    ucdata_load(".:data", UCDATA_CTYPE);

    argc--;
    argv++;
    while (argc > 0) {
        if (params && argv[0][0] == '-') {
            switch (argv[0][1]) {
              case '-': params = 0; break;
              case 'c': docursor = 1; break;
              case 'l': cdir = UCPGBA_CURSOR_LOGICAL; break;
              case 'v': cdir = UCPGBA_CURSOR_VISUAL; break;
              case 'm':
                /*
                 * Load the maps. Assume the file is in the current directory.
                 */
                if (!loadmaps("testpgba.map"))
                  fprintf(stderr,
                          "%s: unable to load maps from 'testpgba.map'.\n",
                          prog);
                else {
                    /*
                     * Find a specific map.
                     */
                    argc--;
                    argv++;
                    for (mp = 0, i = 0; i < maps_count && mp == 0; i++) {
                        if (strcmp(maps[i].name, argv[0]) == 0)
                          mp = maps + i;
                    }
                }
                if (mp == 0)
                  fprintf(stderr, "%s: map '%s' was not found\n",
                          prog, argv[0]);
                break;
              case 'f':
                /*
                 * Load a file containing a list of lines to be tested
                 * with the Bidi algorithm.
                 */
                argc--;
                argv++;
                if ((in = fopen(argv[0], "r")) != NULL) {
                    while (getline(in, buf, 128) != EOF) {
                        if (buf[0] == '#' || buf[0] == 0)
                          continue;
                        if (lines_used == lines_size) {
                            if (lines_size == 0)
                              lines = (char **)
                                  malloc(sizeof(char *) * 16);
                            else
                              lines = (char **)
                                  realloc((char *) lines,
                                          sizeof(char *) * (lines_size + 16));
                            lines_size += 16;
                        }
                        lines[lines_used++] = strdup(buf);
                    }
                    fclose(in);
                }
                break;
              default: printf("unknown option %s\n", argv[0]);
            }
        } else {
            if (argv[0][0] != 0) {
                if (lines_used == lines_size) {
                    if (lines_size == 0)
                      lines = (char **)
                          malloc(sizeof(char *) * 16);
                    else
                      lines = (char **)
                          realloc((char *) lines,
                                  sizeof(char *) * (lines_size + 16));
                    lines_size += 16;
                }
                lines[lines_used++] = strdup(argv[0]);
            }
        }
        argc--;
        argv++;
    }

    if (mp == 0)
      fprintf(stderr, "%s: no map selected. All ASCII assumed.\n", prog);

    for (params = 0; params < lines_used; params++) {
        /*
         * 1. CREATE STRING.
         */
        ustr = makeustr(lines[params], &len, mp);

        /*
         * 2. REORDER STRING.
         */
        str = ucstring_create(ustr, 0, len, UCPGBA_LTR, cdir);

        /*
         * 3. DUMP STRING.
         */
        printf("RESULT: ");
        disp(str, argv[0], 0, mp);
        if (docursor) {
            if (cdir == UCPGBA_CURSOR_VISUAL) {
                printf("Testing visual cursor motion left.\n");
                disp(str, argv[0], 1, mp);
                while (ucstring_cursor_left(str, 1))
                  disp(str, argv[0], 1, mp);
                printf("Testing visual cursor motion right.\n");
                disp(str, argv[0], 1, mp);
                while (ucstring_cursor_right(str, 1))
                  disp(str, argv[0], 1, mp);
            } else {
                printf("Testing logical cursor motion right.\n");
                disp(str, argv[0], 1, mp);
                while (ucstring_cursor_right(str, 1))
                  disp(str, argv[0], 1, mp);
                printf("Testing logical cursor motion left.\n");
                disp(str, argv[0], 1, mp);
                while (ucstring_cursor_left(str, 1))
                  disp(str, argv[0], 1, mp);
            }
        }
        if (str != NULL)
          ucstring_free(str);
        free(ustr);

        /*
         * Free up each line as it is encountered.
         */
        free(lines[params]);
    }

    if (lines_size > 0)
      free(lines);

    ucdata_unload(UCDATA_ALL);
    return 0;
}
