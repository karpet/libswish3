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
static char rcsid[] __attribute__ ((unused)) = "$Id: ucpgba.c,v 1.22 2005/03/25 21:26:52 mleisher Exp $";
#else
static char rcsid[] = "$Id: ucpgba.c,v 1.22 2005/03/25 21:26:52 mleisher Exp $";
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include "ucdata.h"
#include "ucpgba.h"

#define ISRTL_NEUTRAL(cc) ucisprop(cc, UC_R, UC_B|UC_S|UC_WS|UC_AL)
#define ISRTL_NEUTRAL_AL(cc) ucisprop(cc, UC_R|UC_ES,\
                                      UC_B|UC_S|UC_WS|UC_AL|UC_CS|UC_ON)

#define ISWEAKSPECIAL(cc) ucisprop(cc, UC_EN|UC_ES|UC_MN, UC_ET|UC_AN|UC_ON)
#define ISWEAKSPECIAL_AL(cc) ucisprop(cc, UC_EN|UC_ND, UC_AN)

/*
 * This table is temporarily hard-coded here until it can be constructed
 * from the Unicode data files somehow.  The current table is from
 * the forthcoming Unicode 4.1.0.
 */
static unsigned long _symmetric_pairs[] = {
    0x0028, 0x0029,  0x0029, 0x0028,  0x003C, 0x003E,  0x003E, 0x003C, 
    0x005B, 0x005D,  0x005D, 0x005B,  0x007B, 0x007D,  0x007D, 0x007B, 
    0x00AB, 0x00BB,  0x00BB, 0x00AB,  0x2039, 0x203A,  0x203A, 0x2039, 
    0x2045, 0x2046,  0x2046, 0x2045,  0x207D, 0x207E,  0x207E, 0x207D, 
    0x208D, 0x208E,  0x208E, 0x208D,  0x2208, 0x220B,  0x2209, 0x220C, 
    0x220A, 0x220D,  0x220B, 0x2208,  0x220C, 0x2209,  0x220D, 0x220A, 
    0x2215, 0x29F5,  0x223C, 0x223D,  0x223D, 0x223C,  0x2243, 0x22CD, 
    0x2252, 0x2253,  0x2253, 0x2252,  0x2254, 0x2255,  0x2255, 0x2254, 
    0x2264, 0x2265,  0x2265, 0x2264,  0x2266, 0x2267,  0x2267, 0x2266, 
    0x2268, 0x2269,  0x2269, 0x2268,  0x226A, 0x226B,  0x226B, 0x226A, 
    0x226E, 0x226F,  0x226F, 0x226E,  0x2270, 0x2271,  0x2271, 0x2270, 
    0x2272, 0x2273,  0x2273, 0x2272,  0x2274, 0x2275,  0x2275, 0x2274, 
    0x2276, 0x2277,  0x2277, 0x2276,  0x2278, 0x2279,  0x2279, 0x2278, 
    0x227A, 0x227B,  0x227B, 0x227A,  0x227C, 0x227D,  0x227D, 0x227C, 
    0x227E, 0x227F,  0x227F, 0x227E,  0x2280, 0x2281,  0x2281, 0x2280, 
    0x2282, 0x2283,  0x2283, 0x2282,  0x2284, 0x2285,  0x2285, 0x2284, 
    0x2286, 0x2287,  0x2287, 0x2286,  0x2288, 0x2289,  0x2289, 0x2288, 
    0x228A, 0x228B,  0x228B, 0x228A,  0x228F, 0x2290,  0x2290, 0x228F, 
    0x2291, 0x2292,  0x2292, 0x2291,  0x2298, 0x29B8,  0x22A2, 0x22A3, 
    0x22A3, 0x22A2,  0x22A6, 0x2ADE,  0x22A8, 0x2AE4,  0x22A9, 0x2AE3, 
    0x22AB, 0x2AE5,  0x22B0, 0x22B1,  0x22B1, 0x22B0,  0x22B2, 0x22B3, 
    0x22B3, 0x22B2,  0x22B4, 0x22B5,  0x22B5, 0x22B4,  0x22B6, 0x22B7, 
    0x22B7, 0x22B6,  0x22C9, 0x22CA,  0x22CA, 0x22C9,  0x22CB, 0x22CC, 
    0x22CC, 0x22CB,  0x22CD, 0x2243,  0x22D0, 0x22D1,  0x22D1, 0x22D0, 
    0x22D6, 0x22D7,  0x22D7, 0x22D6,  0x22D8, 0x22D9,  0x22D9, 0x22D8, 
    0x22DA, 0x22DB,  0x22DB, 0x22DA,  0x22DC, 0x22DD,  0x22DD, 0x22DC, 
    0x22DE, 0x22DF,  0x22DF, 0x22DE,  0x22E0, 0x22E1,  0x22E1, 0x22E0, 
    0x22E2, 0x22E3,  0x22E3, 0x22E2,  0x22E4, 0x22E5,  0x22E5, 0x22E4, 
    0x22E6, 0x22E7,  0x22E7, 0x22E6,  0x22E8, 0x22E9,  0x22E9, 0x22E8, 
    0x22EA, 0x22EB,  0x22EB, 0x22EA,  0x22EC, 0x22ED,  0x22ED, 0x22EC, 
    0x22F0, 0x22F1,  0x22F1, 0x22F0,  0x22F2, 0x22FA,  0x22F3, 0x22FB, 
    0x22F4, 0x22FC,  0x22F6, 0x22FD,  0x22F7, 0x22FE,  0x22FA, 0x22F2, 
    0x22FB, 0x22F3,  0x22FC, 0x22F4,  0x22FD, 0x22F6,  0x22FE, 0x22F7, 
    0x2308, 0x2309,  0x2309, 0x2308,  0x230A, 0x230B,  0x230B, 0x230A, 
    0x2329, 0x232A,  0x232A, 0x2329,  0x2768, 0x2769,  0x2769, 0x2768, 
    0x276A, 0x276B,  0x276B, 0x276A,  0x276C, 0x276D,  0x276D, 0x276C, 
    0x276E, 0x276F,  0x276F, 0x276E,  0x2770, 0x2771,  0x2771, 0x2770, 
    0x2772, 0x2773,  0x2773, 0x2772,  0x2774, 0x2775,  0x2775, 0x2774, 
    0x27C3, 0x27C4,  0x27C4, 0x27C3,  0x27C5, 0x27C6,  0x27C6, 0x27C5, 
    0x27D5, 0x27D6,  0x27D6, 0x27D5,  0x27DD, 0x27DE,  0x27DE, 0x27DD, 
    0x27E2, 0x27E3,  0x27E3, 0x27E2,  0x27E4, 0x27E5,  0x27E5, 0x27E4, 
    0x27E6, 0x27E7,  0x27E7, 0x27E6,  0x27E8, 0x27E9,  0x27E9, 0x27E8, 
    0x27EA, 0x27EB,  0x27EB, 0x27EA,  0x2983, 0x2984,  0x2984, 0x2983, 
    0x2985, 0x2986,  0x2986, 0x2985,  0x2987, 0x2988,  0x2988, 0x2987, 
    0x2989, 0x298A,  0x298A, 0x2989,  0x298B, 0x298C,  0x298C, 0x298B, 
    0x298D, 0x2990,  0x298E, 0x298F,  0x298F, 0x298E,  0x2990, 0x298D, 
    0x2991, 0x2992,  0x2992, 0x2991,  0x2993, 0x2994,  0x2994, 0x2993, 
    0x2995, 0x2996,  0x2996, 0x2995,  0x2997, 0x2998,  0x2998, 0x2997, 
    0x29B8, 0x2298,  0x29C0, 0x29C1,  0x29C1, 0x29C0,  0x29C4, 0x29C5, 
    0x29C5, 0x29C4,  0x29CF, 0x29D0,  0x29D0, 0x29CF,  0x29D1, 0x29D2, 
    0x29D2, 0x29D1,  0x29D4, 0x29D5,  0x29D5, 0x29D4,  0x29D8, 0x29D9, 
    0x29D9, 0x29D8,  0x29DA, 0x29DB,  0x29DB, 0x29DA,  0x29F5, 0x2215, 
    0x29F8, 0x29F9,  0x29F9, 0x29F8,  0x29FC, 0x29FD,  0x29FD, 0x29FC, 
    0x2A2B, 0x2A2C,  0x2A2C, 0x2A2B,  0x2A2D, 0x2A2E,  0x2A2E, 0x2A2D, 
    0x2A34, 0x2A35,  0x2A35, 0x2A34,  0x2A3C, 0x2A3D,  0x2A3D, 0x2A3C, 
    0x2A64, 0x2A65,  0x2A65, 0x2A64,  0x2A79, 0x2A7A,  0x2A7A, 0x2A79, 
    0x2A7D, 0x2A7E,  0x2A7E, 0x2A7D,  0x2A7F, 0x2A80,  0x2A80, 0x2A7F, 
    0x2A81, 0x2A82,  0x2A82, 0x2A81,  0x2A83, 0x2A84,  0x2A84, 0x2A83, 
    0x2A8B, 0x2A8C,  0x2A8C, 0x2A8B,  0x2A91, 0x2A92,  0x2A92, 0x2A91, 
    0x2A93, 0x2A94,  0x2A94, 0x2A93,  0x2A95, 0x2A96,  0x2A96, 0x2A95, 
    0x2A97, 0x2A98,  0x2A98, 0x2A97,  0x2A99, 0x2A9A,  0x2A9A, 0x2A99, 
    0x2A9B, 0x2A9C,  0x2A9C, 0x2A9B,  0x2AA1, 0x2AA2,  0x2AA2, 0x2AA1, 
    0x2AA6, 0x2AA7,  0x2AA7, 0x2AA6,  0x2AA8, 0x2AA9,  0x2AA9, 0x2AA8, 
    0x2AAA, 0x2AAB,  0x2AAB, 0x2AAA,  0x2AAC, 0x2AAD,  0x2AAD, 0x2AAC, 
    0x2AAF, 0x2AB0,  0x2AB0, 0x2AAF,  0x2AB3, 0x2AB4,  0x2AB4, 0x2AB3, 
    0x2ABB, 0x2ABC,  0x2ABC, 0x2ABB,  0x2ABD, 0x2ABE,  0x2ABE, 0x2ABD, 
    0x2ABF, 0x2AC0,  0x2AC0, 0x2ABF,  0x2AC1, 0x2AC2,  0x2AC2, 0x2AC1, 
    0x2AC3, 0x2AC4,  0x2AC4, 0x2AC3,  0x2AC5, 0x2AC6,  0x2AC6, 0x2AC5, 
    0x2ACD, 0x2ACE,  0x2ACE, 0x2ACD,  0x2ACF, 0x2AD0,  0x2AD0, 0x2ACF, 
    0x2AD1, 0x2AD2,  0x2AD2, 0x2AD1,  0x2AD3, 0x2AD4,  0x2AD4, 0x2AD3, 
    0x2AD5, 0x2AD6,  0x2AD6, 0x2AD5,  0x2ADE, 0x22A6,  0x2AE3, 0x22A9, 
    0x2AE4, 0x22A8,  0x2AE5, 0x22AB,  0x2AEC, 0x2AED,  0x2AED, 0x2AEC, 
    0x2AF7, 0x2AF8,  0x2AF8, 0x2AF7,  0x2AF9, 0x2AFA,  0x2AFA, 0x2AF9, 
    0x2E02, 0x2E03,  0x2E03, 0x2E02,  0x2E04, 0x2E05,  0x2E05, 0x2E04, 
    0x2E09, 0x2E0A,  0x2E0A, 0x2E09,  0x2E0C, 0x2E0D,  0x2E0D, 0x2E0C, 
    0x2E1C, 0x2E1D,  0x2E1D, 0x2E1C,  0x3008, 0x3009,  0x3009, 0x3008, 
    0x300A, 0x300B,  0x300B, 0x300A,  0x300C, 0x300D,  0x300D, 0x300C, 
    0x300E, 0x300F,  0x300F, 0x300E,  0x3010, 0x3011,  0x3011, 0x3010, 
    0x3014, 0x3015,  0x3015, 0x3014,  0x3016, 0x3017,  0x3017, 0x3016, 
    0x3018, 0x3019,  0x3019, 0x3018,  0x301A, 0x301B,  0x301B, 0x301A, 
    0xFF08, 0xFF09,  0xFF09, 0xFF08,  0xFF1C, 0xFF1E,  0xFF1E, 0xFF1C, 
    0xFF3B, 0xFF3D,  0xFF3D, 0xFF3B,  0xFF5B, 0xFF5D,  0xFF5D, 0xFF5B, 
    0xFF5F, 0xFF60,  0xFF60, 0xFF5F,  0xFF62, 0xFF63,  0xFF63, 0xFF62, 
};

