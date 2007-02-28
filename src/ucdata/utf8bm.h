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
#ifndef _h_utf8bm
#define _h_utf8bm

/*
 * $Id: utf8bm.h,v 1.1 2005/02/10 23:52:22 mleisher Exp $
 */

#ifdef __cplusplus
extern "C" {
#endif

#undef __
#ifdef __STDC__
#define __(x) x
#else
#define __(x) ()
#endif

/**************************************************************************
 *
 * Macros used for compiling the search pattern.
 *
 **************************************************************************/

#define UTF8BM_CASE_INSENSITIVE 0x01

/**************************************************************************
 *
 * Structures for the search pattern.
 *
 **************************************************************************/

typedef struct {
    unsigned short key;
    unsigned short skip;
    unsigned short sibs;
    unsigned short kids;
} utf8bm_node_t;

typedef struct {
    /*
     * Lookup trie for the skip table. Used for case insensitive searching.
     */
    utf8bm_node_t *skip;
    unsigned short skip_size;
    unsigned short skip_used;

    /*
     * Skip table for case sensitive and all-ASCII matching.
     */
    unsigned short skip1[256];

    /*
     * A copy of the pattern.
     */
    unsigned char *pat;
    unsigned short pat_used;
    unsigned short pat_size;

    /*
     * An array of pointers to the individual characters in the pattern.  Used
     * to make comparisons much simpler.
     */
    unsigned char **patref;
    unsigned short patref_used;
    unsigned short patref_size;

    unsigned short md4;
    unsigned short flags;
    unsigned short chars;
} utf8bm_t;

/**************************************************************************
 *
 * API.
 *
 **************************************************************************/

extern utf8bm_t *utf8bm_compile __((unsigned char *pat, unsigned short patlen,
                                    unsigned short flags, utf8bm_t *term));

extern int utf8bm_search __((utf8bm_t *term, unsigned char *text,
                             unsigned long textlen, unsigned long offset,
                             unsigned long *match_start,
                             unsigned long *match_end));

extern void utf8bm_free __((utf8bm_t *term));

#undef __

#ifdef __cplusplus
}
#endif

#endif /* _h_utf8bm */
