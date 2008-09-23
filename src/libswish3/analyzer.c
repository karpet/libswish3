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

/* text analyzer
   tokenize strings, stemming

*/

#include "libswish3.h"

extern int SWISH_DEBUG;

swish_Analyzer *
swish_init_analyzer(
    swish_Config *config
)
{
    swish_Analyzer *a;
    a = swish_xmalloc(sizeof(swish_Analyzer));

/* TODO get these all from config */
    a->maxwordlen = SWISH_MAX_WORD_LEN;
    a->minwordlen = SWISH_MIN_WORD_LEN;
    a->lc = 1;
    a->ref_cnt = 0;
    a->tokenize = config->flags->tokenize;

    if (!a->tokenize && SWISH_DEBUG)
        SWISH_DEBUG_MSG("skipping tokenizer");

/* tokenizer set in the parse* function */
    a->tokenizer = NULL;

/* TODO get stemmer via config */
    a->stemmer = NULL;

/* TODO standalone regex lib */
    a->regex = NULL;

    a->stash = NULL;
    
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("analyzer ptr 0x%x", (long int)a);
    }

    return a;
}

/* 
   IMPORTANT -- any struct members that require unique free()s should
   do that prior to calling this function.
   stemmer, for example, or regex
*/

void
swish_free_analyzer(
    swish_Analyzer *a
)
{
    if (a->ref_cnt != 0) {
        SWISH_WARN("analyzer ref_cnt != 0: %d\n", a->ref_cnt);
    }
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("free analyzer");
        swish_mem_debug();
    }
    
    if (a->stash != NULL)
        SWISH_WARN("Analyzer->stash not freed 0x%x", (long int)a->stash);
        
    if (a->regex != NULL)
        SWISH_WARN("Analyzer->regex not freed 0x%x", (long int)a->regex);
        
    if (a->stemmer != NULL)
        SWISH_WARN("Analyzer->stemmer not freed");
        
    
    swish_xfree(a);
}