static int _symmetric_pairs_size =
sizeof(_symmetric_pairs)/sizeof(_symmetric_pairs[0]);

/*
 * This routine looks up the other form of a symmetric pair.
 */
static unsigned long
#ifdef __STDC__
_ucsymmetric_pair(unsigned long c)
#else
_ucsymmetric_pair(c)
unsigned long c;
#endif
{
    int i;

    for (i = 0; i < _symmetric_pairs_size; i += 2) {
        if (_symmetric_pairs[i] == c)
          return _symmetric_pairs[i+1];
    }
    return c;
}

/*
 * This routine creates a new run, copies the text into it, links it into the
 * logical text order chain and returns it to the caller to be linked into
 * the visual text order chain.
 */
static ucrun_t *
#ifdef __STDC__
_add_run(ucstring_t *str, unsigned long *src,
         unsigned long start, unsigned long end, int direction,
         int parent_rtl)
#else
_add_run(str, src, start, end, direction, parent_direction)
ucstring_t *str;
unsigned long *src, start, end;
int direction, parent_rtl;
#endif
{
    long i, t;
    ucrun_t *run;

    run = (ucrun_t *) malloc(sizeof(ucrun_t));
    run->logical_prev = run->logical_next =
        run->visual_next = run->visual_prev = 0;
    run->direction = direction;

    run->cursor = ~0;

    run->chars = (unsigned long *)
        malloc(sizeof(unsigned long) * ((end - start) << 1));
    run->positions = run->chars + (end - start);

    run->source = src;
    run->start = start;
    run->end = end;

    if (direction == UCPGBA_RTL) {
        /*
         * Copy the source text into the run in reverse order and select
         * replacements for the pairwise punctuation and the <> characters.
         */
        for (i = 0, t = end - 1; start < end; start++, t--, i++) {
            run->positions[i] = t;
            /*
             * Remove the call to ucsymmetric() in case the LocalUCData.txt
             * file was not compiled.
             */
            run->chars[i] = _ucsymmetric_pair(src[t]);
        }
    } else {
        /*
         * Copy the source text into the run directly.
         */
        for (i = start; i < end; i++) {
            run->positions[i - start] = i;
            run->chars[i - start] = src[i];
        }
    }

    /*
     * Add the run to the logical list for cursor traversal.
     */
    if (str->logical_first == 0)
      str->logical_first = str->logical_last = run;
    else {
        run->logical_prev = str->logical_last;
        str->logical_last->logical_next = run;
        str->logical_last = run;
    }

    return run;
}

