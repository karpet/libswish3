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
static char rcsid[] __attribute__ ((unused)) = "$Id: ucdata.c,v 1.23 2005/03/24 22:40:19 mleisher Exp $";
#else
static char rcsid[] = "$Id: ucdata.c,v 1.23 2005/03/24 22:40:19 mleisher Exp $";
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "ucdata.h"

/**************************************************************************
 *
 * Miscellaneous types, data, and support functions.
 *
 **************************************************************************/

typedef struct {
    unsigned short bom;
    unsigned short cnt;
    union {
        unsigned long bytes;
        unsigned short len[2];
    } size;
} _ucheader_t;

/*
 * A simple array of 32-bit masks for lookup.
 */
static unsigned long masks32[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020,
    0x00000040, 0x00000080, 0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000, 0x00010000, 0x00020000,
    0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000,
    0x40000000, 0x80000000
};

#define endian_short(cc) (((cc) >> 8) | (((cc) & 0xff) << 8))
#define endian_long(cc) ((((cc) & 0xff) << 24)|((((cc) >> 8) & 0xff) << 16)|\
                        ((((cc) >> 16) & 0xff) << 8)|((cc) >> 24))

/*
 * UTF-8 character lengths. Length determined by:
 *
 *   len = clp[((*ch ^ 0xf0) >> 4) - ((*ch / 0xf8) + (*ch / 0xfc))];
 */
static char clengths[] = {6, 5, 4, 3, 2, 2,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static char *clp = 0;

static FILE *
#ifdef __STDC__
_ucopenfile(char *paths, char *filename, char *mode)
#else
_ucopenfile(paths, filename, mode)
char *paths, *filename, *mode;
#endif
{
    FILE *f;
    char *fp, *dp, *pp, path[BUFSIZ];

    if (filename == 0 || *filename == 0)
      return 0;

    dp = paths;
    while (dp && *dp) {
        pp = path;
        while (*dp && *dp != ':')
          *pp++ = *dp++;
        *pp++ = '/';

        fp = filename;
        while (*fp)
          *pp++ = *fp++;
        *pp = 0;

        if ((f = fopen(path, mode)) != 0)
          return f;

        if (*dp == ':')
          dp++;
    }

    return 0;
}

/**************************************************************************
 *
 * Support for the character properties.
 *
 **************************************************************************/

static unsigned long  _ucprop_size;
static unsigned short *_ucprop_offsets;
static unsigned long  *_ucprop_ranges;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_ucprop_load(char *paths, int reload)
#else
_ucprop_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long size, i;
    _ucheader_t hdr;

    if (_ucprop_size > 0) {
        if (!reload)
          /*
           * The character properties have already been loaded.
           */
          return 0;

        /*
         * Unload the current character property data in preparation for
         * loading a new copy.  Only the first array has to be deallocated
         * because all the memory for the arrays is allocated as a single
         * block.
         */
        free((char *) _ucprop_offsets);
        _ucprop_size = 0;
    }

    if ((in = _ucopenfile(paths, "ctype.dat", "rb")) == 0)
      return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    if ((_ucprop_size = hdr.cnt) == 0) {
        fclose(in);
        return -1;
    }

    /*
     * Allocate all the storage needed for the lookup table.
     */
    _ucprop_offsets = (unsigned short *) malloc(hdr.size.bytes);

    /*
     * Calculate the offset into the storage for the ranges.  The offsets
     * array is on a 4-byte boundary and one larger than the value provided in
     * the header count field.  This means the offset to the ranges must be
     * calculated after aligning the count to a 4-byte boundary.
     */
    if ((size = ((hdr.cnt + 1) * sizeof(unsigned short))) & 3)
      size += 4 - (size & 3);
    size >>= 1;
    _ucprop_ranges = (unsigned long *) (_ucprop_offsets + size);

    /*
     * Load the offset array.
     */
    fread((char *) _ucprop_offsets, sizeof(unsigned short), size, in);

    /*
     * Do an endian swap if necessary.  Don't forget there is an extra node on
     * the end with the final index.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i <= _ucprop_size; i++)
          _ucprop_offsets[i] = endian_short(_ucprop_offsets[i]);
    }

    /*
     * Load the ranges.  The number of elements is in the last array position
     * of the offsets.
     */
    fread((char *) _ucprop_ranges, sizeof(unsigned long),
          _ucprop_offsets[_ucprop_size], in);

    fclose(in);

    /*
     * Do an endian swap if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < _ucprop_offsets[_ucprop_size]; i++)
          _ucprop_ranges[i] = endian_long(_ucprop_ranges[i]);
    }
    return 0;
}

static void
#ifdef __STDC__
_ucprop_unload(void)
#else
_ucprop_unload()
#endif
{
    if (_ucprop_size == 0)
      return;

    /*
     * Only need to free the offsets because the memory is allocated as a
     * single block.
     */
    free((char *) _ucprop_offsets);
    _ucprop_size = 0;
}

static int
#ifdef __STDC__
_ucprop_lookup(unsigned long code, unsigned long n)
#else
_ucprop_lookup(code, n)
unsigned long code, n;
#endif
{
    long l, r, m;

    /*
     * There is an extra node on the end of the offsets to allow this routine
     * to work right.  If the index is 0xffff, then there are no nodes for the
     * property.
     */
    if ((l = _ucprop_offsets[n]) == 0xffff)
      return 0;

    /*
     * Locate the next offset that is not 0xffff.  The sentinel at the end of
     * the array is the max index value.
     */
    for (m = 1;
         n + m < _ucprop_size && _ucprop_offsets[n + m] == 0xffff; m++) ;

    r = _ucprop_offsets[n + m] - 1;

    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a range pair.
         */
        m = (l + r) >> 1;
        m -= (m & 1);
        if (code > _ucprop_ranges[m + 1])
          l = m + 2;
        else if (code < _ucprop_ranges[m])
          r = m - 2;
        else if (code >= _ucprop_ranges[m] && code <= _ucprop_ranges[m + 1])
          return 1;
    }
    return 0;
}

int
#ifdef __STDC__
ucprops(unsigned long code, unsigned long mask1, unsigned long mask2,
        unsigned long *mask1_out, unsigned long *mask2_out)
#else
ucprops(code, mask1, mask2, mask1_out, mask2_out)
unsigned long code, mask1, mask2, *mask1_out, *mask2_out;
#endif
{
    int ret = 0;
    unsigned long i;

    if (mask1 == 0 && mask2 == 0)
      return ret;

    if (mask1_out)
      *mask1_out = 0;
    if (mask2_out)
      *mask2_out = 0;

    for (i = 0; mask1 && i < 32; i++) {
        if ((mask1 & masks32[i]) && _ucprop_lookup(code, i)) {
            *mask1_out |= 1 << i;
            ret = 1;
        }
    }

    for (i = 32; mask2 && i < _ucprop_size; i++) {
        if ((mask2 & masks32[i & 31]) && _ucprop_lookup(code, i)) {
            *mask2_out |= 1 << (i - 32);
            ret = 1;
        }
    }

    return ret;
}

