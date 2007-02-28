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
static char rcsid[] __attribute__ ((unused)) = "$Id: ucgendat.c,v 1.20 2005/03/22 00:05:08 mleisher Exp $";
#else
static char rcsid[] = "$Id: ucgendat.c,v 1.20 2005/03/22 00:05:08 mleisher Exp $";
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

#define ishdigit(cc) (((cc) >= '0' && (cc) <= '9') ||\
                      ((cc) >= 'A' && (cc) <= 'F') ||\
                      ((cc) >= 'a' && (cc) <= 'f'))

/*
 * A header written to the output file with the byte-order-mark and the number
 * of property nodes.
 */
static unsigned short hdr[2] = {0xfeff, 0};

#define NUMPROPS 51
#define NEEDPROPS (NUMPROPS + (4 - (NUMPROPS & 3)))

typedef struct {
    char *name;
    int len;
} _prop_t;

/*
 * List of properties expected to be found in the Unicode Character Database
 * including some implementation specific properties.
 *
 * The implementation specific properties are:
 * Cm = Composed (can be decomposed)
 * Nb = Non-breaking
 * Sy = Symmetric (has left and right forms)
 * Hd = Hex digit
 * Qm = Quote marks
 * Mr = Mirroring
 * Ss = Space, other
 * Cp = Defined character
 */
static _prop_t props[NUMPROPS] = {
    {"Mn", 2}, {"Mc", 2}, {"Me", 2}, {"Nd", 2}, {"Nl", 2}, {"No", 2},
    {"Zs", 2}, {"Zl", 2}, {"Zp", 2}, {"Cc", 2}, {"Cf", 2}, {"Cs", 2},
    {"Co", 2}, {"Cn", 2}, {"Lu", 2}, {"Ll", 2}, {"Lt", 2}, {"Lm", 2},
    {"Lo", 2}, {"Pc", 2}, {"Pd", 2}, {"Ps", 2}, {"Pe", 2}, {"Po", 2},
    {"Sm", 2}, {"Sc", 2}, {"Sk", 2}, {"So", 2}, {"L",  1}, {"R",  1},
    {"EN", 2}, {"ES", 2}, {"ET", 2}, {"AN", 2}, {"CS", 2}, {"B",  1},
    {"S",  1}, {"WS", 2}, {"ON", 2},
    {"Cm", 2}, {"Nb", 2}, {"Sy", 2}, {"Hd", 2}, {"Qm", 2}, {"Mr", 2},
    {"Ss", 2}, {"Cp", 2}, {"Pi", 2}, {"Pf", 2}, {"AL", 2}, {"Bc", 2},
};

typedef struct {
    unsigned long *ranges;
    unsigned short used;
    unsigned short size;
} _ranges_t;

static _ranges_t proptbl[NUMPROPS];

/*
 * Make sure this array is sized to be on a 4-byte boundary at compile time.
 */
static unsigned short propcnt[NEEDPROPS];

/*
 * Array used to collect a decomposition before adding it to the decomposition
 * table.
 */
static unsigned long dectmp[64];
static unsigned long dectmp_size;

typedef struct {
    unsigned long code;
    unsigned short size;
    unsigned short used;
    unsigned long *decomp;
} _decomp_t;

/*
 * List of decomposition.  Created and expanded in order as the characters are
 * encountered.
 */
static _decomp_t *decomps;
static unsigned long decomps_used;
static unsigned long decomps_size;

/*
 * Composition exclusion table stuff.
 */
#define COMPEX_SET(c) (compexs[(c) >> 15] |= (1 << ((c) & 31)))
#define COMPEX_TEST(c) (compexs[(c) >> 15] & (1 << ((c) & 31)))
static unsigned long compexs[2048];

/*
 * Struct for holding a composition pair, and array of composition pairs
 */
typedef struct {
    unsigned long comp;
    unsigned long count;
    unsigned long code1;
    unsigned long code2;
} _comp_t;

static _comp_t *comps;
static unsigned long comps_size;
static unsigned long comps_used;

/*
 * Types and lists for handling lists of case mappings.
 */
typedef struct {
    unsigned long key;
    unsigned long other1;
    unsigned long other2;
} _case_t;

static _case_t *upper;
static _case_t *lower;
static _case_t *title;
static unsigned long upper_used;
static unsigned long upper_size;
static unsigned long lower_used;
static unsigned long lower_size;
static unsigned long title_used;
static unsigned long title_size;

/*
 * This section is for creating and generating a table for doing
 * reasonably fast UTF-8 case comparisons. UTF-8 lower case mappings are stored
 * in a lookup trie.
 */
typedef struct {
    unsigned short key;
    unsigned short offset;
    unsigned short sibs;
    unsigned short kids;
} _tnode_t;

static _tnode_t *trie;
static unsigned short trie_used;
static unsigned short trie_size;

static unsigned char *ltable;
static unsigned short ltable_used;
static unsigned short ltable_size;

static unsigned short *ltrie;
static unsigned short ltrie_used;
static unsigned short ltrie_size;

/*
 * Array used to collect case mappings before adding them to a list.
 */
static unsigned long cases[3];

/*
 * An array to hold ranges for combining classes.
 */
static unsigned long *ccl;
static unsigned long ccl_used;
static unsigned long ccl_size;

/*
 * Structures for handling numbers.
 */
typedef struct {
    unsigned long code;
    unsigned long idx;
} _codeidx_t;

typedef struct {
    short numerator;
    short denominator;
} _num_t;

/*
 * Arrays to hold the mapping of codes to numbers.
 */
static _codeidx_t *ncodes;
static unsigned long ncodes_used;
static unsigned long ncodes_size;

/*
 * Array for holding numbers.
 */
static _num_t *nums;
static unsigned long nums_used;
static unsigned long nums_size;

static void
#ifdef __STDC__
add_range(unsigned long start, unsigned long end, char *p1, char *p2)
#else
add_range(start, end, p1, p2)
unsigned long start, end;
char *p1, *p2;
#endif
{
    int i, j, k, len;
    _ranges_t *rlp;
    char *name;

    for (k = 0; k < 2; k++) {
        if (k == 0) {
            name = p1;
            len = 2;
        } else {
            if (p2 == 0)
              break;

            name = p2;
            len = 1;
        }

        for (i = 0; i < NUMPROPS; i++) {
            if (props[i].len == len && memcmp(props[i].name, name, len) == 0)
              break;
        }

        if (i == NUMPROPS)
          continue;

        rlp = &proptbl[i];

        /*
         * Resize the range list if necessary.
         */
        if (rlp->used == rlp->size) {
            if (rlp->size == 0)
              rlp->ranges = (unsigned long *)
                  malloc(sizeof(unsigned long) << 3);
            else
              rlp->ranges = (unsigned long *)
                  realloc((char *) rlp->ranges,
                          sizeof(unsigned long) * (rlp->size + 8));
            rlp->size += 8;
        }

        /*
         * If this is the first code for this property list, just add it
         * and return.
         */
        if (rlp->used == 0) {
            rlp->ranges[0] = start;
            rlp->ranges[1] = end;
            rlp->used += 2;
            continue;
        }

        /*
         * Optimize the case of adding the range to the end.
         */
        j = rlp->used - 1;
        if (start > rlp->ranges[j]) {
            j = rlp->used;
            rlp->ranges[j++] = start;
            rlp->ranges[j++] = end;
            rlp->used = j;
            continue;
        }

        /*
         * Need to locate the insertion point.
         */
        for (i = 0;
             i < rlp->used && start > rlp->ranges[i + 1] + 1; i += 2) ;

        /*
         * If the start value lies in the current range, then simply set the
         * new end point of the range to the end value passed as a parameter.
         */
        if (rlp->ranges[i] <= start && start <= rlp->ranges[i + 1] + 1) {
            rlp->ranges[i + 1] = end;
            return;
        }

        /*
         * Shift following values up by two.
         */
        for (j = rlp->used; j > i; j -= 2) {
            rlp->ranges[j] = rlp->ranges[j - 2];
            rlp->ranges[j + 1] = rlp->ranges[j - 1];
        }

        /*
         * Add the new range at the insertion point.
         */
        rlp->ranges[i] = start;
        rlp->ranges[i + 1] = end;
        rlp->used += 2;
    }
}

