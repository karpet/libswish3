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

/* string.c -- handle xmlChar and wchar_t strings
 * much of this module based on swstring.c in swish-e vers 2
 * but re-written for UTF-8 support
*/

#ifndef LIBSWISH3_SINGLE_FILE
#include <assert.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <err.h>
#include <limits.h>
#include <errno.h>

#include "libswish3.h"
#endif

extern int SWISH_DEBUG;

static xmlChar *getword(
    xmlChar **in_buf
);

#ifndef LIBSWISH3_SINGLE_FILE
#include "utf8.c"
#endif

/* these string conversion functions based on code from xapian-omega */
#define BUFSIZE 100
#define DATE_BUFSIZE 8
#define DATE_FMT "%04d%02d%02d"

#define CONVERT_TO_STRING(FMT) \
    xmlChar *str;\
    int ret;\
    str = swish_xmalloc(BUFSIZE);\
    ret = snprintf((char*)str, BUFSIZE, (FMT), val);\
    if (ret<0) SWISH_CROAK("snprintf failed with %d", ret);\
    return str;

int
swish_string_to_int(
    char *buf
)
{
    long i;
    errno = 0;
    i = strtol(buf, (char **)NULL, 10);
    /*
       Check for various possible errors 
     */
    if ((errno == ERANGE && (i == LONG_MAX || i == LONG_MIN))
        || (errno != 0 && i == 0)) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }
    return (int)i;
}

boolean
swish_string_to_boolean(
    char *buf
)
{
    if (buf == '\0' || buf == NULL) {
        return SWISH_FALSE;
    }
    if (    buf[0] == 'Y' 
        ||  buf[0] == 'y'
        ||  buf[0] == '1'    
    ) {
        return SWISH_TRUE;
    }
    if (    buf[0] == 'N'
        ||  buf[0] == 'n'
        ||  buf[0] == '0'
    ) {
        return SWISH_FALSE;
    }
    
    return SWISH_FALSE; /* default */
}

xmlChar *
swish_int_to_string(
    int val
)
{
    CONVERT_TO_STRING("%d")
}

xmlChar *
swish_long_to_string(
    long val
)
{
    CONVERT_TO_STRING("%ld")
}

xmlChar *
swish_double_to_string(
    double val
)
{
    CONVERT_TO_STRING("%f")
}

xmlChar *
swish_date_to_string(
    int y,
    int m,
    int d
)
{
    char buf[DATE_BUFSIZE + 1];
    if (y < 0)
        y = 0;
    else if (y > 9999)
        y = 9999;
    if (m < 1)
        m = 1;
    else if (m > 12)
        m = 12;
    if (d < 1)
        d = 1;
    else if (d > 31)
        d = 31;
#ifdef SNPRINTF
    int len = SNPRINTF(buf, sizeof(buf), DATE_FMT, y, m, d);
    if (len == -1 || len >= DATE_BUFSIZE)
        buf[DATE_BUFSIZE] = '\0';
#else
    buf[DATE_BUFSIZE] = '\0';
    sprintf(buf, DATE_FMT, y, m, d);
    if (buf[DATE_BUFSIZE])
        abort();                /* Uh-oh, buffer overrun */
#endif
    return swish_xstrdup((xmlChar *)buf);
}

/* returns the UCS32 value for a UTF8 string -- the character's Unicode value.
   see http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&item_id=IWS-AppendixA
*/

uint32_t
swish_utf8_codepoint(
    xmlChar *utf8
)
{
    uint32_t len;
    len = swish_utf8_chr_len(utf8);

    switch (len) {

    case 1:
        return utf8[0];

    case 2:
        return (utf8[0] - 192) * 64 + utf8[1] - 128;

    case 3:
        return (utf8[0] - 224) * 4096 + (utf8[1] - 128) * 64 + utf8[2] - 128;

    case 4:
    default:
        return (utf8[0] - 240) * 262144 + (utf8[1] - 128) * 4096 + (utf8[2] - 128) * 64 +
            utf8[3] - 128;

    }
}