int
#ifdef __STDC__
ucisprop(unsigned long code, unsigned long mask1, unsigned long mask2)
#else
ucisprop(code, mask1, mask2)
unsigned long code, mask1, mask2;
#endif
{
    unsigned long i;

    if (mask1 == 0 && mask2 == 0)
      return 0;

    for (i = 0; mask1 && i < 32; i++) {
        if ((mask1 & masks32[i]) && _ucprop_lookup(code, i))
          return 1;
    }

    for (i = 32; mask2 && i < _ucprop_size; i++) {
        if ((mask2 & masks32[i & 31]) && _ucprop_lookup(code, i))
          return 1;
    }

    return 0;
}

/**************************************************************************
 *
 * Support for the UTF-8 character properties.
 *
 **************************************************************************/

static unsigned long  _uc8prop_size;
static unsigned short *_uc8prop_offsets;
static unsigned char  *_uc8prop_ranges;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_uc8prop_load(char *paths, int reload)
#else
_uc8prop_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long size, i;
    _ucheader_t hdr;

    if (_uc8prop_size > 0) {
        if (!reload)
          /*
           * The UTF-8 character properties have already been loaded.
           */
          return 0;

        /*
         * Unload the current UTF-8 character property data in preparation for
         * loading a new copy.  Only the first array has to be deallocated
         * because all the memory for the arrays is allocated as a single
         * block.
         */
        free((char *) _uc8prop_offsets);
        _uc8prop_size = 0;
    }

    if ((in = _ucopenfile(paths, "utf8type.dat", "rb")) == 0)
      return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    if ((_uc8prop_size = hdr.cnt) == 0) {
        fclose(in);
        return -1;
    }

    /*
     * Allocate all the storage needed for the lookup table.
     */
    _uc8prop_offsets = (unsigned short *) malloc(hdr.size.bytes);

    /*
     * Calculate the offset into the storage for the ranges.  The offsets
     * array is on a 4-byte boundary and one larger than the value provided in
     * the header count field.  This means the offset to the ranges must be
     * calculated after aligning the count to a 4-byte boundary.
     */
    if ((size = ((hdr.cnt + 1) * sizeof(unsigned short))) & 3)
      size += 4 - (size & 3);
    size >>= 1;
    _uc8prop_ranges = (unsigned char *) (_uc8prop_offsets + size);

    /*
     * Load the offset array.
     */
    fread((char *) _uc8prop_offsets, sizeof(unsigned short), size, in);

    /*
     * Do an endian swap if necessary.  Don't forget there is an extra node on
     * the end with the final index.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i <= _uc8prop_size; i++)
          _uc8prop_offsets[i] = endian_short(_uc8prop_offsets[i]);
    }

    /*
     * Load the ranges.  The number of elements is in the last array position
     * of the offsets.
     */
    fread((char *) _uc8prop_ranges, sizeof(unsigned char),
          _uc8prop_offsets[_uc8prop_size], in);

    fclose(in);

    return 0;
}

static void
#ifdef __STDC__
_uc8prop_unload(void)
#else
_uc8prop_unload()
#endif
{
    if (_uc8prop_size == 0)
      return;

    /*
     * Only need to free the offsets because the memory is allocated as a
     * single block.
     */
    free((char *) _uc8prop_offsets);
    _uc8prop_size = 0;
}

static int
#ifdef __STDC__
_uc8prop_lookup(unsigned char *c, unsigned long n)
#else
_uc8prop_lookup(c, n)
unsigned char *c;
unsigned long n;
#endif
{
    unsigned char *sp, *ep;
    int cbytes, bytes;
    long l, r, m;

    /*
     * Set the pointer for determining lengths of UTF-8 characters.
     */
    if (clp == 0)
      clp = &clengths[2];

    /*
     * There is an extra node on the end of the offsets to allow this routine
     * to work right.  If the index is 0xffff, then there are no nodes for the
     * property.
     */
    if ((l = _uc8prop_offsets[n]) == 0xffff)
      return 0;

    cbytes = clp[((*c ^ 0xf0) >> 4) - ((*c / 0xf8) + (*c / 0xfc))];

    /*
     * Locate the next offset that is not 0xffff.  The sentinel at the end of
     * the array is the max index value.
     */
    for (m = 1;
         n + m < _uc8prop_size && _uc8prop_offsets[n + m] == 0xffff; m++) ;

    r = _uc8prop_offsets[n + m] - 1;

    sp = _uc8prop_ranges + l;
    ep = _uc8prop_ranges + r;

    m = 0;
    bytes = clp[((*sp ^ 0xf0) >> 4) - ((*sp / 0xf8) + (*sp / 0xf9))];
    while (sp < ep && cbytes > bytes) {
        sp += bytes;
        bytes = clp[((*sp ^ 0xf0) >> 4) - ((*sp / 0xf8) + (*sp / 0xf9))];
        /*
         * Count the number of characters skipped so we know if we are at the
         * beginning or end of a character range. If m is even, we are at the
         * beginning of a range, odd if at the end of a range.
         */
        m++;
    }

    /*
     * All ranges have a shorter byte count, so no match possible.
     */
    if (sp == ep)
      return 0;

    if (cbytes < bytes)
      /*
       * The character length is less than the length of the range
       * character. If this happened at the start of the range, then we know
       * the character is *not* in the range because its length is strictly
       * less than the length of the character at the start of the range. If
       * this happened at the end of a range, then the character *must* be in
       * the range because its length is strictly greater than the character
       * at the start of the range and strictly less than the length of the
       * character at the end of the range.
       */
      return (m & 1) ? 1 : 0;

    /*
     * The lengths are equal.
     */
    while (sp < ep && cbytes == bytes && (r = memcmp(c, sp, cbytes)) > 0) {
        sp += bytes;
        bytes = clp[((*sp ^ 0xf0) >> 4) - ((*sp / 0xf8) + (*sp / 0xf9))];
        m++;
    }

    if (sp == ep)
      /*
       * No match possible because end of this set of ranges reached.
       */
      return 0;

    if (r == 0)
      /*
       * Exact match at one end of the range.
       */
      return 1;

    /*
     * One of two possible conditions led to this point.
     *
     * 1) The length of the character is strictly less than the length
     *    of the range character.
     * 2) The byte comparison between the character and the range character
     *    indicates the character is strictly less than the range character.
     *
     * In both of these cases, if we are at the end of a range, then the
     * character is in the range, otherwise it is not.
     */
    return (m & 1) ? 1 : 0;
}