static void
#ifdef __STDC__
ordered_range_insert(unsigned long c, char *name, int len)
#else
ordered_range_insert(c, name, len)
unsigned long c;
char *name;
int len;
#endif
{
    int i, j;
    unsigned long s, e;
    _ranges_t *rlp;

    if (len == 0)
      return;

    /*
     * Deal with directionality and Boundary Neutral codes introduced in
     * Unicode 3.0.
     */
    if ((len == 2 && memcmp(name, "BN", 2) == 0) ||
        (len == 3 &&
         (memcmp(name, "NSM", 3) == 0 || memcmp(name, "PDF", 3) == 0 ||
          memcmp(name, "LRE", 3) == 0 || memcmp(name, "LRO", 3) == 0 ||
          memcmp(name, "RLE", 3) == 0 || memcmp(name, "RLO", 3) == 0))) {
        /*
         * Mark all of these as Other Neutral to preserve compatibility with
         * older versions.
         */
        len = 2;
        name = "ON";
    }

    for (i = 0; i < NUMPROPS; i++) {
        if (props[i].len == len && memcmp(props[i].name, name, len) == 0)
          break;
    }

    if (i == NUMPROPS)
      return;

    /*
     * Have a match, so insert the code in order.
     */
    rlp = &proptbl[i];

    /*
     * Resize the range list if necessary.
     */
    if (rlp->used == rlp->size) {
        if (rlp->size == 0)
          rlp->ranges = (unsigned long *)
              malloc(sizeof(unsigned long) << 3);
        else
          rlp->ranges = (unsigned long *)
              realloc((char *) rlp->ranges,
                      sizeof(unsigned long) * (rlp->size + 8));
        rlp->size += 8;
    }

    /*
     * If this is the first code for this property list, just add it
     * and return.
     */
    if (rlp->used == 0) {
        rlp->ranges[0] = rlp->ranges[1] = c;
        rlp->used += 2;
        return;
    }

    /*
     * Optimize the cases of extending the last range and adding new ranges to
     * the end.
     */
    j = rlp->used - 1;
    e = rlp->ranges[j];
    s = rlp->ranges[j - 1];

    if (c == e + 1) {
        /*
         * Extend the last range.
         */
        rlp->ranges[j] = c;
        return;
    }

    if (c > e + 1) {
        /*
         * Start another range on the end.
         */
        j = rlp->used;
        rlp->ranges[j] = rlp->ranges[j + 1] = c;
        rlp->used += 2;
        return;
    }

    if (c >= s)
      /*
       * The code is a duplicate of a code in the last range, so just return.
       */
      return;

    /*
     * The code should be inserted somewhere before the last range in the
     * list.  Locate the insertion point.
     */
    for (i = 0;
         i < rlp->used && c > rlp->ranges[i + 1] + 1; i += 2) ;

    s = rlp->ranges[i];
    e = rlp->ranges[i + 1];

    if (c == e + 1)
      /*
       * Simply extend the current range.
       */
      rlp->ranges[i + 1] = c;
    else if (c < s) {
        /*
         * Add a new entry before the current location.  Shift all entries
         * before the current one up by one to make room.
         */
        for (j = rlp->used; j > i; j -= 2) {
            rlp->ranges[j] = rlp->ranges[j - 2];
            rlp->ranges[j + 1] = rlp->ranges[j - 1];
        }
        rlp->ranges[i] = rlp->ranges[i + 1] = c;

        rlp->used += 2;
    }
}

static void
#ifdef __STDC__
add_decomp(unsigned long code)
#else
add_decomp(code)
unsigned long code;
#endif
{
    unsigned long i, j, size;

    /*
     * Add the code to the composite property.
     */
    ordered_range_insert(code, "Cm", 2);

    /*
     * Locate the insertion point for the code.
     */
    for (i = 0; i < decomps_used && code > decomps[i].code; i++) ;

    /*
     * Allocate space for a new decomposition.
     */
    if (decomps_used == decomps_size) {
        if (decomps_size == 0)
          decomps = (_decomp_t *) malloc(sizeof(_decomp_t) << 3);
        else
          decomps = (_decomp_t *)
              realloc((char *) decomps,
                      sizeof(_decomp_t) * (decomps_size + 8));
        (void) memset((char *) (decomps + decomps_size), 0,
                      sizeof(_decomp_t) << 3);
        decomps_size += 8;
    }

    if (i < decomps_used && code != decomps[i].code) {
        /*
         * Shift the decomps up by one if the codes don't match.
         */
        for (j = decomps_used; j > i; j--)
          (void) memcpy((char *) &decomps[j], (char *) &decomps[j - 1],
                        sizeof(_decomp_t));
        /*
         * Make sure the slot opened up is initialized so we don't try to
         * use the space taken by the decomposition that was just here.
         */
        (void) memset((char *) &decomps[i], 0, sizeof(_decomp_t));
    }

    /*
     * Insert or replace a decomposition.
     */
    size = dectmp_size + (4 - (dectmp_size & 3));
    if (decomps[i].size < size) {
        if (decomps[i].size == 0)
          decomps[i].decomp = (unsigned long *)
              malloc(sizeof(unsigned long) * size);
        else
          decomps[i].decomp = (unsigned long *)
              realloc((char *) decomps[i].decomp,
                      sizeof(unsigned long) * size);
        decomps[i].size = size;
    }

    if (decomps[i].code != code)
      decomps_used++;

    decomps[i].code = code;
    decomps[i].used = dectmp_size;
    (void) memcpy((char *) decomps[i].decomp, (char *) dectmp,
                  sizeof(unsigned long) * dectmp_size);

    /*
     * NOTICE: This needs changing later so it is more general than simply
     * pairs.  This calculation is done here to simplify allocation elsewhere.
     */
    if (dectmp_size == 2)
      comps_size++;
}

static void
#ifdef __STDC__
add_title(unsigned long code)
#else
add_title(code)
unsigned long code;
#endif
{
    unsigned long i, j;

    /*
     * Always map the code to itself.
     */
    cases[2] = code;

    if (title_used == title_size) {
        if (title_size == 0)
          title = (_case_t *) malloc(sizeof(_case_t) << 3);
        else
          title = (_case_t *) realloc((char *) title,
                                      sizeof(_case_t) * (title_size + 8));
        title_size += 8;
    }

    /*
     * Locate the insertion point.
     */
    for (i = 0; i < title_used && code > title[i].key; i++) ;

    if (i < title_used) {
        /*
         * Shift the array up by one.
         */
        for (j = title_used; j > i; j--)
          (void) memcpy((char *) &title[j], (char *) &title[j - 1],
                        sizeof(_case_t));
    }

    title[i].key = cases[2];    /* Title */
    title[i].other1 = cases[0]; /* Upper */
    title[i].other2 = cases[1]; /* Lower */

    title_used++;
}