static void
#ifdef __STDC__
_ucadd_rtl_segment(ucstring_t *str, unsigned long *source, unsigned long start,
                   unsigned long end, int have_al)
#else
_ucadd_rtl_segment(str, source, start, end, have_al)
ucstring_t *str;
unsigned long *source, start, end;
int have_al;
#endif
{
    unsigned long s, e;
    ucrun_t *run, *lrun;

    /*
     * This is used to splice runs into strings with overall LTR direction.
     * The `lrun' variable will never be NULL because at least one LTR run was
     * added before this RTL run.
     */
    lrun = str->visual_last;

    for (e = s = start; s < end;) {
        if (have_al)
          for (; e < end && ISRTL_NEUTRAL_AL(source[e]); e++) ;
        else
          for (; e < end && ISRTL_NEUTRAL(source[e]); e++) ;

        if (e > s) {
            run = _add_run(str, source, s, e, UCPGBA_RTL, 1);

            /*
             * Add the run to the visual list for cursor traversal.
             */
            if (str->visual_first != 0) {
                if (str->direction == UCPGBA_LTR) {
                    run->visual_prev = lrun;
                    run->visual_next = lrun->visual_next;
                    if (lrun->visual_next != 0)
                      lrun->visual_next->visual_prev = run;
                    lrun->visual_next = run;
                    if (lrun == str->visual_last)
                      str->visual_last = run;
                } else {
                    run->visual_next = str->visual_first;
                    str->visual_first->visual_prev = run;
                    str->visual_first = run;
                }
            } else
              str->visual_first = str->visual_last = run;
        }

        /*
         * Collect digits alone or with weakly directional characters
         * depending on whether we have Arabic Letters present.
         */
        s = e;
        while (1) {
            if (have_al)
              for (; e < end && ISWEAKSPECIAL_AL(source[e]); e++);
            else
              for (; e < end && ISWEAKSPECIAL(source[e]); e++);

            if (e == s || e == end)
              break;
            if (ucisprop(source[e], 0, UC_CS) &&
                ucisdigit(source[e-1]) && ucisdigit(source[e+1]))
              e++;
            else
              break;
        }

        if (e > s) {
            run = _add_run(str, source, s, e, UCPGBA_LTR, 1);

            /*
             * Add the run to the visual list for cursor traversal.
             */
            if (str->visual_first != 0) {
                if (str->direction == UCPGBA_LTR) {
                    run->visual_prev = lrun;
                    run->visual_next = lrun->visual_next;
                    if (lrun->visual_next != 0)
                      lrun->visual_next->visual_prev = run;
                    lrun->visual_next = run;
                    if (lrun == str->visual_last)
                      str->visual_last = run;
                } else {
                    run->visual_next = str->visual_first;
                    str->visual_first->visual_prev = run;
                    str->visual_first = run;
                }
            } else
              str->visual_first = str->visual_last = run;
        }

        /*
         * Collect all weak and neutral non-digit sequences for an RTL
         * segment.  These will appear as part of the next RTL segment or
         * will be added as an RTL segment by themselves.
         */
        for (s = e; e < end && ucisweak(source[e]) && !ucisdigit(source[e]);
             e++) ;
    }

    /*
     * Capture any weak non-digit sequences that occur at the end of the RTL
     * run.
     */
    if (e > s) {
        run = _add_run(str, source, s, e, UCPGBA_RTL, 1);

        /*
         * Add the run to the visual list for cursor traversal.
         */
        if (str->visual_first != 0) {
            if (str->direction == UCPGBA_LTR) {
                run->visual_prev = lrun;
                run->visual_next = lrun->visual_next;
                if (lrun->visual_next != 0)
                  lrun->visual_next->visual_prev = run;
                lrun->visual_next = run;
                if (lrun == str->visual_last)
                  str->visual_last = run;
            } else {
                run->visual_next = str->visual_first;
                str->visual_first->visual_prev = run;
                str->visual_first = run;
            }
        } else
          str->visual_first = str->visual_last = run;
    }
}