void
swish_utf8_next_chr(
    xmlChar *s,
    int *i
)
{
    u8_inc((char *)s, i);
}

void
swish_utf8_prev_chr(
    xmlChar *s, 
    int *i
)
{
    u8_dec((char *)s, i);
}


/* returns length of a UTF8 character, based on first byte (see below) */
int
swish_utf8_chr_len(
    xmlChar *utf8
)
{
    int n;
    n = xmlUTF8Size(utf8);
    if (n == -1)
        SWISH_CROAK("Bad UTF8 string: %s", utf8);
        
    return n;
}

/* returns the number of UCS32 codepoints (characters) in a UTF8 string */
int
swish_utf8_num_chrs(
    xmlChar *utf8
)
{
    int n;
    n = xmlUTF8Strlen(utf8);
    if (n == -1)
        SWISH_CROAK("Bad UTF8 string: %s", utf8);
        
    return n;
}

/* returns true if all bytes in the *str are in the ascii range.
 * this helps speed up string handling when we don't need to worry
 * about multi-byte chars.
*/

/* from the libxml2 xmlstring.c file:
     * utf is a string of 1, 2, 3 or 4 bytes.  The valid strings
     * are as follows (in "bit format"):
     *    0xxxxxxx                                      valid 1-byte
     *    110xxxxx 10xxxxxx                             valid 2-byte
     *    1110xxxx 10xxxxxx 10xxxxxx                    valid 3-byte
     *    11110xxx 10xxxxxx 10xxxxxx 10xxxxxx           valid 4-byte
*/

boolean
swish_is_ascii(
    xmlChar *str
)
{
    int i;
    int len = xmlStrlen(str);

    if (!len || str == NULL)
        return 0;

    for (i = 0; i < len; i++) {
        if (str[i] >= 0x80)
            return 0;

    }
    return 1;
}

char*
swish_get_locale(
)
{
    char *locale;
    
    /* all C programs start with C locale, so initialize with LC_ALL */
    setlocale(LC_ALL, "");
    locale = setlocale(LC_ALL, "");
    if (locale == NULL || !strlen(locale)) {
        //SWISH_DEBUG_MSG("locale for LC_ALL was null");

/* use LC_CTYPE specifically: 
 * http://mail.nl.linux.org/linux-utf8/2001-09/msg00030.html 
 */
        locale = setlocale(LC_CTYPE, "");
        if (locale == NULL || !strlen(locale)) {
            //SWISH_DEBUG_MSG("locale for LC_CTYPE was null");
            locale = getenv("LANG");
            if (locale == NULL || !strlen(locale)) {
                //SWISH_DEBUG_MSG("getenv for LANG was null");
                locale = SWISH_LOCALE;
            }
        }
    }
    return locale;
}

void
swish_verify_utf8_locale(
)
{
    char *loc;
    const xmlChar *enc;

/* a bit about encodings: libxml2 takes whatever encoding the input XML is
 * (latin1, ascii, utf8, etc) and standardizes it using iconv (or other) in xmlChar as
 * UTF-8. However, we must ensure we have UTF-8 locale because all the mb* and wc*
 * routines rely on the locale to correctly interpret chars. 
 */

    loc = swish_get_locale(); 
    enc = xmlStrchr((xmlChar *)loc, (xmlChar)'.');

    if (enc != NULL) {
        enc++;
        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
            SWISH_DEBUG_MSG("encoding = %s", enc);
    }
    else {
        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
            SWISH_DEBUG_MSG("no encoding in %s, using %s", loc, SWISH_DEFAULT_ENCODING);

        enc = (xmlChar *)SWISH_DEFAULT_ENCODING;
    }

    setenv("SWISH_ENCODING", (char *)enc, 0);   /* remember in env var, if not already set */

    if (!loc) {
        SWISH_WARN("can't get locale via setlocale()");
    }
    else if (SWISH_DEBUG) {
        SWISH_DEBUG_MSG("current locale and encoding: %s %s", loc, enc);
    }

    if (u8_is_locale_utf8(loc)) {
/* a-ok */

        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
            SWISH_DEBUG_MSG("locale looks like UTF-8");

    }
    else {
/* must be UTF-8 charset since libxml2 converts everything to UTF-8 */
        if (SWISH_DEBUG)
            SWISH_DEBUG_MSG
                ("Your locale (%s) was not UTF-8 so internally we are using %s", loc,
                 SWISH_LOCALE);

        if (!setlocale(LC_CTYPE, SWISH_LOCALE)) {
            SWISH_WARN("failed to set locale to %s", SWISH_LOCALE);
        }

    }

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER) 
        SWISH_DEBUG_MSG("active locale is %s", setlocale(LC_CTYPE, NULL));

}