static void
#ifdef __STDC__
add_upper(unsigned long code)
#else
add_upper(code)
unsigned long code;
#endif
{
    unsigned long i, j;

    /*
     * Always map the code to itself.
     */
    cases[0] = code;

    /*
     * If the title case character is not present, then make it the same as
     * the upper case.
     */
    if (cases[2] == 0)
      cases[2] = code;

    if (upper_used == upper_size) {
        if (upper_size == 0)
          upper = (_case_t *) malloc(sizeof(_case_t) << 3);
        else
          upper = (_case_t *) realloc((char *) upper,
                                      sizeof(_case_t) * (upper_size + 8));
        upper_size += 8;
    }

    /*
     * Locate the insertion point.
     */
    for (i = 0; i < upper_used && code > upper[i].key; i++) ;

    if (i < upper_used) {
        /*
         * Shift the array up by one.
         */
        for (j = upper_used; j > i; j--)
          (void) memcpy((char *) &upper[j], (char *) &upper[j - 1],
                        sizeof(_case_t));
    }

    upper[i].key = cases[0];    /* Upper */
    upper[i].other1 = cases[1]; /* Lower */
    upper[i].other2 = cases[2]; /* Title */

    upper_used++;
}

static void
#ifdef __STDC__
add_lower(unsigned long code)
#else
add_lower(code)
unsigned long code;
#endif
{
    unsigned long i, j;

    /*
     * Always map the code to itself.
     */
    cases[1] = code;

    /*
     * If the title case character is empty, then make it the same as the
     * upper case.
     */
    if (cases[2] == 0)
      cases[2] = cases[0];

    if (lower_used == lower_size) {
        if (lower_size == 0)
          lower = (_case_t *) malloc(sizeof(_case_t) << 3);
        else
          lower = (_case_t *) realloc((char *) lower,
                                      sizeof(_case_t) * (lower_size + 8));
        lower_size += 8;
    }

    /*
     * Locate the insertion point.
     */
    for (i = 0; i < lower_used && code > lower[i].key; i++) ;

    if (i < lower_used) {
        /*
         * Shift the array up by one.
         */
        for (j = lower_used; j > i; j--)
          (void) memcpy((char *) &lower[j], (char *) &lower[j - 1],
                        sizeof(_case_t));
    }

    lower[i].key = cases[1];    /* Lower */
    lower[i].other1 = cases[0]; /* Upper */
    lower[i].other2 = cases[2]; /* Title */

    lower_used++;
}

static void
#ifdef __STDC__
ordered_ccl_insert(unsigned long c, unsigned long ccl_code)
#else
ordered_ccl_insert(c, ccl_code)
unsigned long c, ccl_code;
#endif
{
    unsigned long i;

    if (ccl_used == ccl_size) {
        if (ccl_size == 0)
          ccl = (unsigned long *) malloc(sizeof(unsigned long) * 24);
        else
          ccl = (unsigned long *)
              realloc((char *) ccl, sizeof(unsigned long) * (ccl_size + 24));
        ccl_size += 24;
    }

    /*
     * Optimize adding the first item.
     */
    if (ccl_used == 0) {
        ccl[0] = ccl[1] = c;
        ccl[2] = ccl_code;
        ccl_used += 3;
        return;
    }

    /*
     * Handle the special case of extending the range on the end.  This
     * requires that the combining class codes are the same.
     */
    if (ccl_code == ccl[ccl_used - 1] && c == ccl[ccl_used - 2] + 1) {
        ccl[ccl_used - 2] = c;
        return;
    }

    /*
     * Handle the special case of adding another range on the end.
     */
    if (c > ccl[ccl_used - 2] + 1 ||
        (c == ccl[ccl_used - 2] + 1 && ccl_code != ccl[ccl_used - 1])) {
        ccl[ccl_used++] = c;
        ccl[ccl_used++] = c;
        ccl[ccl_used++] = ccl_code;
        return;
    }

    /*
     * Locate either the insertion point or range for the code.
     */
    for (i = 0; i < ccl_used && c > ccl[i + 1] + 1; i += 3) ;

    if (ccl_code == ccl[i + 2] && c == ccl[i + 1] + 1) {
        /*
         * Extend an existing range.
         */
        ccl[i + 1] = c;
        return;
    }

    /*
     * We know that either the combining class code did not match or that the
     * character is greater than the end of the range plus 1. So we need to a)
     * insert a new range at the current spot if the combining class codes did
     * not match, or b) insert a new range after the current spot if the
     * character is greater than the end of the range plus 1.
     */
    if (c >= ccl[i+1]+1)
      /*
       * We will need to open up a spot after the current one. Because we
       * know that the combining class is different at this point. This
       * causes the ranges to be inserted in order.
       */
      i += 3;

    /*
     * Start a new range before the current location.
     */
    memmove(&ccl[i+3], &ccl[i], sizeof(unsigned long) * (ccl_used - i));
    ccl[i] = ccl[i + 1] = c;
    ccl[i + 2] = ccl_code;
    ccl_used += 3;
}

/*
 * Adds a number if it does not already exist and returns an index value
 * multiplied by 2.
 */
static unsigned long
#ifdef __STDC__
make_number(short num, short denom)
#else
make_number(num, denom)
short num, denom;
#endif
{
    unsigned long n;

    /*
     * Determine if the number already exists.
     */
    for (n = 0; n < nums_used; n++) {
        if (nums[n].numerator == num && nums[n].denominator == denom)
          return n << 1;
    }

    if (nums_used == nums_size) {
        if (nums_size == 0)
          nums = (_num_t *) malloc(sizeof(_num_t) << 3);
        else
          nums = (_num_t *) realloc((char *) nums,
                                    sizeof(_num_t) * (nums_size + 8));
        nums_size += 8;
    }

    n = nums_used++;
    nums[n].numerator = num;
    nums[n].denominator = denom;

    return n << 1;
}

static void
#ifdef __STDC__
add_number(unsigned long code, short num, short denom)
#else
add_number(code, num, denom)
unsigned long code;
short num, denom;
#endif
{
    unsigned long i, j;

    /*
     * Insert the code in order.
     */
    for (i = 0; i < ncodes_used && code > ncodes[i].code; i++) ;

    /*
     * Handle the case of the codes matching and simply replace the number
     * that was there before.
     */
    if (i < ncodes_used && code == ncodes[i].code) {
        ncodes[i].idx = make_number(num, denom);
        return;
    }

    /*
     * Resize the array if necessary.
     */
    if (ncodes_used == ncodes_size) {
        if (ncodes_size == 0)
          ncodes = (_codeidx_t *) malloc(sizeof(_codeidx_t) << 3);
        else
          ncodes = (_codeidx_t *)
              realloc((char *) ncodes, sizeof(_codeidx_t) * (ncodes_size + 8));

        ncodes_size += 8;
    }

    /*
     * Shift things around to insert the code if necessary.
     */
    if (i < ncodes_used) {
        for (j = ncodes_used; j > i; j--) {
            ncodes[j].code = ncodes[j - 1].code;
            ncodes[j].idx = ncodes[j - 1].idx;
        }
    }
    ncodes[i].code = code;
    ncodes[i].idx = make_number(num, denom);

    ncodes_used++;
}

