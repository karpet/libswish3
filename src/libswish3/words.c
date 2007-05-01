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


/* word tokenizer(s) */

#include <libxml/hash.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <err.h>
#include <stdarg.h>

#include "libswish3.h"

extern int      SWISH_DEBUG;
static int      strip_ascii_chars(xmlChar * word, int len);
static int      strip_wide_chars(wchar_t * word, int len);
static int      is_ignore_start_ascii(char c);
static int      is_ignore_end_ascii(char c);
static int      is_ignore_word_ascii(char c);
static int      is_ignore_start(wint_t c);
static int      is_ignore_end(wint_t c);
static int      is_ignore_word(wint_t c);
static int      bytes_in_chr(wint_t c);
static void     make_ascii_tables();
static int      add_to_wordlist(
                        swish_WordList * list,
                        xmlChar * word,
                        int len,
                        xmlChar * metaname,
                        xmlChar * context,
                        int word_pos,
                        int offset
                );

static int initialized = 0;

void
swish_init_words()
{
    if (initialized)
        return;
        
    make_ascii_tables();
    initialized = 1;
}

/* TODO use xmlList functions instead or perhaps array like StringList */


swish_WordList *
swish_init_wordlist()
{
    swish_WordList *wl = (swish_WordList *) swish_xmalloc(sizeof(swish_WordList));
    wl->head    = NULL;
    wl->tail    = NULL;
    wl->current = NULL;
    wl->nwords  = 0;
    wl->ref_cnt = 0;
    return wl;
}

void
swish_free_wordlist(swish_WordList * list)
{
    swish_Word    *t;

    if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
        swish_debug_msg("freeing swish_WordList");

    /* free each item, then the list itself */
    list->current = list->head;
    while (list->current != NULL)
    {
        if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
            swish_debug_msg("free metaname: %s", list->current->metaname);
            
        swish_xfree(list->current->metaname);
        
        if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
            swish_debug_msg("free context: %s", list->current->context);
            
        swish_xfree(list->current->context);
        
        if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
            swish_debug_msg("free word: %s", list->current->word);
            
        swish_xfree(list->current->word);
                
        if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
            swish_debug_msg("free Word struct");
            
        t = list->current->next;
        swish_xfree(list->current);
        list->current = t;
    }
    
    if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
        swish_debug_msg("reset nwords");
        
    list->nwords = 0;
    
    if (SWISH_DEBUG == SWISH_DEBUG_MEMORY)
        swish_debug_msg("free list");
        
    swish_xfree(list);
}

#ifdef HAVE_REGEX_TOKENIZER
/***********************************************

    TODO: regex tokenizer
    
************************************************/
swish_WordList *    swish_tokenize_regex(
                                      swish_Analyzer * analyzer,
                                      xmlChar * str,
                                      unsigned int offset,
                                      unsigned int word_pos,
                                      xmlChar * metaname, 
                                      xmlChar * context
                                      )
{

    swish_WordList *list = swish_init_wordlist();


    return list;
}
#endif


static
int
is_ignore_start_ascii(char c)
{
    return (!c ||
        isspace(c) ||
        iscntrl(c) ||
        ispunct(c)
        )
    ? 1 : 0;
    
}

static
int
is_ignore_end_ascii(char c)
{
    return (!c ||
        isspace(c) ||
        iscntrl(c) ||
        ispunct(c)
        )
    ? 1 : 0;
}

static
int
is_ignore_word_ascii(char c)
{
    if(SWISH_CONTRACTIONS && c == '\'')
        return 0;
        
    return ( !c ||
        isspace(c) ||
        iscntrl(c) ||
        ispunct(c)
        )
    ? 1 : 0;
}



/* wide character versions of above */

static
int
is_ignore_start(wint_t c)
{
    return (!c ||
        iswspace(c) ||
        iswcntrl(c) ||
        iswpunct(c)
        )
    ? 1 : 0;
    
}

static
int
is_ignore_end(wint_t c)
{
    return (!c ||
        iswspace(c) ||
        iswcntrl(c) ||
        iswpunct(c)
        )
    ? 1 : 0;
}