int
#ifdef __STDC__
uc8props(unsigned char *code, unsigned long mask1, unsigned long mask2,
        unsigned long *mask1_out, unsigned long *mask2_out)
#else
uc8props(code, mask1, mask2, mask1_out, mask2_out)
unsigned char *code;
unsigned long mask1, mask2, *mask1_out, *mask2_out;
#endif
{
    int ret = 0;
    unsigned long i;

    if (mask1 == 0 && mask2 == 0)
      return ret;

    if (mask1_out)
      *mask1_out = 0;
    if (mask2_out)
      *mask2_out = 0;

    for (i = 0; mask1 && i < 32; i++) {
        if ((mask1 & masks32[i]) && _uc8prop_lookup(code, i)) {
            *mask1_out |= 1 << i;
            ret = 1;
        }
    }

    for (i = 32; mask2 && i < _uc8prop_size; i++) {
        if ((mask2 & masks32[i & 31]) && _uc8prop_lookup(code, i)) {
            *mask2_out |= 1 << (i - 32);
            ret = 1;
        }
    }

    return ret;
}

int
#ifdef __STDC__
uc8isprop(unsigned char *code, unsigned long mask1, unsigned long mask2)
#else
uc8isprop(code, mask1, mask2)
unsigned char *code;
unsigned long mask1, mask2;
#endif
{
    unsigned long i;

    if (mask1 == 0 && mask2 == 0)
      return 0;

    for (i = 0; mask1 && i < 32; i++) {
        if ((mask1 & masks32[i]) && _uc8prop_lookup(code, i))
          return 1;
    }

    for (i = 32; mask2 && i < _uc8prop_size; i++) {
        if ((mask2 & masks32[i & 31]) && _uc8prop_lookup(code, i))
          return 1;
    }

    return 0;
}

/**************************************************************************
 *
 * Support for case mapping.
 *
 **************************************************************************/

static unsigned long _uccase_size;
static unsigned short _uccase_len[2];
static unsigned long *_uccase_map;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_uccase_load(char *paths, int reload)
#else
_uccase_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long i;
    _ucheader_t hdr;

    if (_uccase_size > 0) {
        if (!reload)
          /*
           * The case mappings have already been loaded.
           */
          return 0;

        free((char *) _uccase_map);
        _uccase_size = 0;
    }

    if ((in = _ucopenfile(paths, "case.dat", "rb")) == 0)
      return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.len[0] = endian_short(hdr.size.len[0]);
        hdr.size.len[1] = endian_short(hdr.size.len[1]);
    }

    /*
     * Set the node count and lengths of the upper and lower case mapping
     * tables.
     */
    _uccase_size = hdr.cnt * 3;
    _uccase_len[0] = hdr.size.len[0] * 3;
    _uccase_len[1] = hdr.size.len[1] * 3;

    _uccase_map = (unsigned long *)
        malloc(_uccase_size * sizeof(unsigned long));

    /*
     * Load the case mapping table.
     */
    fread((char *) _uccase_map, sizeof(unsigned long), _uccase_size, in);

    fclose(in);

    /*
     * Do an endian swap if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < _uccase_size; i++)
          _uccase_map[i] = endian_long(_uccase_map[i]);
    }
    return 0;
}

static void
#ifdef __STDC__
_uccase_unload(void)
#else
_uccase_unload()
#endif
{
    if (_uccase_size == 0)
      return;

    free((char *) _uccase_map);
    _uccase_size = 0;
}

static unsigned long
#ifdef __STDC__
_uccase_lookup(unsigned long code, long l, long r, int field)
#else
_uccase_lookup(code, l, r, field)
unsigned long code;
long l, r;
int field;
#endif
{
    long m;

    /*
     * Do the binary search.
     */
    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a case mapping triple.
         */
        m = (l + r) >> 1;
        m -= (m % 3);
        if (code > _uccase_map[m])
          l = m + 3;
        else if (code < _uccase_map[m])
          r = m - 3;
        else if (code == _uccase_map[m])
          return _uccase_map[m + field];
    }

    return code;
}

unsigned long
#ifdef __STDC__
uctoupper(unsigned long code)
#else
uctoupper(code)
unsigned long code;
#endif
{
    int field = 0;
    long l, r;

    if (ucislower(code)) {
        /*
         * The character is lower case.
         */
        field = 1;
        l = _uccase_len[0];
        r = (l + _uccase_len[1]) - 3;
    } else if (ucistitle(code)) {
        /*
         * The character is title case.
         */
        field = 2;
        l = _uccase_len[0] + _uccase_len[1];
        r = _uccase_size - 3;
    }
    return (field) ? _uccase_lookup(code, l, r, field) : code;
}

unsigned long
#ifdef __STDC__
uctolower(unsigned long code)
#else
uctolower(code)
unsigned long code;
#endif
{
    int field = 0;
    long l, r;

    if (ucisupper(code)) {
        /*
         * The character is upper case.
         */
        field = 1;
        l = 0;
        r = _uccase_len[0] - 3;
    } else if (ucistitle(code)) {
        /*
         * The character is title case.
         */
        field = 2;
        l = _uccase_len[0] + _uccase_len[1];
        r = _uccase_size - 3;
    }
    return (field) ? _uccase_lookup(code, l, r, field) : code;
}

unsigned long
#ifdef __STDC__
uctotitle(unsigned long code)
#else
uctotitle(code)
unsigned long code;
#endif
{
    int field = 0;
    long l, r;

    if (ucisupper(code)) {
        /*
         * The character is upper case.
         */
        l = 0;
        r = _uccase_len[0] - 3;
        field = 2;
    } else if (ucislower(code)) {
        /*
         * The character is lower case.
         */
        l = _uccase_len[0];
        r = (l + _uccase_len[1]) - 3;
        field = 2;
    }
    return (field) ? _uccase_lookup(code, l, r, field) : code;
}

/**************************************************************************
 *
 * Support for compositions.
 *
 **************************************************************************/

static unsigned long  _uccomp_size;
static unsigned long *_uccomp_data;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_uccomp_load(char *paths, int reload)
#else
_uccomp_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long size, i;
    _ucheader_t hdr;

    if (_uccomp_size > 0) {
        if (!reload)
            /*
             * The compositions have already been loaded.
             */
            return 0;

        free((char *) _uccomp_data);
        _uccomp_size = 0;
    }

    if ((in = _ucopenfile(paths, "comp.dat", "rb")) == 0)
        return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    _uccomp_size = hdr.cnt;
    _uccomp_data = (unsigned long *) malloc(hdr.size.bytes);

    /*
     * Read the composition data in.
     */
    size = hdr.size.bytes / sizeof(unsigned long);
    fread((char *) _uccomp_data, sizeof(unsigned long), size, in);

    fclose(in);

    /*
     * Do an endian swap if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < size; i++)
            _uccomp_data[i] = endian_long(_uccomp_data[i]);
    }

    /*
     * Assume that the data is ordered on count, so that all compositions
     * of length 2 come first. Only handling length 2 for now.
     */
    for (i = 1; i < size; i += 4)
      if (_uccomp_data[i] > 2)
        break;
    _uccomp_size = i - 1;

    return 0;
}