/*****************************************************************
 *
 * TRIE
 *
 * Section added to create a fast lookup table for lower case mappings
 * to support case insensitive comparison of UTF-8 encoded characters.
 *
 *****************************************************************/

/*
 * Convert the UTF-32 code to UTF-8.
 */
static int
#ifdef __STDC__
getutf8(unsigned long code, unsigned char *buf, int buflen)
#else
getutf8(code, buf, buflen)
unsigned long code;
unsigned char *buf;
int buflen;
#endif
{
    int i = 0;

    if (code < 0x80 && i + 1 < buflen) {
        buf[i++] = code;
    } else if (code < 0x800 && i + 2 < buflen) {
        buf[i++] = 0xc0 | (code>>6);
        buf[i++] = 0x80 | (code & 0x3f);
    } else if (code < 0x10000 && i + 3 < buflen) {
        buf[i++] = 0xe0 | (code>>12);
        buf[i++] = 0x80 | ((code>>6) & 0x3f);
        buf[i++] = 0x80 | (code & 0x3f);
    } else if (code < 0x200000 && i + 4 < buflen) {
        buf[i++] = 0xf0 | (code>>18);
        buf[i++] = 0x80 | ((code>>12) & 0x3f);
        buf[i++] = 0x80 | ((code>>6) & 0x3f);
        buf[i++] = 0x80 | (code & 0x3f);
    } else if (code < 0x4000000 && i + 5 < buflen) {
        buf[i++] = 0xf8 | (code>>24);
        buf[i++] = 0x80 | ((code>>18) & 0x3f);
        buf[i++] = 0x80 | ((code>>12) & 0x3f);
        buf[i++] = 0x80 | ((code>>6) & 0x3f);
        buf[i++] = 0x80 | (code & 0x3f);
    } else if (code <= 0x7fffffff && i + 6 < buflen) {
        buf[i++] = 0xfc | (code>>30);
        buf[i++] = 0x80 | ((code>>24) & 0x3f);
        buf[i++] = 0x80 | ((code>>18) & 0x3f);
        buf[i++] = 0x80 | ((code>>12) & 0x3f);
        buf[i++] = 0x80 | ((code>>6) & 0x3f);
        buf[i++] = 0x80 | (code & 0x3f);
    }
    return i;
}

static unsigned short
#ifdef __STDC__
trie_getnode(unsigned short k)
#else
trie_getnode(k)
unsigned short k;
#endif
{
    if (trie_used == trie_size) {
        if (trie_size == 0)
          trie = (_tnode_t *) malloc(sizeof(_tnode_t) * 10);
        else
          trie = (_tnode_t *) realloc((char *) trie, sizeof(_tnode_t) *
                                      (trie_size + 10));
        trie_size += 10;
    }
    trie[trie_used].key = k;
    trie[trie_used].sibs = trie[trie_used].kids = 0;
    return trie_used++;
}

static void
#ifdef __STDC__
trie_insert(unsigned long upper, unsigned long lower)
#else
trie_insert(upper, lower)
unsigned long upper, lower;
#endif
{
    int i, n, t, l, ulen, llen;
    unsigned char ubuf[16], lbuf[16];

    /*
     * Make sure the trie is initialized.
     */
    if (trie_size == 0)
      (void) trie_getnode(0);

    ulen = getutf8(upper, ubuf, 16);
    llen = getutf8(lower, lbuf, 16);

    for (i = t = 0; i < ulen; i++) {
        if (trie[t].kids == 0) {
            n = trie_getnode(ubuf[i]);
            trie[t].kids = n;
            t = n;
        } else if (trie[trie[t].kids].key == ubuf[i])
          t = trie[t].kids;
        else if (trie[trie[t].kids].key > ubuf[i]) {
            n = trie_getnode(ubuf[i]);
            trie[n].sibs = trie[t].kids;
            trie[t].kids = n;
            t = n;
        } else {
            t = trie[t].kids;
            for (l = t; trie[t].sibs && trie[t].key < ubuf[i]; ) {
                l = t;
                t = trie[t].sibs;
            }
            if (trie[t].key < ubuf[i]) {
                n = trie_getnode(ubuf[i]);
                trie[t].sibs = n;
                t = n;
            } else if (trie[t].key > ubuf[i]) {
                n = trie_getnode(ubuf[i]);
                trie[n].sibs = t;
                trie[l].sibs = n;
                t = n;
            }
        }
    }

    /*
     * Add the mapping to the lower case mapping list.
     */
    if (ltable_used + 5 >= ltable_size) {
        if (ltable_size == 0) {
          ltable = (unsigned char *) malloc(sizeof(unsigned char) * 64);
          /*
           * The first byte is always unused so offset values will always
           * be non-zero.
           */
          ltable_used = 1;
        } else
          ltable = (unsigned char *)
              realloc((char *) ltable,
                      sizeof(unsigned char) * (ltable_size + 64));
        ltable_size += 64;
    }
    /*
     * At this point, trie[t] is the node where we want to store the offset
     * into the lower case mappings.
     */
    trie[t].offset = ltable_used;

    /*
     * Add the length of the mapping.
     */
    ltable[ltable_used++] = llen;
    memcpy(ltable + ltable_used, lbuf, llen);
    ltable_used += llen;
}

static void
#ifdef __STDC__
trie_compact(unsigned short t)
#else
trie_compact(t)
unsigned short t;
#endif
{
    unsigned short n, l, lp;
    
    for (l = t, lp = ltrie_used; t; t = trie[t].sibs) {

        if (ltrie_used + 3 >= ltrie_size) {
            if (ltrie_size == 0)
              ltrie = (unsigned short *) malloc(sizeof(unsigned short) * 128);
            else
              ltrie = (unsigned short *)
                  realloc((char *) ltrie,
                          sizeof(unsigned short) * (ltrie_size + 128));
            ltrie_size += 128;
        }

        ltrie[ltrie_used] = trie[t].key;
        ltrie[ltrie_used + 1] = trie[t].offset - 1;

        n = 1;

        /*
         * If the node has children, then set the flag in the
         * unused bits of the key.
         */
        if (trie[t].kids) {
            n = 2;
            ltrie[ltrie_used] |= 0x0100;
            ltrie[ltrie_used + 2] = trie[t].kids;
        }
        ltrie_used += n + 1;
    }

    /*
     * Now go back to process the children.
     */
    for (t = l; t; t = trie[t].sibs, lp += 2) {
        if (trie[t].kids) {
            ltrie[lp+2] = ltrie_used;
            lp++;
            trie_compact(trie[t].kids);
        }
    }
}

/*****************************************************************
 *
 * TRIE
 *
 * End of section.
 *
 *****************************************************************/

/*
 * This is a routine to read a line into a fixed size buffer. This is to
 * restrict any attempts to overflow buffers using external data.
 */
static int
#ifdef __STDC__
ucgetline(FILE *in, char *buf, int bufsize)
#else
ucgetline(in, buf, bufsize)
FILE *in;
char *buf;
int bufsize;
#endif
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

/*
 * This routine assumes that the line is a valid Unicode Character Database
 * entry.
 */