xmlChar *
swish_str_escape_utf8(
    xmlChar *u8str
)
{
    xmlChar *escaped;
    int u8chrs, n_escaped, esc_len;
    
    u8chrs = swish_utf8_num_chrs(u8str);
    
    /* 10 is the max number of ascii chars needed to represent a utf8 chr:
     * \Uxxxxxxxx
     * 1234567890
     */
    esc_len = (10*u8chrs)+1;    /* +1 == nul */
    escaped = swish_xmalloc(esc_len);
    
    /*
    SWISH_DEBUG_MSG("escaping %s len %d for '%s'",
        escaped, esc_len, u8str);
    */    
    n_escaped = u8_escape((char*)escaped, esc_len, (char*)u8str, 0); // TODO quotes?
    
    return escaped;
}

xmlChar *
swish_str_unescape_utf8(
    xmlChar *ascii
)
{
    xmlChar *unescaped;
    int n_unescaped, ascii_len;
    
    ascii_len = xmlStrlen(ascii);        
    unescaped = swish_xmalloc(ascii_len+1);
    n_unescaped = u8_unescape((char*)unescaped, ascii_len+1, (char*)ascii);
    
    return unescaped;
}


/* based on swstring.c  */

int
swish_wchar_t_comp(
    const void *s1,
    const void *s2
)
{
    return (*(wchar_t *) s1 - *(wchar_t *) s2);
}

/* Sort a string */
int
swish_sort_wchar(
    wchar_t * s
)
{
    int i, j, len;
    i = 0;
    j = 0;
    len = wcslen(s);
    qsort(s, len, sizeof(wchar_t), &swish_wchar_t_comp);

/* printf("sorted array s is %d long\n", len); */

    for (i = 0; s[i] != 0; i++)
/* printf("%d = %lc (%d)\n", i, s[i], s[i]); */

        for (i = 1, j = 1; i < (len - 1); i++) {
            if (s[i] != s[j - 1]) {
                s[j++] = s[i];
/* printf("%d item is %lc (%d)\n", j, s[j], s[j]); */
            }
        }

    return s[j];

}

/* based on swstring.c in Swish-e but handles wide char strings instead */

wchar_t *
swish_wstr_tolower(
    wchar_t * s
)
{
    wchar_t *p = (wchar_t *) s;
    while (*p) {
        *p = (wchar_t) towlower(*p);
        p++;
    }
    return s;
}

/* convert a string to lowercase.
 * returns a new malloc'd string, so should be freed eventually
*/
xmlChar *
swish_str_tolower(
    xmlChar *s
)
{

    if (swish_is_ascii(s))
        return swish_ascii_str_tolower(s);
    else
        return swish_utf8_str_tolower(s);

}

/* convert utf8 to wchar,
   lowercase the wchar,
   then convert back to utf8
   and free the wchar
*/
xmlChar *
swish_utf8_str_tolower(
    xmlChar *s
)
{
    xmlChar *str;
    wchar_t *wstr;

/* convert mb to wide -- must free */
    wstr = swish_locale_to_wchar(s);

/* convert wide tolower */
    swish_wstr_tolower(wstr);

/* convert wide back to mb */
    str = swish_wchar_to_locale(wstr);

    swish_xfree(wstr);

    return str;
}

/* based on swstring.c in Swish-e */
xmlChar *
swish_ascii_str_tolower(
    xmlChar *s
)
{
    xmlChar *copy = swish_xstrdup(s);
    xmlChar *p = copy;
    while (*p) {
        *p = tolower(*p);
        p++;
    }
    return copy;
}

