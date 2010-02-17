/*
 * This file is part of libswish3
 * Copyright (C) 2008 Peter Karman
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

/* utf8 tokenizer */

#include <wchar.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <err.h>
#include <stdarg.h>

#include "libswish3.h"

extern int SWISH_DEBUG;

static int is_ignore_start_utf8(
    uint32_t c
);
static int is_ignore_end_utf8(
    uint32_t c
);
static int is_ignore_word_utf8(
    uint32_t c
);
static int is_ignore_start_ascii(
    char c
);
static int is_ignore_end_ascii(
    char c
);
static int is_ignore_word_ascii(
    char c
);
static void make_ascii_tables(
);
static int strip_utf8_chrs(
    xmlChar *token,
    int len
);
static int strip_ascii_chrs(
    xmlChar *word,
    int len
);

static int
is_ignore_start_utf8(
    uint32_t c
)
{
    return (!c || iswspace(c) || iswcntrl(c) || iswpunct(c)
        )
        ? 1 : 0;
}

static int
is_ignore_end_utf8(
    uint32_t c
)
{
    return (!c || iswspace(c) || iswcntrl(c) || iswpunct(c)
        )
        ? 1 : 0;
}

static int
is_ignore_word_utf8(
    uint32_t c
)
{
    if (c == '\'')              /*  contractions allowed */
        return 0;

    if (!c || iswspace(c) || iswcntrl(c) || iswpunct(c)
        )
        return 1;

    return 0;
}

static int
is_ignore_start_ascii(
    char c
)
{
    return (!c || isspace(c) || iscntrl(c) || ispunct(c)
        )
        ? 1 : 0;

}

static int
is_ignore_end_ascii(
    char c
)
{
    return (!c || isspace(c) || iscntrl(c) || ispunct(c)
        )
        ? 1 : 0;
}

static int
is_ignore_word_ascii(
    char c
)
{
    if (c == '\'')              /*  contractions allowed */
        return 0;

    return (!c || isspace(c) || iscntrl(c) || ispunct(c)
        )
        ? 1 : 0;
}

/************************************************
*   mimic the Swish-e WordCharacters lookup tables
*   using the default is*() functions.
*************************************************/

static boolean ascii_init = 0;
static char ascii_word_table[128];
static char ascii_start_table[128];
static char ascii_end_table[128];

static void
make_ascii_tables(
)
{
    int i;
    for (i = 0; i < 127; i++) {
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
    ascii_init = 1;
}

/*************************************************
* remove all ignorable start/end chars
* and return new length of token
* based on code in swish-e vers2 swish_words.c
*************************************************/

static int
strip_utf8_chrs(
    xmlChar *token,
    int len
)
{
    int i, j, k, chr_len, start, end, token_len, cp;
    xmlChar chr[5];

    start = 0;
    end = 0;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("Before: %s", token);

// end chrs -- must do before start chars
    j = len;
    for (i = len; i >= 0; swish_utf8_prev_chr(token, &i)) {
        chr_len = j - i;
        if (!chr_len) {
            j = i;
            continue;
        }
        for (k = 0; k < chr_len; k++) {
            chr[k] = token[i + k];
        }
        chr[k] = '\0';
        if (is_ignore_end_utf8(swish_utf8_codepoint(chr))) {
            token[i] = '\0';
            end++;
        }
        else {
            break;
        }
    }

    chr[0] = '\0';
    j = 0;

// start chrs 
    for (i = 0; token[j] != '\0'; swish_utf8_next_chr(token, &i)) {
        chr_len = i - j;
        if (!chr_len) {
            j = i;
            continue;
        }
        for (k = 0; k < chr_len; k++) {
            chr[k] = token[j + k];
        }
        chr[k] = '\0';
        cp = swish_utf8_codepoint(chr);

        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
            SWISH_DEBUG_MSG("start chr_len %d chr: %s  [%d]", chr_len, chr, cp);

        if (!is_ignore_start_utf8(cp)) {
            break;
        }
        else {
            if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                SWISH_DEBUG_MSG("ignore_start %s", chr);

            token += i;
            start++;
        }
    }

    token_len = xmlStrlen(token) + 1;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("After: %s (stripped %d start chars, %d end chars, len=%d)",
                        token, start, end, token_len);

    return token_len;
}

static int
strip_ascii_chrs(
    xmlChar *word,
    int len
)
{
    int i, j, k, wlen, start, end;
    start = 0;
    end = 0;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("Before: %s", word);

/* end chars -- must do before start chars */

    i = len;

/* Iteratively strip off the last character if it's an ignore character */
    while (i-- > 0) {

        if (!ascii_end_table[word[i]]) {
            word[i] = '\0';
            end++;
        }
        else {
            break;
        }
    }

/* start chars */
    i = 0;

    while (word[i]) {
        k = i;
        if (ascii_start_table[word[k]]) {
            break;
        }
        else {
            i = k + 1;
            start++;
        }
    }

/* If all the chars are valid, just leave word alone */
    if (i != 0) {
        for (k = i, j = 0; word[k] != '\0'; j++, k++) {
            word[j] = word[k];
        }

/* Add the NUL */
        word[j] = '\0';
    }

    wlen = xmlStrlen(word) + 1;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("After: %s (stripped %d start chars, %d end chars, wlen=%d)",
                        word, start, end, wlen);

    return wlen;
}

swish_TokenList *
swish_token_list_init(
)
{
    swish_TokenList *tl;
    tl = swish_xmalloc(sizeof(swish_TokenList));
    tl->buf = xmlBufferCreateSize((size_t) SWISH_BUFFER_CHUNK_SIZE);
    tl->n = 0;
    tl->pos = 0;
    tl->ref_cnt = 0;
    tl->tokens = swish_xmalloc(sizeof(swish_Token *) * SWISH_TOKEN_LIST_SIZE);
    tl->contexts = swish_hash_init(8);

    if (!ascii_init)
        make_ascii_tables();

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("TokenList ptr 0x%x", (long int)tl);
        SWISH_DEBUG_MSG("TokenList->tokens ptr 0x%x", (long int)tl->tokens);
    }

    return tl;
}