static void
#ifdef __STDC__
read_cdata(FILE *in)
#else
read_cdata(in)
FILE *in;
#endif
{
    unsigned long i, lineno, skip, code, ccl_code;
    short wnum, neg, number[2];
    char line[512], *s, *e;

    lineno = skip = 0;
    while (ucgetline(in, line, 512) != EOF) {
        lineno++;

        /*
         * Skip blank lines and lines that start with a '#'.
         */
        if (line[0] == 0 || line[0] == '#')
          continue;

        /*
         * If lines need to be skipped, do it here.
         */
        if (skip) {
            skip--;
            continue;
        }

        /*
         * Collect the code.  The code can be up to 6 hex digits in length to
         * allow surrogates to be specified.
         */
        for (s = line, i = code = 0; *s != ';' && i < 6; i++, s++) {
            code <<= 4;
            if (*s >= '0' && *s <= '9')
              code += *s - '0';
            else if (*s >= 'A' && *s <= 'F')
              code += (*s - 'A') + 10;
            else if (*s >= 'a' && *s <= 'f')
              code += (*s - 'a') + 10;
        }

        /*
         * Handle the following special cases:
         * 1. 3400-4DB5 CJK Ideographic Extension A.
         * 2. 4E00-9FA5 CJK Ideographs.
         * 3. AC00-D7A3 Hangul Syllables.
         * 4. D800-DFFF Surrogates.
         * 5. E000-F8FF Private Use Area.
         * 6. F900-FA2D Han compatibility.
         * 7. 20000-2A6D6 CJK Ideographic Extension B.
         * 8. F0000-FFFFD Plane 15 Private Use Area.
         * 9. 100000-10FFFD Plane 16 Private Use Area.
         */
        switch (code) {
          case 0x3400:
            /*
             * Extension A of Han ideographs.
             */
            add_range(0x3400, 0x4DB5, "Lo", "L");

            /*
             * Add the characters to the defined category.
             */
            add_range(0x3400, 0x4DB5, "Cp", 0);

            skip = 1;
            break;
          case 0x4e00:
            /*
             * The Han ideographs.
             */
            add_range(0x4e00, 0x9fff, "Lo", "L");

            /*
             * Add the characters to the defined category.
             */
            add_range(0x4e00, 0x9fa5, "Cp", 0);

            skip = 1;
            break;
          case 0xac00:
            /*
             * The Hangul syllables.
             */
            add_range(0xac00, 0xd7a3, "Lo", "L");

            /*
             * Add the characters to the defined category.
             */
            add_range(0xac00, 0xd7a3, "Cp", 0);

            skip = 1;
            break;
          case 0xd800:
            /*
             * Make a range of all surrogates and assume some default
             * properties.
             */
            add_range(0x010000, 0x10ffff, "Cs", "L");
            skip = 5;
            break;
          case 0xe000:
            /*
             * The Private Use area.  Add with a default set of properties.
             */
            add_range(0xe000, 0xf8ff, "Co", "L");
            skip = 1;
            break;
          case 0xf900:
            /*
             * The CJK compatibility area.
             */
            add_range(0xf900, 0xfaff, "Lo", "L");

            /*
             * Add the characters to the defined category.
             */
            add_range(0xf900, 0xfaff, "Cp", 0);

            skip = 1;
            break;
          case 0x20000:
            /*
             * Extension B of Han ideographs.
             */
            add_range(0x20000, 0x2A6D6, "Lo", "L");

            /*
             * Add the characters to the defined category.
             */
            add_range(0x20000, 0x2A6D6, "Cp", 0);

            skip = 1;
            break;
          case 0xF0000:
            /*
             * The Plane 15 Private Use area.
             * Add with a default set of properties.
             */
            add_range(0xF0000, 0xffffd, "Co", "L");
            skip = 1;
            break;
          case 0x100000:
            /*
             * The Plane 16 Private Use area.
             * Add with a default set of properties.
             */
            add_range(0x100000, 0x10fffd, "Co", "L");
            skip = 1;
            break;
        }

        if (skip)
          continue;

        /*
         * Add the code to the defined category.
         */
        ordered_range_insert(code, "Cp", 2);

        /*
         * Locate the first general category field.
         */
        for (i = 0; *s != 0 && i < 2; s++) {
            if (*s == ';')
              i++;
        }
        for (e = s; *e && *e != ';'; e++) ;
    
        ordered_range_insert(code, s, e - s);

        /*
         * Locate the combining class code.
         */
        for (s = e; *s != 0 && i < 3; s++) {
            if (*s == ';')
              i++;
        }

        /*
         * Convert the combining class code from decimal.
         */
        for (ccl_code = 0, e = s; *e && *e != ';'; e++)
          ccl_code = (ccl_code * 10) + (*e - '0');

        /*
         * Add the code if it not 0.
         */
        if (ccl_code != 0)
          ordered_ccl_insert(code, ccl_code);

        /*
         * Locate the bidi class field.
         */
        for (s = e; *s != 0 && i < 4; s++) {
            if (*s == ';')
              i++;
        }
        for (e = s; *e && *e != ';'; e++) ;

        ordered_range_insert(code, s, e - s);

        /*
         * Check for a decomposition.
         */
        s = ++e;

/*
 * The following section bracketed by "#ifndef ARABIC_DECOMP" allows all the
 * Arabic presentation forms to be added to the decomposition table so it can
 * be used for "decomposing" presentation form character codes into the
 * nominal character codes.
 */

#ifndef ARABIC_DECOMP

        if (*s != ';' && *s != '<') {

#else /* ARABIC_DECOMP */

#define isap(cc) ((0xFB50 <= (cc) && (cc) <= 0xFBB1) ||\
                  (0xFBD3 <= (cc) && (cc) <= 0xFD3D) ||\
                  (0xFD50 <= (cc) && (cc) <= 0xFD8F) ||\
                  (0xFD92 <= (cc) && (cc) <= 0xFDC7) ||\
                  (0xFDF0 <= (cc) && (cc) <= 0xFDFB) ||\
                  (0xFE70 <= (cc) && (cc) <= 0xFE72) ||\
                  (0xFE74 <= (cc) && (cc) <= 0xFE74) ||\
                  (0xFE76 <= (cc) && (cc) <= 0xFEFC))

        if (*s != ';' && (*s != '<' || isap(code))) {
            if (isap(code))
              while (*s && *s != '>' && *s != ';') s++;

#endif /* ARABIC DECOMP */

            /*
             * Collect the codes of the decomposition.
             */
            for (dectmp_size = 0; *s != ';'; ) {
                /*
                 * Skip all leading non-hex digits.
                 */
                while (!ishdigit(*s))
                  s++;

                for (dectmp[dectmp_size] = 0; ishdigit(*s); s++) {
                    dectmp[dectmp_size] <<= 4;
                    if (*s >= '0' && *s <= '9')
                      dectmp[dectmp_size] += *s - '0';
                    else if (*s >= 'A' && *s <= 'F')
                      dectmp[dectmp_size] += (*s - 'A') + 10;
                    else if (*s >= 'a' && *s <= 'f')
                      dectmp[dectmp_size] += (*s - 'a') + 10;
                }
                dectmp_size++;
            }

            /*
             * If there are any codes in the temporary decomposition array,
             * then add the character with its decomposition.
             */
            if (dectmp_size > 0)
              add_decomp(code);
        }

        /*
         * Skip to the number field.
         */
        for (i = 0; i < 3 && *s; s++) {
            if (*s == ';')
              i++;
        }

        /*
         * Scan the number in.
         */
        number[0] = number[1] = 0;
        for (e = s, neg = wnum = 0; *e && *e != ';'; e++) {
            if (*e == '-') {
                neg = 1;
                continue;
            }

            if (*e == '/') {
                /*
                 * Move the the denominator of the fraction.
                 */
                if (neg)
                  number[wnum] *= -1;
                neg = 0;
                e++;
                wnum++;
            }
            number[wnum] = (number[wnum] * 10) + (*e - '0');
        }

        if (e > s) {
            /*
             * Adjust the denominator in case of integers and add the number.
             */
            if (wnum == 0)
              number[1] = number[0];

            add_number(code, number[0], number[1]);
        }

        /*
         * Skip to the start of the possible case mappings.
         */
        for (s = e, i = 0; i < 4 && *s; s++) {
            if (*s == ';')
              i++;
        }

        /*
         * Collect the case mappings.
         */
        cases[0] = cases[1] = cases[2] = 0;
        for (i = 0; i < 3; i++) {
            while (ishdigit(*s)) {
                cases[i] <<= 4;
                if (*s >= '0' && *s <= '9')
                  cases[i] += *s - '0';
                else if (*s >= 'A' && *s <= 'F')
                  cases[i] += (*s - 'A') + 10;
                else if (*s >= 'a' && *s <= 'f')
                  cases[i] += (*s - 'a') + 10;
                s++;
            }
            if (*s == ';')
              s++;
        }
        if (cases[0] && cases[1]) {
            /*
             * Add the upper and lower mappings for a title case character.
             */
            add_title(code);

            /*
             * Mark the code as having the Bc property (bi-cameral, having
             * case).
             */
            ordered_range_insert(code, "Bc", 2);

            /*
             * Add the title case to lower case mapping to the UTF-8 lookup
             * table.
             */
            trie_insert(code, cases[1]);
        } else if (cases[1]) {
            /*
             * Add the lower and title case mappings for the upper case
             * character.
             */
            add_upper(code);

            /*
             * Mark the code as having the Bc property (bi-cameral, having
             * case).
             */
            ordered_range_insert(code, "Bc", 2);

            /*
             * Add the upper case to lower case mapping to the UTF-8 lookup
             * table.
             */
            trie_insert(code, cases[1]);
        } else if (cases[0]) {
            /*
             * Add the upper and title case mappings for the lower case
             * character.
             */
            add_lower(code);

            /*
             * Mark the code as having the Bc property (bi-cameral, having
             * case).
             */
            ordered_range_insert(code, "Bc", 2);
        }
    }
}