static void
#ifdef __STDC__
_uccomp_unload(void)
#else
_uccomp_unload()
#endif
{
    if (_uccomp_size == 0)
        return;

    free((char *) _uccomp_data);
    _uccomp_size = 0;
}

int
#ifdef __STDC__
uccomp(unsigned long node1, unsigned long node2, unsigned long *comp)
#else
uccomp(node1, node2, comp)
unsigned long node1, node2, *comp;
#endif
{
    int l, r, m;

    l = 0;
    r = _uccomp_size - 1;

    while (l <= r) {
        m = ((r + l) >> 1);
        m -= m & 3;
        if (node1 > _uccomp_data[m+2])
          l = m + 4;
        else if (node1 < _uccomp_data[m+2])
          r = m - 4;
        else if (node2 > _uccomp_data[m+3])
          l = m + 4;
        else if (node2 < _uccomp_data[m+3])
          r = m - 4;
        else {
            *comp = _uccomp_data[m];
            return 1;
        }
    }
    return 0;
}

int
#ifdef __STDC__
uccomp_hangul(unsigned long *str, int len)
#else
uccomp_hangul(str, len)
unsigned long *str;
int len;
#endif
{
    const int SBase = 0xAC00, LBase = 0x1100,
        VBase = 0x1161, TBase = 0x11A7,
        LCount = 19, VCount = 21, TCount = 28,
        NCount = VCount * TCount,   /* 588 */
        SCount = LCount * NCount;   /* 11172 */
    
    int i, rlen;
    unsigned long ch, last, lindex, sindex;

    last = str[0];
    rlen = 1;
    for ( i = 1; i < len; i++ ) {
        ch = str[i];

        /* check if two current characters are L and V */
        lindex = last - LBase;
        if (0 <= lindex && lindex < LCount) {
            unsigned long vindex = ch - VBase;
            if (0 <= vindex && vindex < VCount) {
                /* make syllable of form LV */
                last = SBase + (lindex * VCount + vindex) * TCount;
                str[rlen-1] = last; /* reset last */
                continue;
            }
        }
        
        /* check if two current characters are LV and T */
        sindex = last - SBase;
        if (0 <= sindex && sindex < SCount && (sindex % TCount) == 0) {
            unsigned long tindex = ch - TBase;
            if (0 <= tindex && tindex <= TCount) {
                /* make syllable of form LVT */
                last += tindex;
                str[rlen-1] = last; /* reset last */
                continue;
            }
        }

        /* if neither case was true, just add the character */
        last = ch;
        str[rlen] = ch;
        rlen++;
    }
    return rlen;
}

int
#ifdef __STDC__
uccanoncomp(unsigned long *str, int len)
#else
uccanoncomp(str, len)
unsigned long *str;
int len;
#endif
{
    int i, stpos, copos;
    unsigned long cl, prevcl, st, ch, co;

    st = str[0];
    stpos = 0;
    copos = 1;
    prevcl = uccombining_class(st) == 0 ? 0 : 256;
        
    for (i = 1; i < len; i++) {
        ch = str[i];
        cl = uccombining_class(ch);
        if (uccomp(st, ch, &co) && (prevcl < cl || prevcl == 0))
          st = str[stpos] = co;
        else {
            if (cl == 0) {
                stpos = copos;
                st = ch;
            }
            prevcl = cl;
            str[copos++] = ch;
        }
    }

    return uccomp_hangul(str, copos);
}

/**************************************************************************
 *
 * Support for decompositions.
 *
 **************************************************************************/

static unsigned long  _ucdcmp_size;
static unsigned long *_ucdcmp_nodes;
static unsigned long *_ucdcmp_decomp;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_ucdcmp_load(char *paths, int reload)
#else
_ucdcmp_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long size, i;
    _ucheader_t hdr;

    if (_ucdcmp_size > 0) {
        if (!reload)
            /*
             * The decompositions have already been loaded.
             */
            return 0;

        free((char *) _ucdcmp_nodes);
        _ucdcmp_size = 0;
    }

    if ((in = _ucopenfile(paths, "decomp.dat", "rb")) == 0)
        return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    _ucdcmp_size = hdr.cnt << 1;
    _ucdcmp_nodes = (unsigned long *) malloc(hdr.size.bytes);
    _ucdcmp_decomp = _ucdcmp_nodes + (_ucdcmp_size + 1);

    /*
     * Read the decomposition data in.
     */
    size = hdr.size.bytes / sizeof(unsigned long);
    fread((char *) _ucdcmp_nodes, sizeof(unsigned long), size, in);

    fclose(in);

    /*
     * Do an endian swap if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < size; i++)
            _ucdcmp_nodes[i] = endian_long(_ucdcmp_nodes[i]);
    }
    return 0;
}

static void
#ifdef __STDC__
_ucdcmp_unload(void)
#else
_ucdcmp_unload()
#endif
{
    if (_ucdcmp_size == 0)
      return;

    /*
     * Only need to free the offsets because the memory is allocated as a
     * single block.
     */
    free((char *) _ucdcmp_nodes);
    _ucdcmp_size = 0;
}

int
#ifdef __STDC__
ucdecomp(unsigned long code, unsigned long *num, unsigned long **decomp)
#else
ucdecomp(code, num, decomp)
unsigned long code, *num, **decomp;
#endif
{
    long l, r, m;

    l = 0;
    r = _ucdcmp_nodes[_ucdcmp_size] - 1;

    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a code+offset pair.
         */
        m = (l + r) >> 1;
        m -= (m & 1);
        if (code > _ucdcmp_nodes[m])
          l = m + 2;
        else if (code < _ucdcmp_nodes[m])
          r = m - 2;
        else if (code == _ucdcmp_nodes[m]) {
            *num = _ucdcmp_nodes[m + 3] - _ucdcmp_nodes[m + 1];
            *decomp = &_ucdcmp_decomp[_ucdcmp_nodes[m + 1]];
            return 1;
        }
    }
    return 0;
}