void
swish_token_list_free(
    swish_TokenList *tl
)
{
    if (tl->ref_cnt != 0) {
        SWISH_WARN("freeing TokenList with ref_cnt != 0 (%d)", tl->ref_cnt);
    }

    while (tl->n) {
        tl->n--;
        tl->tokens[tl->n]->ref_cnt--;
        if (tl->tokens[tl->n]->ref_cnt < 1)
            swish_token_free(tl->tokens[tl->n]);
    }

    swish_xfree(tl->tokens);
    xmlBufferFree(tl->buf);
    swish_hash_free(tl->contexts); // MUST free **after** tokens
    swish_xfree(tl);
}

int
swish_token_list_add_token(
    swish_TokenList *tl,
    xmlChar *token,
    int token_len,
    swish_MetaName *meta,
    xmlChar *context
)
{
    int num_of_allocs;
    swish_Token *stoken;

    if (!token_len || !xmlStrlen(token)) {
        SWISH_CROAK("can't add empty token to token list");
    }

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("adding token: %s  meta=%s", token, meta->name);

    stoken = swish_token_init();
    stoken->offset  = xmlBufferLength(tl->buf);
    stoken->len     = token_len - 1;    // -1 to exclude the NUL
    stoken->pos     = ++tl->pos;
    stoken->meta    = meta;
    stoken->meta->ref_cnt++;
        
    /* add the token str to the token_list buffer */
    swish_token_list_set_token(tl, token, token_len);

    /* cache the context string and point at the cached value */
    swish_hash_exists_or_add( tl->contexts, context, context );
    stoken->context = swish_hash_fetch( tl->contexts, context );
    stoken->value   = swish_token_list_get_token_value( tl, stoken );
    stoken->ref_cnt++;

    num_of_allocs = tl->n / SWISH_TOKEN_LIST_SIZE;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENLIST) {
        SWISH_DEBUG_MSG("TokenList size: %d  num_allocs = %d  modulus %d", tl->n,
                        num_of_allocs, tl->n % SWISH_TOKEN_LIST_SIZE);
        swish_token_debug(stoken);
    }

    if (num_of_allocs && !(tl->n % SWISH_TOKEN_LIST_SIZE)) {
        if (SWISH_DEBUG & SWISH_DEBUG_TOKENLIST) {
            SWISH_DEBUG_MSG("realloc for tokens: 0x%x", (long int)tl->tokens);
        }

        tl->tokens =
            (swish_Token **)swish_xrealloc(tl->tokens,
                                           sizeof(swish_Token*) * (SWISH_TOKEN_LIST_SIZE *
                                                                  ++num_of_allocs));

    }
    tl->tokens[tl->n++] = stoken;
    return tl->n;
}