static _decomp_t *
#ifdef __STDC__
find_decomp(unsigned long code)
#else
find_decomp(code)
unsigned long code;
#endif
{
    long l, r, m;

    l = 0;
    r = decomps_used - 1;
    while (l <= r) {
        m = (l + r) >> 1;
        if (code > decomps[m].code)
          l = m + 1;
        else if (code < decomps[m].code)
          r = m - 1;
        else
          return &decomps[m];
    }
    return 0;
}

static void
#ifdef __STDC__
decomp_it(_decomp_t *d)
#else
decomp_it(d)
_decomp_t *d;
#endif
{
    unsigned long i;
    _decomp_t *dp;

    for (i = 0; i < d->used; i++) {
        if ((dp = find_decomp(d->decomp[i])) != 0)
          decomp_it(dp);
        else
          dectmp[dectmp_size++] = d->decomp[i];
    }
}

/*
 * Expand all decompositions by recursively decomposing each character
 * in the decomposition.
 */
static void
#ifdef __STDC__
expand_decomp(void)
#else
expand_decomp()
#endif
{
    unsigned long i;

    for (i = 0; i < decomps_used; i++) {
        dectmp_size = 0;
        decomp_it(&decomps[i]);
        if (dectmp_size > 0)
          add_decomp(decomps[i].code);
    }
}

/*
 * Load composition exclusion data
 */
static void
#ifdef __STDC__
read_compexdata(FILE *in)
#else
read_compexdata(in)
FILE *in;
#endif
{
    int i;
    unsigned long code;
    char line[512], *s;

    (void) memset((char *) compexs, 0, sizeof(unsigned long) << 11);

    while (ucgetline(in, line, 512) != EOF) {
        /*
         * Skip blank lines and lines that start with a '#'.
         */
        if (line[0] == 0 || line[0] == '#')
	    continue;

	/*
         * Collect the code.  Assume max 8 digits
         */

	for (s = line, i = code = 0; *s && *s != '#' && i < 8; i++, s++) {
            code <<= 4;
            if (*s >= '0' && *s <= '9')
              code += *s - '0';
            else if (*s >= 'A' && *s <= 'F')
              code += (*s - 'A') + 10;
            else if (*s >= 'a' && *s <= 'f')
              code += (*s - 'a') + 10;
        }
        COMPEX_SET(code);
    }
}

/*
 * Inserts the composition in ascending order.
 */
static void
#ifdef __STDC__
ordered_comp_insert(unsigned long code, unsigned long d1, unsigned long d2)
#else
ordered_comp_insert(code, d1, d2)
unsigned long code, d1, d2;
#endif
{
    int i;

    for (i = 0; i < comps_used && d1 > comps[i].code1; i++) ;
    for (; i < comps_used && d1 == comps[i].code1 && d2 > comps[i].code2; i++);
    for (; i < comps_used &&
             d1 == comps[i].code1 &&
             d2 == comps[i].code2 &&
             code > comps[i].comp; i++);

    if (i < comps_used)
      /*
       * Need to shift the other values forward by one.
       */
      memmove(&comps[i+1], &comps[i], sizeof(_comp_t) * (comps_used - i + 1));

    comps[i].comp = code;
    comps[i].count = 2;
    comps[i].code1 = d1;
    comps[i].code2 = d2;

    comps_used++;
}

/*
 * Creates array of compositions from decomposition array
 */
static void
#ifdef __STDC__
create_comps(void)
#else
create_comps()     
#endif
{
    unsigned long i;

    comps = (_comp_t *) malloc(comps_size * sizeof(_comp_t));

    for (i = 0; i < decomps_used; i++) {
        /*
         * Eliminate all decompositions longer than two. Packages that
         * had longer decompositions are no longer being used.
         */
	if (decomps[i].used != 2 || COMPEX_TEST(decomps[i].code))
	    continue;
        ordered_comp_insert(decomps[i].code, decomps[i].decomp[0],
                            decomps[i].decomp[1]);
    }
}

static int
#ifdef __STDC__
utf8_length(unsigned long code)
#else
utf8_length(code)
unsigned long code;
#endif
{
    int i = 0;

    if (code < 0x80)
      i = 1;
    else if (code < 0x800)
      i = 2;
    else if (code < 0x10000)
      i = 3;
    else if (code < 0x200000)
      i = 4;
    else if (code < 0x4000000)
      i = 5;
    else if (code <= 0x7ffffff)
      i = 6;

    return i;
}

#define UTPUTC(ch, out) if (putc((ch), out) < 0) return EOF