static
int
is_ignore_word(wint_t c)
{
    if(SWISH_CONTRACTIONS && c == '\'')
        return 0;
        
    if( !c ||
        iswspace(c) ||
        iswcntrl(c) ||
        iswpunct(c)
        )
        return 1;
        
    return 0;
}

/* returns the number of UTF-8 char* needed to hold the codepoint
   represented by 'ch'.
   similar to swish_utf8_chr_len() except that the arg is already
   a 4-byte container and we want to know how many of the 4 bytes
   we really need.
 */
static int
bytes_in_chr(wint_t ch)
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
    
    if( SWISH_DEBUG == SWISH_DEBUG_TOKENIZER )
        swish_debug_msg(" %lc is %d bytes long", ch, len);
        
    return len;
}


swish_WordList *
swish_tokenize_utf8_string(
               swish_Analyzer * analyzer,
               xmlChar * str,
               unsigned int offset,
               unsigned int word_pos,
               xmlChar * metaname,
               xmlChar * context
)
{

    int byte_count = 0;
    swish_WordList *list = swish_init_wordlist();
    xmlChar * utf8_str;

    /* convert xmlChar str into a widechar string for comparing against isw*() functions.
     * the returned pointer must be freed eventually.
     */
    wchar_t        *wide = swish_locale_to_wchar(str);

    /* init other temp vars */
    wchar_t        *word = (wchar_t *) swish_xmalloc(sizeof(wchar_t) * analyzer->maxwordlen);
    wchar_t         c, nextc;
    int             i, w, wl, in_word;

    w = 0;

    /* init the word counter */
    word[w] = 0;

    /* flag to tell us where we are */
    in_word = 0;

    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
        swish_debug_msg("parsing string: '%ls' into words", wide);

    for (i = 0; wide[i] != '\0'; i++)
    {
        c = (int) towlower(wide[i]);
        nextc = (int) towlower(wide[i + 1]);
        byte_count += bytes_in_chr((wint_t)c);

        if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
            swish_debug_msg(" wchar: %lc lower: %lc  int: %d %#x\n    orig: %lc %ld %#lx (next is %lc)",
                   (wint_t) wide[i],
                   (wint_t) c,
                   (int) c,
                   (unsigned int) c,
                   (wint_t) str[i],
                   (long int) str[i],
                   (long int) str[i],
                   (wint_t) nextc);


        /*
            *   either the c is ignored or not
            *   if ignored, and we're accruing a word, end the word
            *   if !ignored, and we're accruing, add to word, else start word
            */
            
            
        if (is_ignore_word((wint_t)c))
        {

            if (in_word)
            {
                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("found end of token: '%lc'", (wint_t)c);

                /* turn off flag */
                in_word = 0;
                
                /* convert back to utf8 */
                word[w] = '\0';
                wl = strip_wide_chars(word, w);
                utf8_str = swish_wchar_to_locale((wchar_t *) word);

                if (wl >= analyzer->minwordlen)
                {
                    swish_add_to_wordlist(  list, 
                                            utf8_str, 
                                            metaname, 
                                            context, 
                                            ++word_pos, 
                                            (byte_count + offset - 1));
                }
                else
                {
                    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                        swish_debug_msg("skipping token '%s' -- too short: %d", utf8_str, wl);
                }
                
                swish_xfree(utf8_str);

                continue;

            }
            else
            {
                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("ignoring char '%lc'", (wint_t)c);
                    
                continue;
            }

        }
        else
        {
        
            if (in_word)
            {
            
                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("adding to token: '%lc'", (wint_t)c);

                word[w++] = c;

                /* end the word if we've reached our limit or the end of the string */
                if (w >= analyzer->maxwordlen || nextc == '\0')
                {

                    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                        swish_debug_msg("forcing end of token: '%lc'", (wint_t)c);


                    /* turn off flag */
                    in_word     = 0;                    
                    
                    word[w]     = '\0';
                    wl          = strip_wide_chars(word, w);
                    utf8_str    = swish_wchar_to_locale((wchar_t *) word);

                    if (wl >= analyzer->minwordlen)
                    {
                        swish_add_to_wordlist(  list, 
                                                utf8_str, 
                                                metaname, 
                                                context, 
                                                ++word_pos, 
                                                byte_count + offset);
                    }
                    else
                    {
                        if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                            swish_debug_msg("skipping token '%ls' -- too short: %d", word, wl);
                    }
                    
                    swish_xfree(utf8_str);

                }

                continue;

            }
            else
            {

                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("start a token with '%lc'", (wint_t)c);

                w = 0;
                in_word = 1;
                word[w++] = c;
                continue;

            }

        }

    }

    swish_xfree(wide);
    swish_xfree(word);

    return list;
}

