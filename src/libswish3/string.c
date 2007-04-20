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
#include <libxml/hash.h>
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <err.h>

#include "libswish3.h"
#include "utf8.c"

extern int      SWISH_DEBUG;


static xmlChar *getword(xmlChar ** in_buf);
static xmlChar *utf8_str_tolower( xmlChar *s );
static xmlChar *ascii_str_tolower( xmlChar *s );
static xmlChar *findlast ( xmlChar *str, xmlChar *set);
static xmlChar *lastptr  ( xmlChar *str );

/* returns length of a UTF8 character, based on first byte (see below) */
int swish_utf8_chr_len( xmlChar *utf8 )
{
    return u8_seqlen((char*)utf8);
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

int swish_is_ascii( xmlChar *str )
{
    int i;
    int len = xmlStrlen(str);
    
    if(!len || str == NULL)
        return 0;
        
    for (i=0; i < len; i++)
    {
        if (str[i] >= 0x80)
            return 0;
            
    }
    return 1;
}


void
swish_verify_utf8_locale()
{
    char          * loc;
    const xmlChar * enc;
    
    /* a bit about encodings: libxml2 takes whatever encoding the input XML is
     * (latin1, ascii, utf8, etc) and standardizes it using iconv in xmlChar as
     * UTF-8. However, we must ensure we have UTF-8 locale because all the mb* and wc*
     * routines rely on the locale to correctly interpret chars. */


    /* use LC_CTYPE specifically: http://mail.nl.linux.org/linux-utf8/2001-09/msg00030.html */
    
    loc = setlocale(LC_CTYPE, "");
    
    enc = xmlStrchr((xmlChar*)loc,(xmlChar)'.');
    
    if (enc != NULL)
    {
        enc++;
        if (SWISH_DEBUG)
            swish_debug_msg("encoding = %s", enc);
    }
    else
    {
        if (SWISH_DEBUG)
            swish_debug_msg("no encoding in %s, using %s", loc, SWISH_DEFAULT_ENCODING);
            
        enc = SWISH_DEFAULT_ENCODING;
    }
    
    setenv("SWISH_ENCODING", enc, 0);  /* remember in env var, if not already set */
    
    if(!loc)
    {
        swish_warn_err("can't get locale via setlocale()");
    }

    if (u8_is_locale_utf8(loc))
    {
        /* a-ok */

    }
    else
    {
        /* must be UTF-8 charset since libxml2 converts everything to UTF-8 */
        if (SWISH_DEBUG)
            swish_debug_msg("Your locale (%s) was not UTF-8 so internally we are using %s",
                     loc, SWISH_LOCALE);
                     
        setlocale(LC_CTYPE, SWISH_LOCALE);

    }

    if (SWISH_DEBUG)
        swish_debug_msg("locale set to %s", loc);

}


/* based on swstring.c  */

int 
swish_wchar_t_comp(const void *s1, const void *s2)
{
    return (*(wchar_t *) s1 - *(wchar_t *) s2);
}

/* Sort a string */
int 
swish_sort_wchar(wchar_t * s)
{
    int             i, j, len;

    len = wcslen(s);
    qsort(s, len, sizeof(wchar_t), &swish_wchar_t_comp);

    /* printf("sorted array s is %d long\n", len); */

    for (i = 0; s[i] != 0; i++)
        /* printf("%d = %lc (%d)\n", i, s[i], s[i]); */

        for (i = 1, j = 1; i < (len - 1); i++)
        {
            if (s[i] != s[j - 1])
            {
                s[j++] = s[i];
                /* printf("%d item is %lc (%d)\n", j, s[j], s[j]); */
            }
        }

    return s[j];

}

/* based on swstring.c in Swish-e but handles wide char strings instead */

wchar_t        *
swish_wstr_tolower(wchar_t * s)
{
    wchar_t        *p = (wchar_t *) s;
    while (*p)
    {
        *p = (wchar_t) towlower(*p);
        p++;
    }
    return s;
}

/* convert a string to lowercase.
 * returns a new malloc'd string, so should be freed eventually 
 */
xmlChar        *
swish_str_tolower(xmlChar * s)
{

    if (swish_is_ascii(s))
        return ascii_str_tolower(s);
    else
        return utf8_str_tolower(s);

}

/* convert utf8 to wchar,
   lowercase the wchar,
   then convert back to utf8
   and free the wchar
 */
static xmlChar *
utf8_str_tolower( xmlChar *s )
{
    xmlChar        *str;
    wchar_t        *wstr;

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
static xmlChar *
ascii_str_tolower( xmlChar *s )
{
    xmlChar *copy = swish_xstrdup(s);
    xmlChar *p    = copy;
    while (*p)
    {    
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

  should be utf8 safe, unless a continuation byte evals true to isspace()
*/

xmlChar        *
swish_str_skip_ws(xmlChar * s)
{
    while (*s && isspace((int) (xmlChar) * s))
        s++;
    return s;
}

/*************************************
* Trim trailing white space
* Returns void
**************************************/

void 
swish_str_trim_ws(xmlChar * s)
{
    int  i = xmlStrlen(s);

    while (i && isspace((int) s[i - 1]))
        s[--i] = '\0';
}


void 
swish_debug_wchars(const wchar_t * widechars)
{
    int             i;
    for (i = 0; widechars[i] != 0; i++)
    {
        printf(" >%lc< %ld %#lx \n",
         (wint_t) widechars[i],
         (long int) widechars[i],
         (long unsigned int) widechars[i]);
    }
}

/* from http://www.triptico.com/software/unicode.html */
wchar_t * swish_locale_to_wchar(xmlChar * str)
{
    wchar_t        *ptr;
    size_t          s;
    int             len;
    
    /* first arg == 0 means 'calculate needed space' */
    s = mbstowcs(0, (const char *)str, 0);

    len = mblen((const char *)str, 4);

    /* a size of -1 is triggered by an error in encoding; never happen in ISO-8859-*
     * locales, but possible in UTF-8 */
    if (s == -1)
    {
        swish_warn_err("error converting mbs to wide str: %s", str);
        return (0);
    }

    /* malloc the necessary space */
    ptr = swish_xmalloc((s + 1) * sizeof(wchar_t));

    /* really do it */
    s = mbstowcs(ptr, (const char *)str, s);

    /* ensure 0-termination */
    ptr[s] = NULL;

    /* remember to free() ptr when done */
    return (ptr);
}


/* from http://www.triptico.com/software/unicode.html */
xmlChar * swish_wchar_to_locale(wchar_t * str)
{
    xmlChar        *ptr;
    size_t          s;

    /* first arg == 0 means 'calculate needed space' */
    s = wcstombs(0, str, 0);

    /* a size of -1 means there are characters that could not be converted to current
     * locale */
    if (s == -1)
    {
        warn("error converting wide chars to mbs: %ls", str);
        return (0);
    }

    /* malloc the necessary space */
    ptr = (xmlChar *) swish_xmalloc(s + 1);

    /* really do it */
    s = wcstombs((char *)ptr, (const wchar_t *)str, s);

    /* ensure 0-termination */
    ptr[s] = NULL;

    /* remember to free() ptr when done */
    return (ptr);
}


/* StringList functions derived from swish-e vers 2 */
swish_StringList *
swish_init_StringList()
{
    swish_StringList *sl = swish_xmalloc(sizeof(swish_StringList));
    sl->n = 0;
    sl->word = swish_xmalloc(2 * sizeof(xmlChar *));    
    /* 2 to allow for
     * NULL-terminate 
     */
    return sl;
}

void 
swish_free_StringList(swish_StringList * sl)
{
    while (sl->n)
        swish_xfree(sl->word[--sl->n]);

    swish_xfree(sl->word);
    swish_xfree(sl);
}



swish_StringList *
swish_make_StringList(xmlChar * line)
{
    swish_StringList   *sl = swish_init_StringList();
    int                  cursize, maxsize;
    xmlChar             *p;

    if (!line)
        return (NULL);

    p = (xmlChar*)strchr((const char*)line,'\n');
    if (p != NULL)
        *p = '\0';

    cursize = 0;
    maxsize = 2;

    p = line;

    while (&line && (p = getword(&line)))
    {
        /* getword returns "" when not null, so need to free it if we are not
         * using it */
        if (!*p)
        {
            swish_xfree(p);
            break;
        }

        if (cursize == maxsize)
        {
            sl->word = (xmlChar **) swish_xrealloc(sl->word, (maxsize *= 2) * sizeof(xmlChar *));
        }

        sl->word[cursize++] = (xmlChar *) p;
    }
    sl->n = cursize;

    /* Add an extra NULL */
    if (cursize == maxsize)
    {
        sl->word = (xmlChar **) swish_xrealloc(sl->word, (maxsize += 1) * sizeof(xmlChar *));
    }

    sl->word[cursize] = NULL;

    return sl;
}

/* Gets the next word in a line. If the word's in quotes, 
 * include blank spaces in the word or phrase.
 * should be utf-8 compatible; only pitfall would be if a continuation byte
 * returns true for isspace().
 */

static
xmlChar        *
getword(xmlChar ** in_buf)
{
    xmlChar         quotechar;
    xmlChar         uc;
    xmlChar        *s = *in_buf;
    xmlChar        *start = *in_buf;
    xmlChar         buf[SWISH_MAX_WORD_LEN + 1];
    xmlChar        *cur_char = buf;
    int             backslash = 0;

    quotechar = '\0';

    s = swish_str_skip_ws(s);

    /* anything to read? */
    if (!*s)
    {
        *in_buf = s;
        return swish_xstrdup((xmlChar*)"\0");
    }


    if (*s == '\"' || *s == '\'')
        quotechar = *s++;

    /* find end of "more words" or word */

    while (*s)
    {
        uc = (xmlChar) * s;

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
        if (uc == '\n' || uc == '\r')
        {
            s++;
            break;
        }


        if (!backslash)
        {
            /* break on ending quote or unquoted space */

            if (uc == quotechar || (!quotechar && isspace((int) uc)))
            {
                s++;    /* past quote or space char. */
                break;
            }

        }
        else
        {
            backslash = 0;
        }

        *cur_char++ = *s++;

        if (cur_char - buf > SWISH_MAX_WORD_LEN)
        {
            swish_warn_err("Parsed word '%s' exceeded max length of %d", 
                             start, SWISH_MAX_WORD_LEN);
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
xmlChar * swish_get_file_ext( xmlChar *url )
{
    xmlChar *p;
    
/*    if (strlen(url) < 3)
        return url;
*/    
    
    if ( SWISH_DEBUG > 10)
        swish_debug_msg("parsing url %s for extension", url );
    
    p = findlast (url, (xmlChar *)SWISH_EXT_SEP);	    /* look for . or /		*/
    
    if ( SWISH_DEBUG > 10)
        swish_debug_msg("p = %s", p );
    
    if (p == NULL)
        return p;
    
    if (p != NULL && *p != SWISH_EXT_CH)	/* found .?					*/
	    return NULL;							/* ... if not, ignore / */

    if ( SWISH_DEBUG > 10)
        swish_debug_msg("p = %s", p );
    
    if ( *p == SWISH_EXT_CH )
        p++; /* skip to next char after . */

    if ( SWISH_DEBUG > 10)
        swish_debug_msg("ext is %s", p );

    return swish_str_tolower( p );
}


/*******************/
/* 
findlast() finds last occurrence in provided string of any of the characters
except the null character in the provided set.

If found, return value is pointer to character found, else it is NULL.
*/

static xmlChar *findlast ( xmlChar *str, xmlChar *set)
{
   xmlChar *p;

   if (str == NULL || set == NULL || *str == '\0' || *set == '\0')
      return (NULL);

   p = lastptr (str);   /* pointer to last char of string */
   assert(p != NULL);

   while (p != str && xmlStrchr (set, *p) == NULL) {
      --p;
   }                 

   /* either p == str or we found a character or both */
   if (xmlStrchr (set, *p) == NULL)
      return (NULL);
   else
      return (p);
}

/*
lastptr() returns a pointer to the last non-null character in the string, if
any.  If the string is null it returns NULL
*/

static xmlChar *lastptr ( xmlChar *str )
{
   xmlChar *p;
   if (str == NULL)
      swish_fatal_err("received null pointer while looking for last NULL");
   if (*str == '\0')
      return (NULL);
   p = str;
   while (*p != '\0')            /* find trailing null char */
      ++p;
   --p;                          /* point to just before it */
   return (p);
}