static int
#ifdef __STDC__
utf8_write(unsigned long code, FILE *out)
#else
utf8_write(code, out)
unsigned long code;
FILE *out;
#endif
{
    if (code < 0x80) {
        UTPUTC(code & 0xff, out);
    } else if (code < 0x800) {
        UTPUTC(0xc0 | (code >> 6), out);
        UTPUTC(0x80 | (code & 0x3f), out);
    } else if (code < 0x10000) {
        UTPUTC(0xe0 | (code >> 12), out);
        UTPUTC(0x80 | ((code >> 6) & 0x3f), out);
        UTPUTC(0x80 | (code & 0x3f), out);
    } else if (code < 0x200000) {
        UTPUTC(0xf0 | (code >> 18), out);
        UTPUTC(0x80 | ((code >> 12) & 0x3f), out);
        UTPUTC(0x80 | ((code >> 6) & 0x3f), out);
        UTPUTC(0x80 | (code & 0x3f), out);
    } else if (code < 0x4000000) {
        UTPUTC(0xf8 | (code >> 24), out);
        UTPUTC(0x80 | ((code >> 18) & 0x3f), out);
        UTPUTC(0x80 | ((code >> 12) & 0x3f), out);
        UTPUTC(0x80 | ((code >> 6) & 0x3f), out);
        UTPUTC(0x80 | (code & 0x3f), out);
    } else if (code <= 0x7ffffff) {
        UTPUTC(0xfc | (code >> 30), out);
        UTPUTC(0x80 | ((code >> 24) & 0x3f), out);
        UTPUTC(0x80 | ((code >> 18) & 0x3f), out);
        UTPUTC(0x80 | ((code >> 12) & 0x3f), out);
        UTPUTC(0x80 | ((code >> 6) & 0x3f), out);
        UTPUTC(0x80 | (code & 0x3f), out);
    }

    return 0;
}