/************************************************
*   mimic the Swish-e WordCharacters lookup tables
*   using the default is*() functions.
*************************************************/

static char ascii_word_table[128];
static char ascii_start_table[128];
static char ascii_end_table[128];

static void
make_ascii_tables()
{
    int i;
 	for (i = 0; i < 127; i++)
    {
        if (is_ignore_word_ascii(i))
 	        ascii_word_table[i] = 0;
        else
            ascii_word_table[i] = 1;
 	    
        if (is_ignore_end_ascii(i))
            ascii_end_table[i] = 0;
        else
            ascii_end_table[i] = 1;
            
        if (is_ignore_start_ascii(i))
            ascii_start_table[i] = 0;
        else
            ascii_start_table[i] = 1;
            
    }
}



/************************************************************
*   This should save some overhead compared to the utf8 version
*   for the common case of all ascii text in a string.
*   Just like all the Swish3 tokenizing code, this is just a sane
*   fallback function. We expect and encourage users to write their
*   own tokenizers and use those instead.
*
**************************************************************/


swish_WordList *
swish_tokenize_ascii_string(
               swish_Analyzer * analyzer,
               xmlChar * str,
               unsigned int offset,
               unsigned int word_pos,
               xmlChar * metaname,
               xmlChar * context
)
{
    char            c, nextc, in_word;
    int             i, w, wl, byte_count;
    swish_WordList * list = swish_init_wordlist();
    xmlChar        * word = swish_xmalloc(sizeof(xmlChar*) * analyzer->maxwordlen);

    if (!initialized)
    {
        swish_warn_err("swish_init_words() was not explicitly called -- initializing....");
        swish_init_words();
    }        

    byte_count  = 0;
    w           = 0;
    word[w]     = 0;
    in_word     = 0;

    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
        swish_debug_msg("tokenizing string: '%s'", str);


    for (i = 0; str[i] != NULL; i++)
    {
        c       = (int) tolower(str[i]);
        nextc   = (int) tolower(str[i + 1]);
        byte_count++;

        if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
            swish_debug_msg(" char: %c lower: %c  int: %d %#x (next is %c)",
                   str[i],
                   c,
                   (int) c,
                   (unsigned int) c,
                   nextc);


           /*
            *   either the c is ignored or not
            *   if ignored, and we're accruing a word, end the word
            *   if !ignored, and we're accruing, add to word, else start word
            */
            
            
        if (!ascii_word_table[(int)c])
        {

            if (in_word)
            {
                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("found end of token: '%c' at %d", c, byte_count);

                /* turn off flag */
                in_word = 0;
                
                /* add NULL */
                word[w] = NULL;
                wl      = strip_ascii_chars(word, w);

                if (wl >= analyzer->minwordlen)
                {
                    swish_add_to_wordlist(  list, 
                                            word, 
                                            metaname, 
                                            context, 
                                            ++word_pos, 
                                            (byte_count + offset - 1));
                }
                else
                {
                    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                        swish_debug_msg("skipping token '%s' -- too short: %d", word, wl);
                }
                
                continue;

            }
            else
            {
                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("ignoring char '%c'", c);
                    
                continue;
            }

        }
        else
        {
        
            if (in_word)
            {
            
                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("adding to token: '%c' %d", c, byte_count);

                word[w++] = c;

                /* end the word if we've reached our limit or the end of the string */
                if (w >= analyzer->maxwordlen || nextc == NULL)
                {

                    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                        swish_debug_msg("forcing end of token: '%c' %d", c, byte_count);


                    /* turn off flag */
                    in_word = 0;                    
                    
                    word[w] = NULL;
                    wl = strip_ascii_chars(word, w);
                    
                    if (wl >= analyzer->minwordlen)
                    {
                        swish_add_to_wordlist(  list, 
                                                word, 
                                                metaname, 
                                                context, 
                                                ++word_pos, 
                                                byte_count + offset);
                    }
                    else
                    {
                        if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                            swish_debug_msg("skipping token '%s' -- too short: %d", word, wl);
                    }
                    
                }

                continue;

            }
            else
            {

                if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
                    swish_debug_msg("start a token with '%c' %d", c, byte_count);

                w = 0;
                in_word = 1;
                word[w++] = c;
                continue;

            }

        }

    }

    swish_xfree(word);

    return list;

}