/*
  -- Skip white spaces...
  -- position to non space character
  -- return: ptr. to non space char or \0
  -- 2001-01-30  rasc

  TODO make utf8 safe. 
*/

xmlChar *
swish_str_skip_ws(
    xmlChar *s
)
{
    while (*s && isspace((int)(xmlChar)*s))
        s++;
    return s;
}

/*************************************
* Trim trailing white space
* Returns void
**************************************/

// TODO make utf8 safe
void
swish_str_trim_ws(
    xmlChar *s
)
{
    int i = xmlStrlen(s);

    while (i && isspace((int)s[i - 1]))
        s[--i] = '\0';
}

boolean
swish_str_all_ws(
    xmlChar *s
)
{
    return swish_str_all_ws_len(s, xmlStrlen(s));
}

boolean
swish_str_all_ws_len(
    xmlChar * s, 
    int len
)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!isspace((int)s[i])) {
            return 0;
        }
    }
    return 1;
}

/* change all ascii controll chars < 32 to space */
void
swish_str_ctrl_to_ws(
    xmlChar *s
)
{
    int i, k;
    if (!swish_is_ascii(s)) // TODO utf8-safe
        return;
        
    i = xmlStrlen(s);
    for(k=0; k<i; k++) {
        if ((int)s[k] < 32)
            s[k] = SWISH_SPACE;
    }
}

void
swish_debug_wchars(
    const wchar_t * widechars
)
{
    int i;
    for (i = 0; widechars[i] != 0; i++) {
        printf(" >%lc< %ld %#lx \n", (wint_t) widechars[i], (long int)widechars[i],
               (long unsigned int)widechars[i]);
    }
}

/* returns the number of UTF-8 char* needed to hold the codepoint
   represented by 'ch'.
   similar to swish_utf8_chr_len() except that the arg is already
   a 4-byte container and we want to know how many of the 4 bytes
   we really need.
*/
int
swish_bytes_in_wchar(
    int ch
)
{
    int len = 0;

    if (ch < 0x80) {
        len = 1;
    }
    if (ch < 0x800) {
        len = 2;
    }
    if (ch < 0x10000) {
        len = 3;
    }
    if (ch < 0x110000) {
        len = 4;
    }

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG(" %lc is %d bytes long", ch, len);

    return len;
}


/* from http://www.triptico.com/software/unicode.html */
wchar_t *
swish_locale_to_wchar(
    xmlChar *str
)
{
    wchar_t *ptr;
    size_t s;
    int len;
    char *loc;

/* first arg == 0 means 'calculate needed space' */
    s = mbstowcs(0, (const char *)str, 0);

    len = mblen((const char *)str, 4);

/* a size of -1 is triggered by an error in encoding; 
 * never happen in ISO-8859-* locales, but possible in UTF-8 
 */
    if (s == -1) {
        loc = swish_get_locale();
        SWISH_CROAK("error converting mbs to wide str under locale %s : %s", 
            loc, str);
    }


/* malloc the necessary space */
    ptr = swish_xmalloc((s + 1) * sizeof(wchar_t));

/* really do it */
    s = mbstowcs(ptr, (const char *)str, s);

/* ensure NUL termination */
    ptr[s] = '\0';

/* remember to free() ptr when done */
    return (ptr);
}

/* from http://www.triptico.com/software/unicode.html */
xmlChar *
swish_wchar_to_locale(
    wchar_t * str
)
{
    xmlChar *ptr;
    size_t s;

/* first arg == 0 means 'calculate needed space' */
    s = wcstombs(0, str, 0);

/* a size of -1 means there are characters that could not be converted to current
     * locale */
    if (s == -1)
        SWISH_CROAK("error converting wide chars to mbs: %ls", str);

/* malloc the necessary space */
    ptr = (xmlChar *)swish_xmalloc(s + 1);

/* really do it */
    s = wcstombs((char *)ptr, (const wchar_t *)str, s);

/* ensure NUL termination */
    ptr[s] = '\0';

/* remember to free() ptr when done */
    return (ptr);
}

