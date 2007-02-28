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
static char rcsid[] __attribute__ ((unused)) = "$Id: utf8bm.c,v 1.4 2005/02/10 23:52:48 mleisher Exp $";
#else
static char rcsid[] = "$Id: utf8bm.c,v 1.4 2005/02/10 23:52:48 mleisher Exp $";
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utf8bm.h"
#include "ucdata.h"

/*
 * Used internally to identify patterns that are all ASCII.
 */
#define UTF8BM_ALL_ASCII 0x80

/*
 * NOT IMPLEMENTED YET.
 * Testing for spaces and combining marks can be expensive when the UTF-8
 * characters have to be converted to UTF-32 before testing with 'ucdata'.
 * Maybe implement a UTF-8 specific version of 'ucdata'?
 */
#define UTF8BM_IGNORE_COMBINING  0x02
#define UTF8BM_COMPRESS_SPACES   0x04

static unsigned short
trie_node(utf8bm_t *term, unsigned short key)
{
    if (term->skip_used == term->skip_size) {
        if (term->skip_size == 0)
          term->skip = (utf8bm_node_t *)
              malloc(sizeof(utf8bm_node_t) * 32);
        else
          term->skip = (utf8bm_node_t *)
              realloc((char *) term->skip,
                      sizeof(utf8bm_node_t) * (term->skip_size + 32));
        term->skip_size += 32;
    }

    term->skip[term->skip_used].key = key;
    term->skip[term->skip_used].skip = 
        term->skip[term->skip_used].sibs = 
        term->skip[term->skip_used].kids = 0;

    return term->skip_used++;
}

static void
trie_add(utf8bm_t *term, unsigned char *c, int clen, unsigned short skip)
{
    int i, n, t, l;

    if (term->skip_used == 0)
      (void) trie_node(term, 0);

    for (i = t = 0; i < clen; i++) {
        if (term->skip[t].kids == 0) {
            n = trie_node(term, c[i]);
            term->skip[t].kids = n;
            t = n;
        } else if (term->skip[term->skip[t].kids].key == c[i])
          t = term->skip[t].kids;
        else if (term->skip[term->skip[t].kids].key > c[i]) {
            n = trie_node(term, c[i]);
            term->skip[n].sibs = term->skip[t].kids;
            term->skip[t].kids = n;
            t = n;
        } else {
            t = term->skip[t].kids;
            for (l = t; term->skip[t].sibs && term->skip[t].key < c[i]; ) {
                l = t;
                t = term->skip[t].sibs;
            }
            if (term->skip[t].key < c[i]) {
                n = trie_node(term, c[i]);
                term->skip[t].sibs = n;
                t = n;
            } else if (term->skip[t].key > c[i]) {
                n = trie_node(term, c[i]);
                term->skip[n].sibs = t;
                term->skip[l].sibs = n;
                t = n;
            }
        }
    }

    /*
     * If the skip value passed is 0, and the skip value in the trie node
     * is not 0, then we need to set the skip tuning factor for the sentinel
     * character.
     */
    if (!skip && term->skip[t].skip) {
        term->md4 = term->skip[t].skip;
        term->skip[t].skip = 0;
    } else
      term->skip[t].skip = skip;
}

/*
 * A simple array for comparing ASCII characters in a case insensitive
 * way.
 */
static unsigned char ascii_casemap[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
    0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};

/*
 * Special values for determining lengths of UTF-8 encoded characters.  Basic
 * formula: (*s ^ 0xf0) >> 4 where *s is the first byte of a UTF-8 encoded
 * character produces an index into the clengths array pointed at by
 * 'clp'. This pointer starts at clengths[2] so that a calculation for the
 * index may be done once and produce a -1 and a -2 index for 5 and 6-byte
 * lengths respectively.
 */