/***************************************************************************
 *   main tokenizer function
 *   mostly just tests the string and analyzer and calls the right function.
 ***************************************************************************/

swish_WordList *
swish_tokenize(swish_Analyzer * analyzer, xmlChar * str, ...)
{
    xmlChar * metaname, * context;
    unsigned int word_pos, offset;
    va_list args;
    
    va_start(args, str);
    offset      = va_arg(args, unsigned int);
    word_pos    = va_arg(args, unsigned int);
    metaname    = va_arg(args, xmlChar *);
    context     = va_arg(args, xmlChar *);
    
    va_end(args);

#ifdef HAVE_REGEX_TOKENIZER
    if (analyzer->regex != NULL)
    {
        swish_debug_msg("tokenize with regex '%s'", analyzer->regex);
        return swish_tokenize_regex( analyzer, str, offset, word_pos, metaname, context );
                                    
    }
#endif

    if (swish_is_ascii( str ))
    {
        //swish_debug_msg("%s is ascii", str);
        return swish_tokenize_ascii_string( analyzer, str, offset, word_pos, metaname, context );
    }
    else
    {
        //swish_debug_msg("%s is utf8", str);
        return swish_tokenize_utf8_string( analyzer, str, offset, word_pos, metaname, context );
    }
    
}

/*************************************************
* remove all ignorable start/end chars
* and return new length of word
* based on code in swish-e vers2 swish_words.c
*************************************************/

static int
strip_wide_chars(wchar_t * word, int len)
{
    int             i, j, k, start, end;
    start = 0;
    end = 0;

    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
        swish_debug_msg("Before: %ls", word);

    /* end chars -- must do before start chars */

    i = len;

    /* Iteratively strip off the last character if it's an ignore character */
    while (i-- > 0)
    {

        if (is_ignore_end((wint_t)word[i]))
        {
            word[i] = '\0';
            end++;
        }
        else
        {
            break;
        }
    }

    /* start chars */
    i = 0;

    while (word[i])
    {
        k = i;
        if (!is_ignore_start((wint_t)word[k]))
        {
            break;
        }
        else
        {
            i = k + 1;
            start++;
        }
    }

    /* If all the chars are valid, just leave word alone */
    if (i != 0)
    {
        for (k = i, j = 0; word[k] != '\0'; j++, k++)
        {
            word[j] = word[k];
        }

        /* Add the NULL */
        word[j] = '\0';
    }

    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
        swish_debug_msg("After: %ls (stripped %d start chars, %d end chars)", word, start, end);

    return (int) wcslen(word);
}

/* ascii version of above */
static int
strip_ascii_chars(xmlChar * word, int len)
{
    int             i, j, k, start, end;
    start = 0;
    end = 0;

    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
        swish_debug_msg("Before: %s", word);

    /* end chars -- must do before start chars */

    i = len;

    /* Iteratively strip off the last character if it's an ignore character */
    while (i-- > 0)
    {

        if (!ascii_end_table[word[i]])
        {
            word[i] = NULL;
            end++;
        }
        else
        {
            break;
        }
    }

    /* start chars */
    i = 0;

    while (word[i])
    {
        k = i;
        if (ascii_start_table[word[k]])
        {
            break;
        }
        else
        {
            i = k + 1;
            start++;
        }
    }

    /* If all the chars are valid, just leave word alone */
    if (i != 0)
    {
        for (k = i, j = 0; word[k] != '\0'; j++, k++)
        {
            word[j] = word[k];
        }

        /* Add the NULL */
        word[j] = NULL;
    }

    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
        swish_debug_msg("After: %s (stripped %d start chars, %d end chars)", word, start, end);

    return xmlStrlen(word);
}