static void
#ifdef __STDC__
_ucadd_ltr_segment(ucstring_t *str, unsigned long *source, unsigned long start,
                   unsigned long end)
#else
_ucadd_ltr_segment(str, source, start, end)
ucstring_t *str;
unsigned long *source, start, end;
#endif
{
    ucrun_t *run;

    run = _add_run(str, source, start, end, UCPGBA_LTR, 0);

    /*
     * Add the run to the visual list for cursor traversal.
     */
    if (str->visual_first != 0) {
        if (str->direction == UCPGBA_LTR) {
            run->visual_prev = str->visual_last;
            str->visual_last->visual_next = run;
            str->visual_last = run;
        } else {
            run->visual_next = str->visual_first;
            str->visual_first->visual_prev = run;
            str->visual_first = run;
        }
    } else
      str->visual_first = str->visual_last = run;
}

#define UC_LTR_PROPS1 (UC_L|UC_MN|UC_EN|UC_ES|UC_PE)
#define UC_LTR_PROPS2 (UC_ET|UC_CS|UC_B|UC_S|UC_WS|UC_ON)

#define UC_RTL_PROPS1 (UC_R|UC_MN|UC_EN|UC_ES|UC_PE)
#define UC_RTL_PROPS2 (UC_ET|UC_CS|UC_B|UC_S|UC_WS|UC_ON|UC_AN|UC_AL)