int
#ifdef __STDC__
ucdecomp_hangul(unsigned long code, unsigned long *num, unsigned long decomp[])
#else
ucdecomp_hangul(code, num, decomp)
unsigned long code, *num, decomp[];
#endif
{
    if (!ucishangul(code))
      return 0;

    code -= 0xac00;
    decomp[0] = 0x1100 + (unsigned long) (code / 588);
    decomp[1] = 0x1161 + (unsigned long) ((code % 588) / 28);
    decomp[2] = 0x11a7 + (unsigned long) (code % 28);
    *num = (decomp[2] != 0x11a7) ? 3 : 2;

    return 1;
}

int
#ifdef __STDC__
uccanondecomp(const unsigned long *in, int inlen,
              unsigned long **out, int *outlen)
#else
uccanondecomp(in, inlen, out, outlen)
const unsigned long *in;
int inlen;
unsigned long **out;
int *outlen;
#endif
{
    int i, j, k, l, size;
    unsigned long num, class, *decomp, hangdecomp[3];

    size = inlen;
    *out = (unsigned long *) malloc(size * sizeof(**out));
    if (*out == NULL)
        return *outlen = -1;

    i = 0;
    for (j = 0; j < inlen; j++) {
        if (ucdecomp(in[j], &num, &decomp)) {
            if (size - i < num) {
                size = inlen + i - j + num - 1;
                *out = (unsigned long *) realloc(*out, size * sizeof(**out));
                if (*out == NULL)
                    return *outlen = -1;
            }
            for (k = 0; k < num; k++) {
                class = uccombining_class(decomp[k]);
                if (class == 0) {
                    (*out)[i] = decomp[k];
                } else {
                    for (l = i; l > 0; l--)
                        if (class >= uccombining_class((*out)[l-1]))
                            break;
                    memmove(*out + l + 1, *out + l, (i - l) * sizeof(**out));
                    (*out)[l] = decomp[k];
                }
                i++;
            }
        } else if (ucdecomp_hangul(in[j], &num, hangdecomp)) {
            if (size - i < num) {
                size = inlen + i - j + num - 1;
                *out = (unsigned long *) realloc(*out, size * sizeof(**out));
                if (*out == NULL)
                    return *outlen = -1;
            }
            for (k = 0; k < num; k++) {
                (*out)[i] = hangdecomp[k];
                i++;
            }
        } else {
            if (size - i < 1) {
                size = inlen + i - j;
                *out = (unsigned long *) realloc(*out, size * sizeof(**out));
                if (*out == NULL)
                    return *outlen = -1;
            }
            class = uccombining_class(in[j]);
            if (class == 0) {
                (*out)[i] = in[j];
            } else {
                for (l = i; l > 0; l--)
                    if (class >= uccombining_class((*out)[l-1]))
                        break;
                memmove(*out + l + 1, *out + l, (i - l) * sizeof(**out));
                (*out)[l] = in[j];
            }
            i++;
        }
    }
    return *outlen = i;
}

/**************************************************************************
 *
 * Support for combining classes.
 *
 **************************************************************************/

static unsigned long  _uccmcl_size;
static unsigned long *_uccmcl_nodes;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_uccmcl_load(char *paths, int reload)
#else
_uccmcl_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long i;
    _ucheader_t hdr;

    if (_uccmcl_size > 0) {
        if (!reload)
            /*
             * The combining classes have already been loaded.
             */
            return 0;

        free((char *) _uccmcl_nodes);
        _uccmcl_size = 0;
    }

    if ((in = _ucopenfile(paths, "cmbcl.dat", "rb")) == 0)
        return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    _uccmcl_size = hdr.cnt * 3;
    _uccmcl_nodes = (unsigned long *) malloc(hdr.size.bytes);

    /*
     * Read the combining classes in.
     */
    fread((char *) _uccmcl_nodes, sizeof(unsigned long), _uccmcl_size, in);

    fclose(in);

    /*
     * Do an endian swap if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < _uccmcl_size; i++)
            _uccmcl_nodes[i] = endian_long(_uccmcl_nodes[i]);
    }
    return 0;
}

static void
#ifdef __STDC__
_uccmcl_unload(void)
#else
_uccmcl_unload()
#endif
{
    if (_uccmcl_size == 0)
      return;

    free((char *) _uccmcl_nodes);
    _uccmcl_size = 0;
}

unsigned long
#ifdef __STDC__
uccombining_class(unsigned long code)
#else
uccombining_class(code)
unsigned long code;
#endif
{
    long l, r, m;

    l = 0;
    r = _uccmcl_size - 1;

    while (l <= r) {
        m = (l + r) >> 1;
        m -= (m % 3);
        if (code > _uccmcl_nodes[m + 1])
          l = m + 3;
        else if (code < _uccmcl_nodes[m])
          r = m - 3;
        else if (code >= _uccmcl_nodes[m] && code <= _uccmcl_nodes[m + 1])
          return _uccmcl_nodes[m + 2];
    }
    return 0;
}

/**************************************************************************
 *
 * Support for numeric values.
 *
 **************************************************************************/

static unsigned long *_ucnum_nodes;
static unsigned long _ucnum_size;
static short *_ucnum_vals;

/*
 * Return -1 on error, 0 if okay
 */
static int
#ifdef __STDC__
_ucnumb_load(char *paths, int reload)
#else
_ucnumb_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned long size, i;
    _ucheader_t hdr;

    if (_ucnum_size > 0) {
        if (!reload)
          /*
           * The numbers have already been loaded.
           */
          return 0;

        free((char *) _ucnum_nodes);
        _ucnum_size = 0;
    }

    if ((in = _ucopenfile(paths, "num.dat", "rb")) == 0)
      return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    _ucnum_size = hdr.cnt;
    _ucnum_nodes = (unsigned long *) malloc(hdr.size.bytes);
    _ucnum_vals = (short *) (_ucnum_nodes + _ucnum_size);

    /*
     * Read the combining classes in.
     */
    fread((char *) _ucnum_nodes, sizeof(unsigned char), hdr.size.bytes, in);

    fclose(in);

    /*
     * Do an endian swap if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < _ucnum_size; i++)
          _ucnum_nodes[i] = endian_long(_ucnum_nodes[i]);

        /*
         * Determine the number of values that have to be adjusted.
         */
        size = (hdr.size.bytes -
                (_ucnum_size * (sizeof(unsigned long) << 1))) /
            sizeof(short);

        for (i = 0; i < size; i++)
          _ucnum_vals[i] = endian_short(_ucnum_vals[i]);
    }
    return 0;
}

static void
#ifdef __STDC__
_ucnumb_unload(void)
#else
_ucnumb_unload()
#endif
{
    if (_ucnum_size == 0)
      return;

    free((char *) _ucnum_nodes);
    _ucnum_size = 0;
}