/***********************************************
* add a xmlChar string to the linked word list
*
***********************************************/
static int
add_to_wordlist(
        swish_WordList * list,
        xmlChar * word,
        int len,
        xmlChar * metaname,
        xmlChar * context,
        int word_pos,
        int offset
)
{
    swish_Word  *thisword = (swish_Word *) swish_xmalloc(sizeof(swish_Word));
    
    if (SWISH_DEBUG == SWISH_DEBUG_TOKENIZER)
    {
        swish_debug_msg(" >>>>>>>>swish_Word<<<<<<<<:  %s", word);
        swish_debug_msg("     --METANAME--:  %s", metaname);
        swish_debug_msg("     --CONTEXT---:  %s", context);
        swish_debug_msg("     --POSITION--:  %d", word_pos);
        swish_debug_msg("     --OFFSET----:  %d", offset);
        swish_debug_msg("     --WORD LEN--:  %d", len);
    }

    /* add to wordlist */

    thisword->word     = word;
    thisword->position = word_pos;
    
    if (metaname != NULL)
        thisword->metaname = swish_xstrdup(metaname);
    else
        thisword->metaname = swish_xstrdup((xmlChar*)SWISH_DEFAULT_METANAME);
        
    if (context != NULL)
        thisword->context  = swish_xstrdup(context);
    else
        thisword->context  = swish_xstrdup((xmlChar*)SWISH_DEFAULT_METANAME);
        
    thisword->end_offset   = offset;
    thisword->start_offset = offset - len + 1;  /* +1 because want the first byte */

    /* add thisword to list */
    if (list->head == 0)
    {
        list->head = thisword;
        thisword->prev = 0;
    }
    else
    {
        list->tail->next = thisword;
        thisword->prev = list->tail;
    }

    list->tail = thisword;
    thisword->next = 0;

    /* increment total count */
    list->nwords++;

    return len;
}

size_t
swish_add_to_wordlist(
        swish_WordList * list,
        xmlChar * word,
        xmlChar * metaname,
        xmlChar * context,
        int word_pos,
        int offset
)
{

    size_t     len = xmlStrlen(word);
    xmlChar   *wordcopy = swish_xstrdup(word);
    add_to_wordlist(list, wordcopy, (int)len, metaname, context, word_pos, offset);
    return len;
}


/* similar to swish_add_to_wordlist() but instead of xstrdup we use xstrndup
   to avoid extra mallocs in the calling code. See the perl bindings sp_tokenizer()
   for an example.
 */
int 
swish_add_to_wordlist_len(  
        swish_WordList * list, 
        xmlChar * str,
        int len,
        xmlChar * metaname,
        xmlChar * context,
        int word_pos, 
        int offset )
{
    xmlChar  *word = swish_xstrndup(str, len);
    return add_to_wordlist(list, word, len, metaname, context, word_pos, offset);
}


void
swish_debug_wordlist( swish_WordList * list )
{
    swish_debug_msg("%d words in list", list->nwords);
    list->current = list->head;
    while (list->current != NULL)
    {
        swish_debug_msg("   ---------- WORD ---------  ");
        swish_debug_msg("word  : %s", list->current->word);
        swish_debug_msg(" meta : %s", list->current->metaname);
        swish_debug_msg(" context : %s", list->current->context);
        swish_debug_msg("  pos : %d", list->current->position);
        swish_debug_msg("soffset: %d", list->current->start_offset);
        swish_debug_msg("eoffset: %d", list->current->end_offset);
            
        list->current = list->current->next;
    }

}