ucstring_t *
#ifdef __STDC__
ucstring_create(unsigned long *source, unsigned long start, unsigned long end,
                int default_direction, int cursor_motion)
#else
ucstring_create(source, start, end, default_direction, cursor_motion)
unsigned long *source, start, end;
int default_direction, cursor_motion;
#endif
{
    int rtl_first, have_al;
    unsigned long s, e, re, m1 = 0, m2 = 0;
    ucstring_t *str;

    str = (ucstring_t *) malloc(sizeof(ucstring_t));

    /*
     * Set the initial values.
     */
    str->cursor_motion = cursor_motion;
    str->logical_first = str->logical_last = 0;
    str->visual_first = str->visual_last = str->cursor = 0;
    str->source = source;
    str->start = start;
    str->end = end;

    /*
     * If the length of the string is 0, then just return it at this point.
     */
    if (start == end)
      return str;

    /*
     * This flag indicates whether the collection loop for RTL is called
     * before the LTR loop the first time.
     */
    rtl_first = 0;

    /*
     * Look for the first character in the string that has strong
     * directionality.
     */
    for (s = start; s < end && !ucisstrong(source[s]); s++) ;

    if (s == end)
      /*
       * If the string contains no characters with strong directionality, use
       * the default direction.
       */
      str->direction = default_direction;
    else
      str->direction = ucisrtl(source[s]) ? UCPGBA_RTL : UCPGBA_LTR;

    if (str->direction == UCPGBA_RTL)
      /*
       * Set the flag that causes the RTL collection loop to run first.
       */
      rtl_first = 1;

    /*
     * This loop now separates the string into runs based on directionality.
     */
    for (s = e = 0, re = ~0; s < end; s = e) {
        /*
         * Flag that indicates the presence of Arabic Letter (AL) characters.
         */
        have_al = 0;

        if (!rtl_first) {
            /*
             * Determine the next run of LTR text.
             */

            re = ~0;
            while (e < end &&
                   ucprops(source[e], UC_LTR_PROPS1, UC_LTR_PROPS2,
                           &m1, &m2)) {
                /*
                 * Set a run endpoint so backtracking is not necessary to
                 * determine the real end of the run.
                 */
                if ((m1 & (UC_L|UC_MN|UC_EN|UC_PE)) || (m2 & UC_ET))
                  re = e;
                e++;
            }
            if (str->direction != UCPGBA_LTR && re != ~0)
              e = re + 1;

            /*
             * Add the LTR segment to the string.
             */
            if (e > s)
              _ucadd_ltr_segment(str, source, s, e);
        }

        /*
         * Determine the next run of RTL text.
         */
        re = ~0;
        s = e;
        while (e < end &&
               ucprops(source[e], UC_RTL_PROPS1, UC_RTL_PROPS2, &m1, &m2)) {
            if (!have_al && (m2 & (UC_AL|UC_AN)))
              /*
               * If an Arabic Letter or Number is encountered, set the flag so
               * expressions containing numbers will be handled properly.
               */
              have_al = 1;
            else if (have_al && (m1 & UC_R))
              /*
               * If some non-Arabic RTL character is encountered, clear
               * the flag so expressions containing numbers
               * will be handled properly.
               */
              have_al = 0;

            /*
             * Set a run endpoint so backtracking is not necessary to
             * determine the real end of the run.
             */
            if ((m1 & (UC_R|UC_MN|UC_EN|UC_PE)) || (m2 & (UC_ET|UC_AN|UC_AL)))
              re = e;

            e++;
        }
        if (str->direction != UCPGBA_RTL && re != ~0)
          e = re + 1;

        /*
         * Add the RTL segment to the string.
         */
        if (e > s)
          _ucadd_rtl_segment(str, source, s, e, have_al);

        /*
         * Clear the flag that allowed the RTL collection loop to run first
         * for strings with overall RTL directionality.
         */
        rtl_first = 0;
    }

    /*
     * Set up the initial cursor run.
     */
    str->cursor = str->logical_first;
    if (str != 0)
      str->cursor->cursor = (str->cursor->direction == UCPGBA_RTL) ?
          str->cursor->end - str->cursor->start : 0;

    return str;
}

