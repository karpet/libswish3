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

extern int SWISH_DEBUG;

static xmlChar *getword(
    xmlChar **in_buf
);
static xmlChar *findlast(
    xmlChar *str,
    xmlChar *set
);
static xmlChar *lastptr(
    xmlChar *str
);

/* originally based on libutf8; this version (and other u8_* functions)
   are from http://cprogramming.com/tutorial/unicode.html
 */
static int
u8_is_locale_utf8(
    char *locale
);

/* move to next character */
static void u8_inc(
    char *s,
    int *i
);

/* move to previous character */
static void u8_dec(
    char *s,
    int *i
);


/* is c the start of a utf8 sequence? */
#define isutf(c) (((c)&0xC0)!=0x80)

static void
u8_inc(
    char *s,
    int *i
)
{
    (void)(isutf(s[++(*i)]) || isutf(s[++(*i)]) || isutf(s[++(*i)]) || ++(*i));
}

static void
u8_dec(
    char *s,
    int *i
)
{
    (void)(isutf(s[--(*i)]) || isutf(s[--(*i)]) || isutf(s[--(*i)]) || --(*i));
}


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

/* TODO need these ??
inline uint32_t
binary_string_to_int(
    const std::string & s
)
{
    if (s.size() != 4)
        return (uint32_t) - 1;
    uint32_t
        v;
    memcpy(&v, s.data(), 4);
    return ntohl(v);
}

inline
    std::string
int_to_binary_string(
    uint32_t v
)
{
    v = htonl(v);
    return std::string(reinterpret_cast < const char *>(&v), 4);
}

*/

/* returns the UCS32 value for a UTF8 string -- the character's Unicode value.
   see http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&item_id=IWS-AppendixA
*/
int
swish_utf8_codepoint(
    xmlChar *utf8
)
{
    int len;
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

static int
u8_is_locale_utf8(
    char *locale
)
{
    // this code based on libutf8 
    const char *cp = locale;

    for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++) {
        if (*cp == '.') {
            const char *encoding = ++cp;
            for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++);
            if ((cp - encoding == 5 && !strncmp(encoding, "UTF-8", 5))
                || (cp - encoding == 4 && !strncmp(encoding, "utf8", 4)))
                return 1;       // it's UTF-8
            break;
        }
    }
    return 0;
}


void
swish_verify_utf8_locale(
)
{
    char *loc;
    const xmlChar *enc;

/* a bit about encodings: libxml2 takes whatever encoding the input XML is
 * (latin1, ascii, utf8, etc) and standardizes it using iconv in xmlChar as
 * UTF-8. However, we must ensure we have UTF-8 locale because all the mb* and wc*
 * routines rely on the locale to correctly interpret chars. 
 */

/* use LC_CTYPE specifically: http://mail.nl.linux.org/linux-utf8/2001-09/msg00030.html */

    loc = setlocale(LC_CTYPE, NULL);

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
        SWISH_DEBUG_MSG("locale set to %s", setlocale(LC_CTYPE, NULL));

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
        loc = setlocale(LC_CTYPE, NULL);
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
swish_init_stringlist(
)
{
    swish_StringList *sl = swish_xmalloc(sizeof(swish_StringList));
    sl->n = 0;
    sl->word = swish_xmalloc(2 * sizeof(xmlChar *));
/* 2 to allow for NUL-terminate */
    return sl;
}

void
swish_free_stringlist(
    swish_StringList * sl
)
{
    while (sl->n)
        swish_xfree(sl->word[--sl->n]);

    swish_xfree(sl->word);
    swish_xfree(sl);
}

void
swish_merge_stringlists(
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
    swish_free_stringlist(sl1);
}

swish_StringList *
swish_copy_stringlist(
    swish_StringList * sl
)
{
    swish_StringList *s2;
    int i;
    s2 = swish_init_stringlist();
    s2->word = (xmlChar **)swish_xrealloc(s2->word, sl->n * sizeof(xmlChar *) + 1);
    for (i = 0; i < sl->n; i++) {
        s2->word[i] = swish_xstrdup(sl->word[i]);
    }
    s2->n = sl->n;
    return s2;
}

swish_StringList *
swish_make_stringlist(
    xmlChar *line
)
{
    swish_StringList *sl;
    int cursize, maxsize;
    xmlChar *p;

    if (!line)
        return (NULL);

    sl = swish_init_stringlist();
    p = (xmlChar *)strchr((const char *)line, '\n');
    if (p != NULL)
        *p = '\0';

    cursize = 0;
    maxsize = 2;

    p = line;

/*    while (&line && (p = getword(&line))) {  */
    while ((p = getword(&line))) {  
/* getword returns "" when not null, so need to free it if we are not using it */
        if (!*p) {
            swish_xfree(p);
            break;
        }

        if (cursize == maxsize) {
            sl->word =
                (xmlChar **)swish_xrealloc(sl->word, (maxsize *= 2) * sizeof(xmlChar *));
        }

        sl->word[cursize++] = (xmlChar *)p;
    }
    sl->n = cursize;

/* Add an extra NUL */
    if (cursize == maxsize) {
        sl->word =
            (xmlChar **)swish_xrealloc(sl->word, (maxsize += 1) * sizeof(xmlChar *));
    }

    sl->word[cursize] = NULL;

    return sl;
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

/* parse a URL to determine file ext */
/* inspired by http://www.tug.org/tex-archive/tools/zoo/ by Rahul Dhesi */
xmlChar *
swish_get_file_ext(
    xmlChar *url
)
{
    xmlChar *p;

/*    if (strlen(url) < 3)
        return url;
*/

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("parsing url %s for extension", url);

    p = findlast(url, (xmlChar *)SWISH_EXT_SEP);        /* look for . or /         */

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("p = %s", p);

    if (p == NULL)
        return p;

    if (p != NULL && *p != SWISH_EXT_CH)        /* found .?                     */
        return NULL;            /* ... if not, ignore / */

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("p = %s", p);

    if (*p == SWISH_EXT_CH)
        p++;                    /* skip to next char after . */

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("ext is %s", p);

    return swish_str_tolower(p);
}

/*******************/
/*
findlast() finds last occurrence in provided string of any of the characters
except the null character in the provided set.

If found, return value is pointer to character found, else it is NULL.
*/

static xmlChar *
findlast(
    xmlChar *str,
    xmlChar *set
)
{
    xmlChar *p;

    if (str == NULL || set == NULL || *str == '\0' || *set == '\0')
        return (NULL);

    p = lastptr(str);           /* pointer to last char of string */
    assert(p != NULL);

    while (p != str && xmlStrchr(set, *p) == NULL) {
        --p;
    }

/* either p == str or we found a character or both */
    if (xmlStrchr(set, *p) == NULL)
        return (NULL);
    else
        return (p);
}

/*
lastptr() returns a pointer to the last non-null character in the string, if
any.  If the string is null it returns NULL
*/

static xmlChar *
lastptr(
    xmlChar *str
)
{
    xmlChar *p;
    if (str == NULL)
        SWISH_CROAK("received null pointer while looking for last NULL");
    if (*str == '\0')
        return (NULL);
    p = str;
    while (*p != '\0')          /* find trailing null char */
        ++p;
    --p;                        /* point to just before it */
    return (p);
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