int
swish_token_list_set_token(
    swish_TokenList *tl,
    xmlChar *token,
    int len
)
{
    int ret;
    /* include the NUL so token->value can be treated like substr */
    ret = xmlBufferAdd(tl->buf, token, len);    
    if (ret != 0) {
        SWISH_CROAK("error appending token to buffer: %d", ret);
    }
    return ret;
}

swish_Token *
swish_token_init(
)
{
    swish_Token *t;
    t = swish_xmalloc(sizeof(swish_Token));
    t->pos = 0;
    t->offset = 0;
    t->meta = NULL;
    t->context = NULL;
    t->value = NULL;
    t->len = 0;
    t->ref_cnt = 0;
    return t;
}

void
swish_token_free(
    swish_Token *t
)
{
    if (t->ref_cnt != 0) {
        SWISH_WARN("freeing Token with ref_cnt != 0 (%d)", t->ref_cnt);
    }
    
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("freeing Token 0x%x with MetaName ref_cnt %d", 
            (long int)t, t->meta->ref_cnt);
    }
    
    t->meta->ref_cnt--;
    if (t->meta->ref_cnt == 0) {
        if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
            SWISH_DEBUG_MSG("Token's MetaName ref_cnt == 0 ... freeing MetaName");
        }
        swish_metaname_free(t->meta);
    }
    
    swish_xfree(t);
}

void
swish_token_debug(
    swish_Token *t
)
{
    SWISH_DEBUG_MSG("\n\
    t->ref_cnt      = %d\n\
    t->pos          = %d\n\
    t->context      = %s\n\
    t->meta         = %d [%s]\n\
    t->offset       = %d\n\
    t->len          = %d\n\
    t->value        = %s\n\
    ", t->ref_cnt, t->pos, t->context, t->meta->id, t->meta->name, t->offset, t->len, t->value);

}

void
swish_token_list_debug(
    swish_TokenIterator *it
)
{
    swish_Token *t;

    SWISH_DEBUG_MSG("Token buf:\n%s", xmlBufferContent(it->tl->buf));
    SWISH_DEBUG_MSG("Token buf length: %d\n", xmlBufferLength(it->tl->buf));
    SWISH_DEBUG_MSG("Number of tokens: %d", it->tl->n);

    while ((t = swish_token_iterator_next_token(it)) != NULL) {
        swish_token_debug(t);
    }
}

swish_TokenIterator *
swish_token_iterator_init(
    swish_Analyzer *a
)
{
    swish_TokenIterator *it;
    it = swish_xmalloc(sizeof(swish_TokenIterator));
    it->a = a;
    it->a->ref_cnt++;
    it->pos = 0;
    it->tl = swish_token_list_init();
    it->tl->ref_cnt++;
    it->ref_cnt = 0;
    return it;
}

void
swish_token_iterator_free(
    swish_TokenIterator *it
)
{
    if (it->ref_cnt != 0) {
        SWISH_WARN("freeing TokenIterator with ref_cnt != 0 (%d)", it->ref_cnt);
    }
    
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG(
        "freeing TokenIterator %d with TokenList ref_cnt %d and Analyzer ref_cnt %d", 
        it, it->tl->ref_cnt, it->a->ref_cnt);
    }
    
    it->a->ref_cnt--;
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("freeing TokenIterator with Analyzer ref_cnt = %d",
            it->a->ref_cnt);
    }
    if (it->a->ref_cnt == 0)
        swish_analyzer_free(it->a);
        
    it->tl->ref_cnt--;
    if (it->tl->ref_cnt == 0)
        swish_token_list_free(it->tl);
    
    swish_xfree(it);
}

xmlChar *
swish_token_list_get_token_value(
    swish_TokenList *tl,
    swish_Token *t
)
{
    const xmlChar *buf;
    buf = xmlBufferContent(tl->buf);
    buf += t->offset;
    return (xmlChar*)buf;
}