void
#ifdef __STDC__
ucstring_free(ucstring_t *s)
#else
ucstring_free(s)
ucstring_t *s;
#endif
{
    ucrun_t *l, *r;

    if (s == 0)
      return;

    for (l = 0, r = s->visual_first; r != 0; r = r->visual_next) {
        if (r->end > r->start)
          free((char *) r->chars);
        if (l)
          free((char *) l);
        l = r;
    }
    if (l)
      free((char *) l);

    free((char *) s);
}

int
#ifdef __STDC__
ucstring_set_cursor_motion(ucstring_t *str, int cursor_motion)
#else
ucstring_set_cursor_motion(s, cursor_motion)
ucstring_t *str;
int cursor_motion;
#endif
{
    int n;

    if (str == 0)
      return -1;

    n = str->cursor_motion;
    str->cursor_motion = cursor_motion;
    return n;
}

static int
#ifdef __STDC__
_ucstring_visual_cursor_right(ucstring_t *str, int count)
#else
_ucstring_visual_cursor_right(str, count)
ucstring_t *str;
int count;
#endif
{
    int cnt = count;
    ucrun_t *cursor;

    if (str == 0)
      return 0;

    cursor = str->cursor;
    while (cnt > 0) {
        if (cursor->cursor == cursor->end) {
            if (cursor->visual_next == 0)
              return (cnt != count);
            str->cursor = cursor = cursor->visual_next;
            cursor->cursor = cursor->start;
        }
        cursor->cursor++;
        cnt--;
    }
    return 1;
}