/* StringList functions derived from swish-e vers 2 */
swish_StringList *
swish_stringlist_init(
)
{
    swish_StringList *sl = swish_xmalloc(sizeof(swish_StringList));
    sl->n = 0;
    sl->max = 2; /* 2 to allow for NUL-terminate */
    sl->word = swish_xmalloc(sl->max * sizeof(xmlChar *));
    return sl;
}

void
swish_stringlist_free(
    swish_StringList * sl
)
{
    while (sl->n)
        swish_xfree(sl->word[--sl->n]);

    swish_xfree(sl->word);
    swish_xfree(sl);
}

void
swish_stringlist_merge(
    swish_StringList * sl1,
    swish_StringList * sl2
)
{
    int i;
    // add sl1 -> sl2
    sl2->word =
        (xmlChar **)swish_xrealloc(sl2->word, (sl1->n + sl2->n) * sizeof(xmlChar *) + 1);
    for (i = 0; i < sl1->n; i++) {
        // copy is a little overhead, but keeps mem count simple
        sl2->word[sl2->n++] = swish_xstrdup(sl1->word[i]);
    }
    swish_stringlist_free(sl1);
}

swish_StringList *
swish_stringlist_copy(
    swish_StringList * sl
)
{
    swish_StringList *s2;
    int i;
    s2 = swish_stringlist_init();
    s2->word = (xmlChar **)swish_xrealloc(s2->word, sl->n * sizeof(xmlChar *) + 1);
    for (i = 0; i < sl->n; i++) {
        s2->word[i] = swish_xstrdup(sl->word[i]);
    }
    s2->n = sl->n;
    return s2;
}

void
swish_stringlist_debug(
    swish_StringList *sl
)
{
    int i;
    for (i=0; i<sl->n; i++) {
        SWISH_DEBUG_MSG("[%d] %s", i, sl->word[i]);
    }
}

swish_StringList *  
swish_stringlist_parse_sort_string(
    xmlChar *sort_string,
    swish_Config *cfg
)
{
    xmlChar *sort_string_lc, *prop, *dir, *normalized;
    swish_StringList *sl;
    int i, nlen;
    
    /*  normalize so we know we are comparing ASC vs asc 
     *  and since propertynames are always lowercased.
     */
    sort_string_lc = swish_str_tolower(sort_string);
    sl = swish_stringlist_build(sort_string);
    swish_xfree(sort_string_lc);
    
    /* 2x longer should be ample */
    nlen = 2*xmlStrlen(sort_string);
    normalized = swish_xmalloc(nlen);
    normalized[0] = '\0';
    
    /* create the normalized string */
    for (i=0; i < sl->n; i++) {
        prop = sl->word[i]; /* just for code clarity */
        if (cfg) {
            swish_property_get_id(prop, cfg->properties); /* will croak if invalid */
        }
        if (i < sl->n) {
            dir = sl->word[i+1];
        }
        else {
            dir = NULL;
        }
        normalized = xmlStrncat(normalized, BAD_CAST " ", 1);
        normalized = xmlStrncat(normalized, prop, xmlStrlen(prop));
        normalized = xmlStrncat(normalized, BAD_CAST " ", 1);
        if (xmlStrEqual(dir, BAD_CAST "asc")
            ||
            xmlStrEqual(dir, BAD_CAST "desc")
        ) {
            normalized = xmlStrncat(normalized, dir, xmlStrlen(dir));
            i++;    /* bump to next prop */
        }
        else {
            normalized = xmlStrncat(normalized, BAD_CAST "asc", 3);
        }
    }
    swish_stringlist_free(sl);
    sl = swish_stringlist_build(normalized);
    swish_xfree(normalized);
    return sl;
}