int
#ifdef __STDC__
ucnumber_lookup(unsigned long code, struct ucnumber *num)
#else
ucnumber_lookup(code, num)
unsigned long code;
struct ucnumber *num;
#endif
{
    long l, r, m;
    short *vp;

    l = 0;
    r = _ucnum_size - 1;
    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a code+offset pair.
         */
        m = (l + r) >> 1;
        m -= (m & 1);
        if (code > _ucnum_nodes[m])
          l = m + 2;
        else if (code < _ucnum_nodes[m])
          r = m - 2;
        else {
            vp = _ucnum_vals + _ucnum_nodes[m + 1];
            num->numerator = (int) *vp++;
            num->denominator = (int) *vp;
            return 1;
        }
    }
    return 0;
}

int
#ifdef __STDC__
ucdigit_lookup(unsigned long code, int *digit)
#else
ucdigit_lookup(code, digit)
unsigned long code;
int *digit;
#endif
{
    long l, r, m;
    short *vp;

    l = 0;
    r = _ucnum_size - 1;
    while (l <= r) {
        /*
         * Determine a "mid" point and adjust to make sure the mid point is at
         * the beginning of a code+offset pair.
         */
        m = (l + r) >> 1;
        m -= (m & 1);
        if (code > _ucnum_nodes[m])
          l = m + 2;
        else if (code < _ucnum_nodes[m])
          r = m - 2;
        else {
            vp = _ucnum_vals + _ucnum_nodes[m + 1];
            if (*vp == *(vp + 1)) {
              *digit = *vp;
              return 1;
            }
            return 0;
        }
    }
    return 0;
}

struct ucnumber
#ifdef __STDC__
ucgetnumber(unsigned long code)
#else
ucgetnumber(code)
unsigned long code;
#endif
{
    struct ucnumber num;

    /*
     * Initialize with some arbitrary value, because the caller simply cannot
     * tell for sure if the code is a number without calling the ucisnumber()
     * macro before calling this function.
     */
    num.numerator = num.denominator = -111;

    (void) ucnumber_lookup(code, &num);

    return num;
}

int
#ifdef __STDC__
ucgetdigit(unsigned long code)
#else
ucgetdigit(code)
unsigned long code;
#endif
{
    int dig;

    /*
     * Initialize with some arbitrary value, because the caller simply cannot
     * tell for sure if the code is a number without calling the ucisdigit()
     * macro before calling this function.
     */
    dig = -111;

    (void) ucdigit_lookup(code, &dig);

    return dig;
}

/**************************************************************************
 *
 * Support for the UTF-8 lower case table.
 *
 **************************************************************************/

static unsigned short *_uccasecmp_nodes;
static unsigned short _uccasecmp_size;
static unsigned char *_uccasecmp_lower;

static int
#ifdef __STDC__
_uccasecmp_load(char *paths, int reload)
#else
_uccasecmp_load(paths, reload)
char *paths;
int reload;
#endif
{
    FILE *in;
    unsigned short i;
    _ucheader_t hdr;

    if (_uccasecmp_size > 0) {
        if (!reload)
          return 0;

        free((char *) _uccasecmp_nodes);
        _uccasecmp_size = 0;
    }
    if ((in = _ucopenfile(paths, "casecmp.dat", "rb")) == 0)
      return -1;

    /*
     * Load the header.
     */
    fread((char *) &hdr, sizeof(_ucheader_t), 1, in);

    if (hdr.bom == 0xfffe) {
        hdr.cnt = endian_short(hdr.cnt);
        hdr.size.bytes = endian_long(hdr.size.bytes);
    }

    _uccasecmp_size = hdr.cnt;
    _uccasecmp_nodes = (unsigned short *) malloc(hdr.size.bytes);
    _uccasecmp_lower = (unsigned char *) (_uccasecmp_nodes + _uccasecmp_size);

    /*
     * Read in the lookup trie.
     */
    fread((char *) _uccasecmp_nodes, sizeof(unsigned char), hdr.size.bytes, in);

    fclose(in);

    /*
     * Byte swap the lookup trie if necessary.
     */
    if (hdr.bom == 0xfffe) {
        for (i = 0; i < _uccasecmp_size; i++)
          _uccasecmp_nodes[i] = endian_short(_uccasecmp_nodes[i]);
    }

    return 0;
}

static void
#ifdef __STDC__
_uccasecmp_unload(void)
#else
_uccasecmp_unload()
#endif
{
    if (_uccasecmp_size == 0)
      return;

    free((char *) _uccasecmp_nodes);
    _uccasecmp_size = 0;
}

int
#ifdef __STDC__
ucgetutf8lower(unsigned char *upper, unsigned char **lower)
#else
ucgetutf8lower(upper, lower)
unsigned char *upper,  **lower;
#endif
{
    int i, n, t, l;

    if (clp == 0)
      clp = &clengths[2];

    /*
     * Determine the number of bytes used for the first character.
     */
    n = clp[((*upper ^ 0xf0) >> 4) - ((*upper / 0xf8) + (*upper / 0xfc))];

    for (i = t = 0; i < n; i++) {
        for (l = 0; (_uccasecmp_nodes[t] & 0xff) != upper[i] &&
                    (_uccasecmp_nodes[t] & 0xff) > l;
             l = (_uccasecmp_nodes[t] & 0xff),
             t += 2 + ((_uccasecmp_nodes[t] & 0x100) >> 8));

        if (upper[i] == (_uccasecmp_nodes[t] & 0xff) && i + 1 == n)
          /*
           * A full match has been made.
           */
          break;

        if (upper[i] != (_uccasecmp_nodes[t] & 0xff) ||
            !(_uccasecmp_nodes[t] & 0x100)) {
            /*
             * The keys don't match or the node has no children.  Return a
             * pointer to the "upper" character and return it's length.
             */
            *lower = upper;
            return n;
        }

        t = _uccasecmp_nodes[t+2];
    }

    l = (int) _uccasecmp_lower[_uccasecmp_nodes[t+1] + 1];
    if (lower != 0)
      *lower = _uccasecmp_lower + (_uccasecmp_nodes[t+1] + 2);
    return l;
}

/**************************************************************************
 *
 * Setup and cleanup routines.
 *
 **************************************************************************/

/*
 * Return 0 if okay, negative on error
 */