static int
#ifdef __STDC__
_ucstring_logical_cursor_right(ucstring_t *str, int count)
#else
_ucstring_logical_cursor_right(str, count)
ucstring_t *str;
int count;
#endif
{
    int cnt = count;
    ucrun_t *cursor;

    if (str == 0)
      return 0;

    cursor = str->cursor;
    while (cnt > 0) {
        if (cursor->direction == UCPGBA_RTL) {
            if (cursor->cursor == cursor->start) {
                if (cursor->logical_next == 0)
                  return (cnt != count);
                str->cursor = cursor = cursor->logical_next;
                cursor->cursor = (cursor->direction == UCPGBA_LTR) ?
                    cursor->start : cursor->end;
            }
            if (cursor->direction == UCPGBA_LTR)
              cursor->cursor++;
            else
              cursor->cursor--;
        } else {
            if (cursor->cursor == cursor->end) {
                if (cursor->logical_next == 0)
                  return (cnt != count);
                str->cursor = cursor = cursor->logical_next;
                cursor->cursor = (cursor->direction == UCPGBA_LTR) ?
                    cursor->start : cursor->end;
            }
            if (cursor->direction == UCPGBA_LTR)
              cursor->cursor++;
            else
              cursor->cursor--;
        }
        cnt--;
    }
    return 1;
}