swish_Token *
swish_token_iterator_next_token(
    swish_TokenIterator *it
)
{
    swish_Token *t;
    t = NULL;
    
/* SWISH_DEBUG_MSG("next_token: %d %d", it->pos, it->tl->n); */
    if (it->pos >= it->tl->n)
        return NULL;

    
    t = it->tl->tokens[it->pos++];
    t->value = swish_token_list_get_token_value(it->tl, t);
    return t;
}

/* returns number of tokens added to TokenList */
int
swish_tokenize(
    swish_TokenIterator *ti, 
    xmlChar *buf, 
    swish_MetaName *meta,
    xmlChar *context
)
{
    if (swish_is_ascii(buf)) {
        return swish_tokenize_ascii(ti, buf, meta, context);
    }
    else {
        return swish_tokenize_utf8(ti, buf, meta, context);
    }
}

int
swish_tokenize_utf8(
    swish_TokenIterator *ti, 
    xmlChar *buf, 
    swish_MetaName *meta,
    xmlChar *context
)
{
    uint32_t cp;
    int nstart, byte_pos, prev_pos, i, chr_len, token_len, maxwordlen, minwordlen;
    swish_TokenList *tl;
    boolean inside_token;
    xmlChar chr[5];             /*  max len of UCS32 plus NUL */
    xmlChar *token, *copy, *buf_lower;
    
    tl          = ti->tl;
    maxwordlen  = ti->a->maxwordlen;
    minwordlen  = ti->a->minwordlen;
    token       = swish_xmalloc(sizeof(xmlChar) * maxwordlen);
    buf_lower   = swish_utf8_str_tolower(buf);
    nstart      = tl->n;
    inside_token = 0;
    byte_pos    = 0;
    prev_pos    = 0;
    token_len   = 0;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("starting tokenize3 for meta=%s", meta->name);

/*
       iterate over each utf8 character, evaluating its Unicode value,
       and creating tokens
*/

    for (   byte_pos = 0; 
            buf_lower[prev_pos] != '\0';
            swish_utf8_next_chr(buf_lower, &byte_pos)
    ) {
        chr_len = byte_pos - prev_pos;
        if (!chr_len) {
            prev_pos = byte_pos;
            continue;
        }

        for (i = 0; i < chr_len; i++) {
            chr[i] = buf_lower[prev_pos + i];
        }
        chr[i] = '\0';

        cp = swish_utf8_codepoint(chr);

        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER) {
            SWISH_DEBUG_MSG("%d %d: ut8 chr '%s' unicode %d  len %d next byte: %d",
                            byte_pos, prev_pos, chr, cp, chr_len, buf_lower[prev_pos + 1]);

        }
        
        prev_pos = byte_pos;

        if (is_ignore_word_utf8(cp)) {

            if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                SWISH_DEBUG_MSG("%s is ignore_word", chr);

            if (inside_token) {
                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("found end of token: '%s'", chr);

                inside_token = 0;       /*  turn off flag */

                token[++token_len] = '\0';
                copy = token;
                token_len = strip_utf8_chrs(token, token_len);

                if (token[0] != '\0' && token_len >= minwordlen) {

                    swish_token_list_add_token(tl, token, token_len, meta, context);

                }
                else {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("skipping token '%s' -- too short: %d", token,
                                        token_len);
                }

                token = copy;   /*  restore to top of array so we do not leak */

                if (cp == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;

            }
            else {
                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("ignoring chr '%s'", chr);

                if (cp == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;
            }

        }
        else {

            if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                SWISH_DEBUG_MSG("%s is NOT ignore_word", chr);

            if (inside_token) {

                /* edge case */
                if ((chr_len + token_len) > maxwordlen) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("token_len = %d  forcing end of token: '%s'",
                                        token_len, chr);
                    continue;
                }

                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("adding to token: '%s'", chr);

                memcpy(&token[token_len], chr, chr_len * sizeof(xmlChar));
                token[token_len + chr_len] = '\0';
                token_len += chr_len;

                if (token_len >= maxwordlen || buf_lower[byte_pos] == '\0') {

                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("token_len = %d  forcing end of token: '%s'",
                                        token_len, chr);

                    inside_token = 0;   /*  turn off flag */

                    token[++token_len] = '\0';
                    copy = token;
                    token_len = strip_utf8_chrs(token, token_len);

                    if (token[0] != '\0' && token_len >= minwordlen) {

                        swish_token_list_add_token(tl, token, token_len, meta, context);

                    }
                    else {
                        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                            SWISH_DEBUG_MSG("skipping token '%s' -- too short: %d", token,
                                            token_len);
                    }

                    token = copy;       /*  restore to top of array */

                }

                if (cp == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;

            }
            else {

                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("start a token with '%s'", chr);

                token[0] = '\0';
                token_len = 0;
                inside_token = 1;       /*  turn on flag */
                /* edge case */
                if (chr_len > maxwordlen)
                    continue;

                memcpy(&token[0], chr, chr_len * sizeof(xmlChar));
                token[chr_len] = '\0';
                token_len += chr_len;
                
                /* special case for one-character tokens */
                if (buf_lower[prev_pos] == '\0' && minwordlen == 1) {
                    inside_token        = 0;
                    token[token_len++]  = '\0';
                    swish_token_list_add_token(tl, token, token_len, meta, context);
                }

                if (cp == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;

            }

        }

    }

    swish_xfree(token);
    swish_xfree(buf_lower);
    return tl->n - nstart;
}

int
swish_tokenize_ascii(
    swish_TokenIterator *ti, 
    xmlChar *buf, 
    swish_MetaName *meta,
    xmlChar *context
)
{
    char c, nextc;
    boolean inside_token;
    int i, token_len, nstart, maxwordlen, minwordlen;
    xmlChar *token, *copy;
    swish_TokenList *tl;
    
    tl              = ti->tl;
    maxwordlen      = ti->a->maxwordlen;
    minwordlen      = ti->a->minwordlen;
    token           = swish_xmalloc(sizeof(xmlChar) * maxwordlen);
    nstart          = tl->n;
    token_len       = 0;
    token[0]        = '\0';
    inside_token    = 0;

    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
        SWISH_DEBUG_MSG("tokenizing string: '%s'", buf);

    for (i = 0; buf[i] != '\0'; i++) {
        c = (char)tolower(buf[i]);
        nextc = (char)tolower(buf[i + 1]);

        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
            SWISH_DEBUG_MSG(" char: %c lower: %c  int: %d %#x (next is %c)", buf[i], c,
                            (int)c, (unsigned int)c, nextc);
                            
        if (!ascii_word_table[(int)c]) {

            if (inside_token) {
                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("found end of token: '%c' at %d", c, i);

                inside_token = 0;
                token[token_len++] = '\0';
                copy = token;
                token_len = strip_ascii_chrs(token, token_len);

                if (token[0] != '\0' && token_len >= minwordlen) {
                    swish_token_list_add_token(tl, token, token_len, meta, context);
                }
                else {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("skipping token '%s' -- too short: %d", token,
                                        token_len);
                }

                token = copy;
                
                if (c == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }


                continue;

            }
            else {
                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("ignoring char '%c'", c);

                if (c == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;
            }

        }
        else {

            if (inside_token) {

                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("adding to token: '%c' %d", c, i);

                token[token_len++] = c;

                if (token_len >= maxwordlen || nextc == '\0') {

                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("forcing end of token: '%c' %d", c, i);

                    inside_token = 0;

                    token[token_len++] = '\0';
                    copy = token;
                    token_len = strip_ascii_chrs(token, token_len);

                    if (token[0] != '\0' && token_len >= minwordlen) {
                        swish_token_list_add_token(tl, token, token_len, meta, context);
                    }
                    else {
                        if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                            SWISH_DEBUG_MSG("skipping token '%s' -- too short: %d", token,
                                            token_len);
                    }

                    token = copy;

                }

                if (c == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;

            }
            else {

                if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                    SWISH_DEBUG_MSG("start a token with '%c' %d", c, i);

                token_len = 0;
                inside_token = 1;
                token[token_len++] = c;
                
                /* special case for one-character tokens */
                if (nextc == '\0' && minwordlen == 1) {
                    inside_token        = 0;
                    token[token_len++]  = '\0';
                    swish_token_list_add_token(tl, token, token_len, meta, context);
                }
                
                if (c == SWISH_TOKENPOS_BUMPER[0]) {
                    if (SWISH_DEBUG & SWISH_DEBUG_TOKENIZER)
                        SWISH_DEBUG_MSG("found tokenpos bumper byte at pos %d", tl->pos);
                    tl->pos++;
                }

                continue;

            }

        }

    }

    swish_xfree(token);
    return tl->n - nstart;
}