int
#ifdef __STDC__
ucdata_load(char *paths, int masks)
#else
ucdata_load(paths, masks)
char *paths;
int masks;
#endif
{
    int error = 0;

    if (masks & UCDATA_CTYPE)
      error |= _ucprop_load(paths, 0) < 0 ? UCDATA_CTYPE : 0;
    if (masks & UCDATA_CASE)
      error |= _uccase_load(paths, 0) < 0 ? UCDATA_CASE : 0;
    if (masks & UCDATA_DECOMP)
      error |= _ucdcmp_load(paths, 0) < 0 ? UCDATA_DECOMP : 0;
    if (masks & UCDATA_CMBCL)
      error |= _uccmcl_load(paths, 0) < 0 ? UCDATA_CMBCL : 0;
    if (masks & UCDATA_NUM)
      error |= _ucnumb_load(paths, 0) < 0 ? UCDATA_NUM : 0;
    if (masks & UCDATA_COMP)
      error |= _uccomp_load(paths, 0) < 0 ? UCDATA_COMP : 0;
    if (masks & UCDATA_CASECMP)
      error |= _uccasecmp_load(paths, 0) < 0 ? UCDATA_CASECMP : 0;
    if (masks & UCDATA_UC8CTYPE)
      error |= _uc8prop_load(paths, 0) < 0 ? UCDATA_UC8CTYPE : 0;

    return -error;
}

void
#ifdef __STDC__
ucdata_unload(int masks)
#else
ucdata_unload(masks)
int masks;
#endif
{
    if (masks & UCDATA_CTYPE)
      _ucprop_unload();
    if (masks & UCDATA_CASE)
      _uccase_unload();
    if (masks & UCDATA_DECOMP)
      _ucdcmp_unload();
    if (masks & UCDATA_CMBCL)
      _uccmcl_unload();
    if (masks & UCDATA_NUM)
      _ucnumb_unload();
    if (masks & UCDATA_COMP)
      _uccomp_unload();
    if (masks & UCDATA_CASECMP)
      _uccasecmp_unload();
    if (masks & UCDATA_UC8CTYPE)
      _uc8prop_unload();
}

/*
 * Return 0 if okay, negative on error
 */
int
#ifdef __STDC__
ucdata_reload(char *paths, int masks)
#else
ucdata_reload(paths, masks)
char *paths;
int masks;
#endif
{
    int error = 0;

    if (masks & UCDATA_CTYPE)
      error |= _ucprop_load(paths, 1) < 0 ? UCDATA_CTYPE : 0;
    if (masks & UCDATA_CASE)
      error |= _uccase_load(paths, 1) < 0 ? UCDATA_CASE : 0;
    if (masks & UCDATA_DECOMP)
      error |= _ucdcmp_load(paths, 1) < 0 ? UCDATA_DECOMP : 0;
    if (masks & UCDATA_CMBCL)
      error |= _uccmcl_load(paths, 1) < 0 ? UCDATA_CMBCL : 0;
    if (masks & UCDATA_NUM)
      error |= _ucnumb_load(paths, 1) < 0 ? UCDATA_NUM : 0;
    if (masks & UCDATA_COMP)
      error |= _uccomp_load(paths, 1) < 0 ? UCDATA_COMP : 0;
    if (masks & UCDATA_CASECMP)
      error |= _uccasecmp_load(paths, 1) < 0 ? UCDATA_CASECMP : 0;
    if (masks & UCDATA_UC8CTYPE)
      error |= _uc8prop_load(paths, 1) < 0 ? UCDATA_UC8CTYPE : 0;

    return -error;
}

/**************************************************************************
 *
 * Conversion routines.
 *
 **************************************************************************/

unsigned long
uctoutf32(unsigned char utf8[], int bytes)
{
    unsigned long out = 0;

    switch (bytes) {
      case 1:
        out = utf8[0];
        break;
      case 2:
        out = ((utf8[0] & 0x1f) << 6) | (utf8[1] & 0x3f);
        break;
      case 3:
        out = ((utf8[0] & 0xf) << 12) | ((utf8[1] & 0x3f) << 6) |
            (utf8[2] & 0x3f);
        break;
      case 4:
        out = ((utf8[0] & 7) << 18) | ((utf8[1] & 0x3f) << 12) |
            ((utf8[2] & 0x3f) << 6) | (utf8[3] & 0x3f);
        break;
      case 5:
        out = ((utf8[0] & 3) << 24) | ((utf8[1] & 0x3f) << 18) |
            ((utf8[2] & 0x3f) << 12) | ((utf8[3] & 0x3f) << 6) |
            (utf8[4] & 0x3f);
        break;
      case 6:
        out = ((utf8[0] & 1) << 30) | ((utf8[1] & 0x3f) << 24) |
            ((utf8[2] & 0x3f) << 18) | ((utf8[3] & 0x3f) << 12) |
            ((utf8[4] & 0x3f) << 6) | (utf8[5] & 0x3f);
        break;
    }
    return out;
}

int
uctoutf8(unsigned long ch, unsigned char buf[], int bufsize)
{
    int i = 0;

    if (ch < 0x80) {
        if (i + 1 >= bufsize)
          return -1;
        buf[i++] = ch & 0xff;
    } else if (ch < 0x800) {
        if (i + 2 >= bufsize)
          return -1;
        buf[i++] = 0xc0 | (ch >> 6);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch < 0x10000) {
        if (i + 3 >= bufsize)
          return -1;
        buf[i++] = 0xe0 | (ch >> 12);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch < 0x200000) {
        if (i + 4 >= bufsize)
          return -1;
        buf[i++] = 0xf0 | (ch >> 18);
        buf[i++] = 0x80 | ((ch >> 12) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch < 0x4000000) {
        if (i + 5 >= bufsize)
          return -1;
        buf[i++] = 0xf8 | (ch >> 24);
        buf[i++] = 0x80 | ((ch >> 18) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 12) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    } else if (ch <= 0x7ffffff) {
        if (i + 6 >= bufsize)
          return -1;
        buf[i++] = 0xfc | (ch >> 30);
        buf[i++] = 0x80 | ((ch >> 24) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 18) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 12) & 0x3f);
        buf[i++] = 0x80 | ((ch >> 6) & 0x3f);
        buf[i++] = 0x80 | (ch & 0x3f);
    }
    return i;
}

#ifdef TEST