int
#ifdef __STDC__
ucstring_cursor_right(ucstring_t *str, int count)
#else
ucstring_cursor_right(str, count)
ucstring_t *str;
int count;
#endif
{
    if (str == 0)
      return 0;
    return (str->cursor_motion == UCPGBA_CURSOR_VISUAL) ?
        _ucstring_visual_cursor_right(str, count) :
        _ucstring_logical_cursor_right(str, count);
}

static int
#ifdef __STDC__
_ucstring_visual_cursor_left(ucstring_t *str, int count)
#else
_ucstring_visual_cursor_left(str, count)
ucstring_t *str;
int count;
#endif
{
    int cnt = count;
    ucrun_t *cursor;

    if (str == 0)
      return 0;

    cursor = str->cursor;
    while (cnt > 0) {
        if (cursor->cursor == cursor->start) {
            if (cursor->visual_prev == 0)
              return (cnt != count);

            str->cursor = cursor = cursor->visual_prev;
            cursor->cursor = cursor->end;
        }
        cursor->cursor--;
        cnt--;
    }
    return 1;
}

static int
#ifdef __STDC__
_ucstring_logical_cursor_left(ucstring_t *str, int count)
#else
_ucstring_logical_cursor_left(str, count)
ucstring_t *str;
int count;
#endif
{
    int cnt = count;
    ucrun_t *cursor;

    if (str == 0)
      return 0;

    cursor = str->cursor;
    while (cnt > 0) {
        if (cursor->direction == UCPGBA_RTL) {
            if (cursor->cursor == cursor->end) {
                if (cursor->logical_prev == 0)
                  return (cnt != count);
                str->cursor = cursor = cursor->logical_prev;
                cursor->cursor = (cursor->direction == UCPGBA_LTR) ?
                    cursor->end : cursor->start;
            }
            if (cursor->direction == UCPGBA_LTR)
              cursor->cursor--;
            else
              cursor->cursor++;
        } else {
            if (cursor->cursor == cursor->start) {
                if (cursor->logical_prev == 0)
                  return (cnt != count);
                str->cursor = cursor = cursor->logical_prev;
                cursor->cursor = (cursor->direction == UCPGBA_LTR) ?
                    cursor->end : cursor->start;
            }
            if (cursor->direction == UCPGBA_LTR)
              cursor->cursor--;
            else
              cursor->cursor++;
        }
        cnt--;
    }
    return 1;
}

int
#ifdef __STDC__
ucstring_cursor_left(ucstring_t *str, int count)
#else
ucstring_cursor_left(str, count)
ucstring_t *str;
int count;
#endif
{
    if (str == 0)
      return 0;
    return (str->cursor_motion == UCPGBA_CURSOR_VISUAL) ?
        _ucstring_visual_cursor_left(str, count) :
        _ucstring_logical_cursor_left(str, count);
}

void
#ifdef __STDC__
ucstring_cursor_info(ucstring_t *str, int *direction, unsigned long *position)
#else
ucstring_cursor_info(str, direction, position)
ucstring_t *str, int *direction;
unsigned long *position;
#endif
{
    long c;
    unsigned long size;
    ucrun_t *cursor;

    if (str == 0 || direction == 0 || position == 0)
      return;

    cursor = str->cursor;

    *direction = cursor->direction;

    c = cursor->cursor;
    size = cursor->end - cursor->start;

    if (c == size)
      *position = (cursor->direction == UCPGBA_RTL) ?
          cursor->start : cursor->positions[c - 1];
    else if (c == -1)
      *position = (cursor->direction == UCPGBA_RTL) ?
          cursor->end : cursor->start;
    else
      *position = cursor->positions[c];
}
