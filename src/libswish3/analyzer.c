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

extern int      SWISH_DEBUG;

swish_Analyzer *
swish_init_analyzer( swish_Config * config )
{
    swish_Analyzer *a = (swish_Analyzer *) swish_xmalloc(sizeof(swish_Analyzer));
    
    /* TODO get this from config */
    a->maxwordlen = SWISH_MAX_WORD_LEN;
    a->minwordlen = SWISH_MIN_WORD_LEN;
    a->lc         = 1;
    a->ref_cnt    = 0;
        
    if (xmlStrEqual((xmlChar*)SWISH_DEFAULT_VALUE,
                    xmlHashLookup(
                        swish_subconfig_hash(config,(xmlChar*)SWISH_WORDS),
                        (xmlChar*)SWISH_PARSE_WORDS
                    )
                  )
       )
    {
        a->tokenize = 1;
    }
    else
    {
        if (SWISH_DEBUG)
            swish_debug_msg("skipping WordList");
            
        a->tokenize = 0;
    }
    
    /* tokenizer set in the parse* function */
    a->tokenizer = NULL;
    
    /* TODO get stemmer via config */
    a->stemmer   = NULL;
    
    /* TODO standalone regex lib */
    a->regex     = NULL;
    
    a->stash      = NULL;
    
    return a;
}

/* TODO
   IMPORTANT -- any struct members that require unique free()s should
   do that prior to calling this function.
   stemmer, for example, or regex
 */
 
void 
swish_free_analyzer( swish_Analyzer * a )
{
    swish_xfree(a);   
}