int
#ifdef __STDC__
main(void)
#else
main()
#endif
{
    unsigned char upper[5], *lower;
    int dig, ulen, llen;
    unsigned long i, lo, *dec;
    struct ucnumber num;

    ucdata_load(".:data", UCDATA_ALL);

    /*
     * Test the lookup of UTF-8 lower case mappings from upper case
     * characters encoded in UTF-8.
     */
    for (lo = 0; lo < 4; lo++) {

        ulen = llen = 0;

        switch (lo) {
          case 0:
            /* U+0041 (utf-8: 0x41) maps to U+0061 (utf-8: 0x61) */
            upper[ulen++] = 0x41;
            break;
          case 1:
            /* U+0186 (utf-8: 0xc6 0x86) maps to U+0254 (utf-8: 0xc9 0x94) */
            upper[ulen++] = 0xc6;
            upper[ulen++] = 0x86;
            break;
          case 2:
            /*
             * U+1F59 (utf-8: 0xe1 0xbd 0x99) maps to
             * U+1F51 (utf-8: 0xe1 0xbd 0x91).
             */
            upper[ulen++] = 0xe1;
            upper[ulen++] = 0xbd;
            upper[ulen++] = 0x99;
            break;
          case 3:
            /*
             * U+10410 (utf-8: 0xf0 0x90 0x90 x090) maps to
             * U+10438 (utf-8: 0xf0 0x90 0x90 0xb8)
             */
            upper[ulen++] = 0xf0;
            upper[ulen++] = 0x90;
            upper[ulen++] = 0x90;
            upper[ulen++] = 0x90;
            break;
        }
        if ((llen = ucgetutf8lower(upper, &lower)) > 0) {
            switch (lo) {
              case 0: printf("UTF8LOWER: [U+0041] "); break;
              case 1: printf("UTF8LOWER: [U+0186] "); break;
              case 2: printf("UTF8LOWER: [U+1F59] "); break;
              case 3: printf("UTF8LOWER: [U+10410] "); break;
            }
            for (i = 0; i < ulen; i++)
              printf("%02X ", upper[i]);
            switch (lo) {
              case 0: printf("-> [U+0061] "); break;
              case 1: printf("-> [U+0254] "); break;
              case 2: printf("-> [U+1F51] "); break;
              case 3: printf("-> [U+10438] "); break;
            }
            for (i = 0; i < llen; i++)
              printf("%02X ", lower[i]);
            putchar('\n');
        }
    }

    if (ucissymmetric(0x28))
      printf("UCISSYMMETRIC: 0x28 YES\n");
    else
      printf("UCISSYMMETRIC: 0x28 NO\n");

    if (ucisprop(0x633, 0, UC_AL))
      printf("UCISAL: 0x0633 YES\n");
    else
      printf("UCISAL: 0x0633 NO\n");

    if (ucisprop(0x633, UC_R, 0))
      printf("UCISRTL: 0x0633 YES\n");
    else
      printf("UCISRTL: 0x0633 NO\n");

    if (ucisweak(0x30))
      printf("UCISWEAK: 0x30 WEAK\n");
    else
      printf("UCISWEAK: 0x30 NOT WEAK\n");

    printf("TOLOWER FF3A: 0x%04lX\n", uctolower(0xff3a));
    printf("TOUPPER FF5A: 0x%04lX\n", uctoupper(0xff5a));

    if (ucisalpha(0x1d5))
      printf("UCISALPHA: 0x01D5 ALPHA\n");
    else
      printf("UCISALPHA: 0x01D5 NOT ALPHA\n");

    if (ucisupper(0x1d5)) {
        printf("UCISUPPER: 0x01D5 UPPER\n");
        lo = uctolower(0x1d5);
        printf("TOLOWER 0x01D5: 0x%04lx\n", lo);
        lo = uctotitle(0x1d5);
        printf("TOTITLE 0x01D5: 0x%04lx\n", lo);
    } else
      printf("UCISUPPER: 0x01D5 NOT UPPER\n");

    if (ucistitle(0x1d5))
      printf("UCISTITLE: 0x01D5 TITLE\n");
    else
      printf("UCISTITLE: 0x01D5 NOT TITLE\n");

    if (uciscomposite(0x1d5))
      printf("0x01D5 COMPOSITE\n");
    else
      printf("0x01D5 NOT COMPOSITE\n");

    if (uchascase(0x1d5))
      printf("0x01D5 HAS CASE VARIANTS\n");
    if (!uchascase(0x1024))
      printf("0x1024 HAS NO CASE VARIANTS\n");

    if (ucdecomp(0x1d5, &lo, &dec)) {
        printf("0x01D5 DECOMPOSES TO ");
        for (i = 0; i < lo; i++)
          printf("0x%04lx ", dec[i]);
        putchar('\n');
    } else
      printf("0x01D5 NO DECOMPOSITION\n");

    if (uccomp(0x47, 0x301, &lo))
      printf("0x0047 0x0301 COMPOSES TO 0x%04lX\n", lo);
    else
      printf("0x0047 0x0301 DOES NOT COMPOSE TO 0x%04lX\n", lo);

    lo = uccombining_class(0x41);
    printf("UCCOMBINING_CLASS 0x41 %ld\n", lo);
    lo = uccombining_class(0x1d16f);
    printf("UCCOMBINING_CLASS 0x1D16F %ld\n", lo);

    if (ucisxdigit(0xfeff))
      printf("0xFEFF HEX DIGIT\n");
    else
      printf("0xFEFF NOT HEX DIGIT\n");

    if (ucisdefined(0x10000))
      printf("0x10000 DEFINED\n");
    else
      printf("0x10000 NOT DEFINED\n");

    if (ucnumber_lookup(0x30, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0x30 = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0x30 = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0x30 NOT A NUMBER\n");

    if (ucnumber_lookup(0xbc, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0xbc = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0xbc = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0xbc NOT A NUMBER\n");


    if (ucnumber_lookup(0xff19, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0xff19 = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0xff19 = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0xff19 NOT A NUMBER\n");

    if (ucnumber_lookup(0x4e00, &num)) {
        if (num.numerator != num.denominator)
          printf("UCNUMBER: 0x4e00 = %d/%d\n", num.numerator, num.denominator);
        else
          printf("UCNUMBER: 0x4e00 = %d\n", num.numerator);
    } else
      printf("UCNUMBER: 0x4e00 NOT A NUMBER\n");

    if (ucdigit_lookup(0x06f9, &dig))
      printf("UCDIGIT: 0x6f9 = %d\n", dig);
    else
      printf("UCDIGIT: 0x6f9 NOT A NUMBER\n");

    dig = ucgetdigit(0x0969);
    printf("UCGETDIGIT: 0x969 = %d\n", dig);

    num = ucgetnumber(0x30);
    if (num.numerator != num.denominator)
      printf("UCGETNUMBER: 0x30 = %d/%d\n", num.numerator, num.denominator);
    else
      printf("UCGETNUMBER: 0x30 = %d\n", num.numerator);

    num = ucgetnumber(0xbc);
    if (num.numerator != num.denominator)
      printf("UCGETNUMBER: 0xbc = %d/%d\n", num.numerator, num.denominator);
    else
      printf("UCGETNUMBER: 0xbc = %d\n", num.numerator);

    num = ucgetnumber(0xff19);
    if (num.numerator != num.denominator)
      printf("UCGETNUMBER: 0xff19 = %d/%d\n", num.numerator, num.denominator);
    else
      printf("UCGETNUMBER: 0xff19 = %d\n", num.numerator);

    ucdata_cleanup();

    return 0;
}

#endif /* TEST */