swish_StringList *
swish_stringlist_build(
    xmlChar *line
)
{
    swish_StringList *sl;
    xmlChar *p;

    if (!line)
        return (NULL);

    sl = swish_stringlist_init();
    p = (xmlChar *)strchr((const char *)line, '\n');
    if (p != NULL)
        *p = '\0';

    p = line;

    while ((p = getword(&line))) {
    
/* getword returns "" when not null, 
 * so need to free it if we are not using it 
 */
        if (!*p) {
            swish_xfree(p);
            break;
        }
        
        swish_stringlist_add_string(sl, (xmlChar*)p);
    }

/* Add an extra NUL */
    if (sl->n == sl->max) {
        sl->word =
            (xmlChar **)swish_xrealloc(sl->word, (sl->max += 1) * sizeof(xmlChar *));
    }

    sl->word[sl->n] = NULL;

    return sl;
}

unsigned int
swish_stringlist_add_string(
    swish_StringList *sl,
    xmlChar *str
)
{
    if (sl->n == sl->max) {
        sl->word = (xmlChar **)swish_xrealloc(sl->word, (sl->max *= 2) * sizeof(xmlChar *));
    }

    sl->word[sl->n++] = str;
    return sl->n;
}

/* Gets the next word in a line. If the word's in quotes,
 * include blank spaces in the word or phrase.
 * should be utf-8 compatible; only pitfall would be if a continuation byte
 * returns true for isspace().
*/

static xmlChar *
getword(
    xmlChar **in_buf
)
{
    xmlChar quotechar;
    xmlChar uc;
    xmlChar *s = *in_buf;
    xmlChar *start = *in_buf;
    xmlChar buf[SWISH_MAX_WORD_LEN + 1];
    xmlChar *cur_char = buf;
    int backslash = 0;

    quotechar = '\0';

    s = swish_str_skip_ws(s);

/* anything to read? */
    if (!*s) {
        *in_buf = s;
        return swish_xstrdup((xmlChar *)"\0");
    }

    if (*s == '\"' || *s == '\'')
        quotechar = *s++;

/* find end of "more words" or word */

    while (*s) {
        uc = (xmlChar)*s;

        if (uc == '\\' && !backslash && quotechar)
/* only enable backslash
         * inside of quotes */
        {
            s++;
            backslash++;
            continue;
        }

/* Can't see why we would need to escape these, can you? - always fed a
         * single line */
        if (uc == '\n' || uc == '\r') {
            s++;
            break;
        }

        if (!backslash) {
/* break on ending quote or unquoted space */

            if (uc == quotechar || (!quotechar && isspace((int)uc))) {
                s++;            /* past quote or space char. */
                break;
            }

        }
        else {
            backslash = 0;
        }

        *cur_char++ = *s++;

        if (cur_char - buf > SWISH_MAX_WORD_LEN) {
            SWISH_WARN("Parsed word '%s' exceeded max length of %d", start,
                       SWISH_MAX_WORD_LEN);
        }

    }

    if (backslash)
        *cur_char++ = '\\';

    *cur_char = '\0';

    *in_buf = s;

    return swish_xstrdup(buf);

}

/*
 * based on charDecode_C_Escape() in swstring.c in swish-e
 */

char    
swish_get_C_escaped_char(xmlChar *s, xmlChar **se)
{
    char    c,
           *se2;

    if (*s != '\\') {
        /* no escape   */
        c = *s;                 /* return same char */

    }
    else {

        switch (*(++s))
        {                       /* can be optimized ... */
        case 'a':
            c = '\a';
            break;
        case 'b':
            c = '\b';
            break;
        case 'f':
            c = '\f';
            break;
        case 'n':
            c = '\n';
            break;
        case 'r':
            c = '\r';
            break;
        case 't':
            c = '\t';
            break;
        case 'v':
            c = '\v';
            break;

        // TODO support full UTF-8
        case 'x':              /* Hex  \xff  */
            c = (char) strtoul((char*)++s, &se2, 16);
            s = (xmlChar*)--se2;
            break;

        case '0':              /* Oct  \0,  \012 */
            c = (char) strtoul((char*)s, &se2, 8);
            s = (xmlChar*)--se2;
            break;

        case '\0':             /* outch!! null after \ */
            s--;               /* it's a "\"    */
            
        default:
            c = *s;            /* the escaped character */
            break;
        }

    }

    if (se)
        *se = s + 1;
    return c;
}