static char clengths[] = {6, 5, 4, 3, 2, 2,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static char *clp = 0;

/*
 * Determine the skip value for the current byte or UTF-8 character.
 */
static unsigned short
utf8bm_skip(utf8bm_t *term, unsigned char *text)
{
    int i, n, t;

    /*
     * If the pattern is all ASCII or is case sensitive, then we can simply
     * get the skip value from the table.
     */
    if ((term->flags & UTF8BM_ALL_ASCII) ||
        !(term->flags & UTF8BM_CASE_INSENSITIVE))
      return term->skip1[*text];

    /*
     * Have to do a trie lookup to determine the skip value for this
     * character.
     */
    n = clp[((*text ^ 0xf0) >> 4) - ((*text / 0xf8) + (*text / 0xfc))];

    t = term->skip[0].kids;
    for (i = 0; i < n; i++) {
        for (; term->skip[t].sibs && term->skip[t].key != text[i];
             t = term->skip[t].sibs);
        if (term->skip[t].key != text[i])
          /*
           * This character was not found, so return the number of chars
           * to skip.
           */
          return term->chars;

        if (term->skip[t].kids)
          t = term->skip[t].kids;
    }
    return term->skip[t].skip;
}

static int
utf8bm_equal(utf8bm_t *term, unsigned char *text, unsigned long offset)
{
    int i, n;
    unsigned char **pp, *sp, *ep, *lower;

    if (clp == 0)
      clp = &clengths[2];

    if (!(term->flags & UTF8BM_CASE_INSENSITIVE))
      /*
       * Case sensitive compare. It doesn't matter if the term is all ASCII
       * or not, an exact match is required.
       */
      return !memcmp((text + offset + 1) - term->pat_used, term->pat,
                     term->pat_used);

    if (term->flags & UTF8BM_ALL_ASCII) {
        /*
         * All-ASCII case insensitive compare. Done this way to avoid
         * dependence on routines that may or may not exist on a particular
         * OS.
         */
        n = term->pat_used - 2;
        sp = ((text + offset) - term->pat_used) + 1;
        for (i = 0; i < n; i++) {
            if (term->pat[i] != ascii_casemap[sp[i]])
              return 0;
        }
        return 1;
    }

    /*
     * Case insensitive compare.
     */

    /*
     * Skip to the previous character in the text because we know that the
     * final character already matches.
     */
    for (ep = text + offset, sp = ep - 1;
         sp > text && (*sp >= 0x80 && *sp <= 0xbf); sp--);

    for (pp = term->patref + (term->patref_used - 2);
         pp >= term->patref; pp--) {
        n = ucgetutf8lower(sp, &lower);

        if (n != *(pp + 1) - *pp || memcmp(lower, *pp, n))
          /*
           * The length of the UTF-8 sequences do not match or the lower case
           * version from the text does not match the lower case version in
           * the pattern.
           */
          return 0;

        /*
         * Skip backward one character in the text.
         */
        sp--;
        while (sp > text && *sp >= 0x80 && *sp <= 0xbf)
          sp--;
    }

    return 1;
}

/************************************************************************
 *
 * Public section.
 *
 ************************************************************************/

void
utf8bm_free(utf8bm_t *term)
{
    if (term) {
        if (term->pat_size > 0)
          free(term->pat);
        if (term->skip_size > 0)
          free(term->skip);
        if (term->patref_size > 0)
          free(term->patref);
        free(term);
    }
}

/*
 * If 'term' is NULL, a return value is allocated, otherwise the structure
 * passed will be used to compile the term.
 */
utf8bm_t *
utf8bm_compile(unsigned char *pat, unsigned short patlen, unsigned short flags,
               utf8bm_t *term)
{
    int i, cno, len, plen, have_case;
    unsigned long lower, upper, title;
    unsigned char utf8[8], *lp, *lc;

    if (term == 0) {
        /*
         * Allocate a term if one is not provided.
         */
        term = (utf8bm_t *) malloc(sizeof(utf8bm_t));
        memset(term, 0, sizeof(utf8bm_t));
    }

    /*
     * Initialize the pointer used to quickly determine UTF8 sequence lengths.
     */
    if (clp == 0)
      clp = &clengths[2];

    term->flags = flags;
    term->skip_used = term->pat_used = term->patref_used = term->chars = 0;

    if (flags & UTF8BM_CASE_INSENSITIVE) {
        /*
         * Add this flag in case an all-ASCII search term is provided.
         */
        term->flags |= UTF8BM_ALL_ASCII;

        /*
         * Count the number of characters. Necessary to assign skip values
         * as we construct the skip trie.
         */
        for (have_case = 0, plen = patlen, lp = pat; plen > 0;
             lp += len, plen -= len, term->chars++) {
            len = clp[((*lp ^ 0xf0) >> 4) - ((*lp / 0xf8) + (*lp / 0xfc))];
            /*
             * This is to check whether the pattern contains any characters
             * that actually have case variants. If not, then we can skip the
             * extra stuff needed to do case insensitive searching. If at
             * least one character with case variants is found, then we have
             * to build the per-character skip lookup trie.
             */
            if (!have_case) {
                lower = uctoutf32(lp, len);
                if (uchascase(lower))
                  have_case = 1;
            }

            /*
             * This is here to capture the case of an all-ASCII pattern.
             */
            if ((term->flags & UTF8BM_ALL_ASCII) && *lp > 0x7f)
              term->flags &= ~UTF8BM_ALL_ASCII;
        }

        /*
         * If no characters were found that have case variants, then we can
         * turn off case insensitive searching. Speeds up the search a bit.
         */
        if (!have_case)
          term->flags &= ~UTF8BM_CASE_INSENSITIVE;
    }

    if (term->flags & UTF8BM_CASE_INSENSITIVE) {
        if (term->flags & UTF8BM_ALL_ASCII) {
            /*
             * Initialize the skip array.
             */
            for (len = 0; len < 256; len++)
              term->skip1[len] = term->chars;
        }

        for (plen = patlen, cno = term->chars, len = 0, lp = pat; plen > 0;
             lp += len, plen -= len, cno--) {

            /*
             * We need to get the lower, upper, and title encodings of the
             * character so it can be added to the skip trie. The three case
             * possibilities for a character cannot simply be merged because
             * of differing lengths or common byte values that appear in
             * different places in the UTF-8 forms. That messes up the skip
             * factor that the Boyer-Moore algorithm depends on.
             */

            /*
             * Get the UTF-32 forms.
             */
            len = ucgetutf8lower(lp, &lc);
            lower = uctoutf32(lc, len);
            upper = uctoupper(lower);
            title = uctotitle(lower);

            /*
             * Append the lower version of the character to the copy of
             * the pattern.
             */
            if (term->pat_used == term->pat_size) {
                if (term->pat_size == 0)
                  term->pat = (unsigned char *)
                      malloc(sizeof(unsigned char) * 24);
                else
                  term->pat = (unsigned char *)
                      realloc((char *) term->pat,
                              sizeof(unsigned char) * (term->pat_size + 24));
                term->pat_size += 24;
            }
            memcpy(term->pat + term->pat_used, lc, len);
            term->pat_used += len;

            if (term->flags & UTF8BM_ALL_ASCII) {
                if (cno - 1 == 0) {
                    /*
                     * Set the skip tuning factor.
                     */
                    term->md4 = term->skip1[lower];
                    term->skip1[lower] = term->skip1[upper] = 0;
                } else
                  term->skip1[lower] = term->skip1[upper] = cno - 1;
            } else {
                /*
                 * Make sure the space exists for the pattern reference array.
                 */
                if (term->patref_size < term->chars) {
                    if (term->patref_size == 0)
                      term->patref = (unsigned char **)
                          malloc(sizeof(unsigned char *) * term->chars);
                    else
                      term->patref = (unsigned char **)
                          realloc(term->patref, sizeof(unsigned char *) *
                                  term->chars);
                    term->patref_size = term->chars;
                }

                /*
                 * Add the pointer to the lower case character just added.
                 */
                term->patref[term->patref_used++] =
                    term->pat + (term->pat_used - len);

                /*
                 * Add the lower case form to the skip trie with the current
                 * skip value in terms of characters, not bytes.
                 */
                trie_add(term, lc, len, cno - 1);

                if (upper != lower) {
                    /*
                     * Add the upper case form.
                     */
                    len = uctoutf8(upper, utf8, 8);
                    trie_add(term, utf8, len, cno - 1);
                }
                if (title != lower) {
                    /*
                     * Add the title case form.
                     */
                    len = uctoutf8(title, utf8, 8);
                    trie_add(term, utf8, len, cno - 1);
                }
            }
        }
    } else {
        /*
         * Make a copy of the pattern string.
         */
        if (term->pat_size < patlen) {
            if (term->pat_size == 0)
              term->pat = (unsigned char *)
                  malloc(sizeof(unsigned char) * patlen);
            else
              term->pat = (unsigned char *)
                  realloc(term->pat, sizeof(unsigned char) * patlen);
            term->pat_size = patlen;
        }
        memcpy(term->pat, pat, patlen);
        term->pat_used = patlen;

        /*
         * Initialize the skip array.
         */
        for (i = 0; i < 256; i++)
          term->skip1[i] = patlen;

        for (i = 0; i < patlen - 1; i++)
          term->skip1[pat[i]] = (patlen - 1) - i;

        /*
         * Set the skip tuning value.
         */
        term->md4 = term->skip1[pat[patlen - 1]];
        term->skip1[pat[patlen - 1]] = 0;
    }

    return term;
}

int
utf8bm_search(utf8bm_t *term, unsigned char *text, unsigned long textlen,
              unsigned long offset,
              unsigned long *match_start, unsigned long *match_end)
{
    int i, n, t;

    for (n = 0; offset < textlen;) {
        while (offset < textlen && (n = utf8bm_skip(term, text+offset))) {
            if ((term->flags & UTF8BM_CASE_INSENSITIVE) &&
                !(term->flags & UTF8BM_ALL_ASCII)) {
                /*
                 * In this case, we skip characters, not bytes.
                 */
                for (i = 0; i < n; i++) {
                    t = *(text+offset);
                    offset +=
                        clp[((t ^ 0xf0) >> 4) - ((t / 0xf8) + (t / 0xfc))];
                }
            } else
              offset += n;
        }

        /*
         * The end of the text was reached without a match.
         */
        if (offset >= textlen && n != 0)
          return 0;

        /*
         * If the pattern length is 1, we have a match and don't need to
         * check any farther.
         */
        if (term->chars == 1) {
            *match_end = offset + 1;
            *match_start = *match_end - term->pat_used;
            return 1;
        }

        if (utf8bm_equal(term, text, offset)) {
            if ((term->flags & UTF8BM_CASE_INSENSITIVE) &&
                !(term->flags & UTF8BM_ALL_ASCII)) {
                t = *(text+offset);
                *match_end = offset +
                    clp[((t ^ 0xf0) >> 4) - ((t / 0xf8) + (t / 0xfc))];
            } else
              *match_end = offset + 1;
            *match_start = *match_end - term->pat_used;
            return 1;
        }

        if ((term->flags & UTF8BM_CASE_INSENSITIVE) &&
            !(term->flags & UTF8BM_ALL_ASCII)) {
            /*
             * In this case, we skip characters, not bytes.
             */
            for (i = 0; i < term->md4; i++) {
                t = *(text+offset);
                offset += clp[((t ^ 0xf0) >> 4) - ((t / 0xf8) + (t / 0xfc))];
            }
        } else
          offset += term->md4;
    }
    return 0;
}

#ifdef BMTEST


static unsigned char pat[] = {0xe1, 0x80, 0xa4, 0};
static unsigned char text[] = {'G', 0xe1, 0x80, 0xa4, 'o', 0};

#if 0
static unsigned char pat[] = {0xc7, 0xb3, 0};
static unsigned char text[] = {0xc7, 0xb1, 'o', 0xc7, 0xb2, 'g', 0};
#endif

int
main(int argc, char **argv)
{
    char *prog = argv[0];
    utf8bm_t *term;
    int flags = 0;
    unsigned long s, e;

    argc--;
    argv++;

    while (argc > 0) {
        if (argv[0][0] == '-') {
            switch (argv[0][1]) {
              case 'i':
                flags |= UTF8BM_CASE_INSENSITIVE;
                break;
              default:
                fprintf(stderr, "%s: unknown parameter '%s'.\n",
                        prog, argv[0]);
            }
        }
        argc--;
        argv++;
    }

    if (ucdata_load(".:data", UCDATA_ALL)) {
        fprintf(stderr, "%s: problem loading ucdata.\n", prog);
        return 1;
    }

    term = utf8bm_compile(pat, strlen(pat), flags, 0);

    s = e = 0;
    while (utf8bm_search(term, text, strlen(text), s, &s, &e)) {
        printf("MATCH %ld-%ld\n", s, e);
        s = e;
    }
    if (!s && !e)
      printf("NO MATCH\n");

    utf8bm_free(term);

    ucdata_unload(UCDATA_ALL);

    return 0;
}

#endif