static void
#ifdef __STDC__
write_cdata(char *opath)
#else
write_cdata(opath)
char *opath;
#endif
{
    FILE *out;
    unsigned long i, j, idx, bytes;
    unsigned short casecnt[2];
    char zero = 0, path[BUFSIZ];

    /*****************************************************************
     *
     * Generate the ctype data.
     *
     *****************************************************************/

    /*
     * Open the ctype.dat file.
     */
    sprintf(path, "%s/ctype.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    /*
     * Collect the offsets for the properties.  The offsets array is
     * on a 4-byte boundary to keep things efficient for architectures
     * that need such a thing.
     */
    for (i = idx = 0; i < NUMPROPS; i++) {
        propcnt[i] = (proptbl[i].used != 0) ? idx : 0xffff;
        idx += proptbl[i].used;
    }

    /*
     * Add the sentinel index which is used by the binary search as the upper
     * bound for a search.
     */
    propcnt[i] = idx;

    /*
     * Record the actual number of property lists.  This may be different than
     * the number of offsets actually written because of aligning on a 4-byte
     * boundary.
     */
    hdr[1] = NUMPROPS;

    /*
     * Calculate the byte count needed for the offsets and the ranges.
     */
    bytes = (sizeof(unsigned short) * NEEDPROPS) +
        (sizeof(unsigned long) * idx);
        
    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write the byte count.
     */
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    /*
     * Write the property list counts.
     */
    fwrite((char *) propcnt, sizeof(unsigned short), NEEDPROPS, out);

    /*
     * Write the property lists.
     */
    for (i = 0; i < NUMPROPS; i++) {
        if (proptbl[i].used > 0)
          fwrite((char *) proptbl[i].ranges, sizeof(unsigned long),
                 proptbl[i].used, out);
    }

    fclose(out);

    /*****************************************************************
     *
     * Generate the UTF-8 ctype data.
     *
     *****************************************************************/

    /*
     * Open the ctype.dat file.
     */
    sprintf(path, "%s/utf8type.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    /*
     * Collect the offsets for the properties.
     */
    for (i = idx = 0; i < NUMPROPS; i++) {
        propcnt[i] = (proptbl[i].used != 0) ? idx : 0xffff;
        for (j = 0; j < proptbl[i].used; j++)
          idx += utf8_length(proptbl[i].ranges[j]);
    }

    /*
     * Add the sentinel index which is used by the binary search as the upper
     * bound for a search.
     */
    propcnt[i] = idx;

    /*
     * Record the actual number of property lists.  This may be different than
     * the number of offsets actually written because of aligning on a 4-byte
     * boundary.
     */
    hdr[1] = NUMPROPS;

    /*
     * Calculate the byte count needed for the offsets and the ranges.
     * Make sure the byte count is adjusted to a 4-byte boundary.
     */
    bytes = (sizeof(unsigned short) * NEEDPROPS) +
        (sizeof(unsigned char) * (idx + (4 - (idx & 3))));
        
    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write the byte count.
     */
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    /*
     * Write the property list counts.
     */
    fwrite((char *) propcnt, sizeof(unsigned short), NEEDPROPS, out);

    /*
     * Write the property lists.
     */
    for (i = 0; i < NUMPROPS; i++) {
        for (j = 0; j < proptbl[i].used; j++)
          utf8_write(proptbl[i].ranges[j], out);
    }
    /*
     * Pad to a 4-byte boundary.
     */
    for (i = idx; i < bytes; i++)
      putc(zero, out);

    fclose(out);

    /*****************************************************************
     *
     * Generate the case mapping data.
     *
     *****************************************************************/

    /*
     * Open the case.dat file.
     */
    sprintf(path, "%s/case.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    /*
     * Write the case mapping tables.
     */
    hdr[1] = upper_used + lower_used + title_used;
    casecnt[0] = upper_used;
    casecnt[1] = lower_used;

    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write the upper and lower case table sizes.
     */
    fwrite((char *) casecnt, sizeof(unsigned short), 2, out);

    if (upper_used > 0)
      /*
       * Write the upper case table.
       */
      fwrite((char *) upper, sizeof(_case_t), upper_used, out);

    if (lower_used > 0)
      /*
       * Write the lower case table.
       */
      fwrite((char *) lower, sizeof(_case_t), lower_used, out);

    if (title_used > 0)
      /*
       * Write the title case table.
       */
      fwrite((char *) title, sizeof(_case_t), title_used, out);

    fclose(out);

    /*****************************************************************
     *
     * Generate the composition data.
     *
     *****************************************************************/
    
    /*
     * Create compositions from decomposition data
     */
    create_comps();
    
    /*
     * Open the comp.dat file.
     */
    sprintf(path, "%s/comp.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
	return;
    
    /*
     * Write the header.
     */
    hdr[1] = (unsigned short) comps_used * 4;
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);
    
    /*
     * Write out the byte count to maintain header size.
     */
    bytes = comps_used * sizeof(_comp_t);
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);
    
    /*
     * Now, if comps exist, write them out.
     */
    if (comps_used > 0)
        fwrite((char *) comps, sizeof(_comp_t), comps_used, out);
    
    fclose(out);
    
    /*****************************************************************
     *
     * Generate the decomposition data.
     *
     *****************************************************************/

    /*
     * Fully expand all decompositions before generating the output file.
     */
    expand_decomp();

    /*
     * Open the decomp.dat file.
     */
    sprintf(path, "%s/decomp.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    hdr[1] = decomps_used;

    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write a temporary byte count which will be calculated as the
     * decompositions are written out.
     */
    bytes = 0;
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    if (decomps_used) {
        /*
         * Write the list of decomp nodes.
         */
        for (i = idx = 0; i < decomps_used; i++) {
            fwrite((char *) &decomps[i].code, sizeof(unsigned long), 1, out);
            fwrite((char *) &idx, sizeof(unsigned long), 1, out);
            idx += decomps[i].used;
        }

        /*
         * Write the sentinel index as the last decomp node.
         */
        fwrite((char *) &idx, sizeof(unsigned long), 1, out);

        /*
         * Write the decompositions themselves.
         */
        for (i = 0; i < decomps_used; i++)
          fwrite((char *) decomps[i].decomp, sizeof(unsigned long),
                 decomps[i].used, out);

        /*
         * Seek back to the beginning and write the byte count.
         */
        bytes = (sizeof(unsigned long) * idx) +
            (sizeof(unsigned long) * ((hdr[1] << 1) + 1));
        fseek(out, sizeof(unsigned short) << 1, 0L);
        fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

        fclose(out);
    }

    /*****************************************************************
     *
     * Generate the combining class data.
     *
     *****************************************************************/

    /*
     * Open the cmbcl.dat file.
     */
    sprintf(path, "%s/cmbcl.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    /*
     * Set the number of ranges used.  Each range has a combining class which
     * means each entry is a 3-tuple.
     */
    hdr[1] = ccl_used / 3;

    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write out the byte count to maintain header size.
     */
    bytes = ccl_used * sizeof(unsigned long);
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    if (ccl_used > 0)
      /*
       * Write the combining class ranges out.
       */
      fwrite((char *) ccl, sizeof(unsigned long), ccl_used, out);

    fclose(out);

    /*****************************************************************
     *
     * Generate the number data.
     *
     *****************************************************************/

    /*
     * Open the num.dat file.
     */
    sprintf(path, "%s/num.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    /*
     * The count part of the header will be the total number of codes that
     * have numbers.
     */
    hdr[1] = (unsigned short) (ncodes_used << 1);
    bytes = (ncodes_used * sizeof(_codeidx_t)) + (nums_used * sizeof(_num_t));

    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write out the byte count to maintain header size.
     */
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    /*
     * Now, if number mappings exist, write them out.
     */
    if (ncodes_used > 0) {
        fwrite((char *) ncodes, sizeof(_codeidx_t), ncodes_used, out);
        fwrite((char *) nums, sizeof(_num_t), nums_used, out);
    }

    fclose(out);

    /*****************************************************************
     *
     * Generate the special UTF-8 title&upper->lower case lookup table.
     *
     *****************************************************************/

    /*
     * The trie must be compacted before writing it out.
     */
    trie_compact(trie[0].kids);

    /*
     * Next, make sure the table containing the lower case mappings is
     * adjusted to a 4-byte boundry. Some systems need this.
     */
    bytes = ltable_used + (4 - (ltable_used & 3));
    if (bytes > ltable_size) {
        /*
         * Unlikely, but just in case.
         */
        ltable = (unsigned char *) realloc((char *) ltable, bytes);
        ltable_size = ltable_used = (unsigned short) bytes;
    }

    /*
     * Make sure the lookup trie is also on a 4-byte boundary.
     */
    bytes = (ltrie_used << 1) + (4 - ((ltrie_used << 1) & 3));
    if (bytes > ltrie_size << 1) {
        ltrie = (unsigned short *) realloc((char *) ltrie, bytes);
        ltrie_size = ltrie_used = bytes >> 1;
    }

    /*
     * Open the casecmp.dat file.
     */
    sprintf(path, "%s/casecmp.dat", opath);
    if ((out = fopen(path, "wb")) == 0)
      return;

    /*
     * The header field holds the number of nodes used by the lookup trie.
     */
    hdr[1] = ltrie_used;
    bytes = (ltrie_used << 1) + ltable_used;

    /*
     * Write the header.
     */
    fwrite((char *) hdr, sizeof(unsigned short), 2, out);

    /*
     * Write out the total byte count to maintain header size.
     */
    fwrite((char *) &bytes, sizeof(unsigned long), 1, out);

    /*
     * Write out the lookup trie.
     */
    if (ltrie_used > 0)
      fwrite((char *) ltrie, sizeof(unsigned short), ltrie_used, out);

    /*
     * Write out the lower case mappings.
     */
    if (ltable_used > 0)
      fwrite((char *) ltable, sizeof(unsigned char), ltable_used, out);

    fclose(out);
}

static void
#ifdef __STDC__
free_cdata(void)
#else
free_cdata()
#endif
{
    int i;

    /*
     * Free everything up just to be tidy.
     */
    for (i = 0; i < NUMPROPS; i++) {
        if (proptbl[i].size > 0)
          free(proptbl[i].ranges);
    }
    for (i = 0; i < decomps_used; i++) {
        if (decomps[i].size > 0)
          free(decomps[i].decomp);
    }
    if (decomps_size > 0)
      free(decomps);
    if (comps_size > 0)
      free(comps);
    if (upper_size > 0)
      free(upper);
    if (lower_size > 0)
      free(lower);
    if (title_size > 0)
      free(title);
    if (trie_size > 0)
      free(trie);
    if (ltable_size > 0)
      free(ltable);
    if (ltrie_size > 0)
      free(ltrie);
    if (ccl_size > 0)
      free(ccl);
    if (ncodes_size > 0)
      free(ncodes);
    if (nums_size > 0)
      free(nums);
}

static void
#ifdef __STDC__
usage(char *prog)
#else
usage(prog)
char *prog;
#endif
{
    fprintf(stderr,
            "Usage: %s [-o output-directory|-x composition-exclusions]", prog);
    fprintf(stderr, " datafile1 datafile2 ...\n\n");
    fprintf(stderr,
            "-o output-directory\n\t\tWrite the output files to a different");
    fprintf(stderr, " directory (default: .).\n");
    fprintf(stderr,
            "-x composition-exclusion\n\t\tFile of composition codes");
    fprintf(stderr, " that should be excluded.\n");
    exit(1);
}

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif
{
    FILE *in;
    char *prog, *opath;
    int files = 0, len;

    if ((prog = strrchr(argv[0], '/')) != 0)
      prog++;
    else
      prog = argv[0];

    opath = 0;
    in = stdin;

    argc--;
    argv++;

    while (argc > 0) {
        if (argv[0][0] == '-') {
            switch (argv[0][1]) {
              case 'o':
                argc--;
                argv++;
                opath = argv[0];
                /*
                 * Limit the path size to avoid buffer overflow exploits in
                 * the sprintf() statements used when writing out the data
                 * files.
                 */
                if ((len = strlen(opath)) + 20 >= BUFSIZ) {
                    fprintf(stderr,
                            "%s: path too long (%d) - max length is %d. %s\n",
                            prog, len, BUFSIZ - 21,
                            "Saving in current directory.");
                    opath = 0;
                }
                break;
              case 'x':
                argc--;
                argv++;
                if ((in = fopen(argv[0], "rb")) == 0)
                  fprintf(stderr,
                          "%s: unable to open composition exclusion file %s\n",
                          prog, argv[0]);
                else {
                    read_compexdata(in);
                    fclose(in);
                    in = 0;
                }
                break;
              default:
                usage(prog);
            }
        } else {
            if (in != stdin && in != 0)
              fclose(in);
            if ((in = fopen(argv[0], "rb")) == 0)
              fprintf(stderr, "%s: unable to open ctype file %s\n",
                      prog, argv[0]);
            else {
                read_cdata(in);
                fclose(in);
                in = 0;
	    }
            files++;
        }
        argc--;
        argv++;
    }

    if (files > 0) {
        if (opath == 0)
          opath = ".";
        write_cdata(opath);
        free_cdata();
    } else
      fprintf(stderr, "%s: no input files provided on command line.\n", prog);

    return 0;
}
