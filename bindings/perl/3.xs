/* Copyright 2008 Peter Karman (perl@peknet.com)
 * This program is free software; you can redistribute it and/or modify
 * under the same terms as Perl itself. 
 */

/* all XS stuff is prefixed with 'sp_' for Swish Perl */

#include "xs_boiler.h"
#include "headers.h"
#include "macros.h"
#include "xs_helpers.c"


MODULE = SWISH::3       PACKAGE = SWISH::3

PROTOTYPES: enable

INCLUDE: XS/Constants.xs

swish_3*
init(CLASS)
    char* CLASS;

    PREINIT:
        HV* stash;
        HV* analyzer_stash;
        swish_3* s3;

    CODE:
        stash   = newHV();
        s3      = swish_init_swish3( &sp_handler, newRV_inc((SV*)stash) );
        s3->ref_cnt++;
        
        sp_hv_store(stash, DATA_CLASS_KEY,      newSVpv(DATA_CLASS, 0));
        sp_hv_store(stash, CONFIG_CLASS_KEY,    newSVpv(CONFIG_CLASS, 0));
        sp_hv_store(stash, ANALYZER_CLASS_KEY,  newSVpv(ANALYZER_CLASS, 0));
        sp_hv_store(stash, PARSER_CLASS_KEY,    newSVpv(PARSER_CLASS, 0));

        //sp_describe_object(s3->stash);
        //sp_describe_object(newRV_noinc((SV*)s3->stash));

        s3->analyzer->tokenizer = &sp_tokenize;
        analyzer_stash  = newHV();
        s3->analyzer->stash = newRV_inc((SV*)analyzer_stash);
                
        RETVAL = s3;
                
    OUTPUT:
        RETVAL



SV*
xml2_version(self)
    SV* self;
    
    CODE:
        RETVAL = newSVpvn( LIBXML_DOTTED_VERSION, strlen(LIBXML_DOTTED_VERSION) );
        
    OUTPUT:
        RETVAL
        
              
SV*
version(self)
    SV* self;
    
    CODE:
        RETVAL = newSVpvn( SWISH_LIB_VERSION, strlen(SWISH_LIB_VERSION) );
        
    OUTPUT:
        RETVAL   

SV*
slurp(self, filename)
    swish_3*    self;
    char*       filename;
    
    PREINIT:
        xmlChar* buf;
    
    CODE:
        buf     = swish_slurp_file((xmlChar*)filename);
        RETVAL  = newSVpv( (const char*)buf, 0 );
        swish_xfree(buf);
        
    OUTPUT:
        RETVAL

int
parse_file(self, filename)
    swish_3* self;
	SV*	     filename;
    
    PREINIT:
        char* file;
        
    CODE:
        file = SvPV(filename, PL_na);
                
# need to swap return values to make it Perlish
        RETVAL = swish_parse_file( self, (xmlChar*)file ) ? 0 : 1;
                        
    OUTPUT:
        RETVAL
        
     
int
parse_buffer(self, buffer)
    swish_3*    self;
    SV*         buffer;
    
    PREINIT:
        char* buf;
        
    CODE:
        buf     = SvPV(buffer, PL_na);
 
# need to swap return values to make it Perlish
        RETVAL = swish_parse_buffer( self, (xmlChar*)buf ) ? 0 : 1;
                
    OUTPUT:
        RETVAL

# TODO parse_fh



# accessors/mutators
void
_set_or_get(self, ...)
    swish_3* self;
ALIAS:
    set_config          = 1
    get_config          = 2
    set_analyzer        = 3
    get_analyzer        = 4
    set_parser          = 5
    get_parser          = 6
    set_handler         = 7
    get_handler         = 8
    set_data_class      = 9
    get_data_class      = 10
    set_parser_class    = 11
    get_parser_class    = 12
    set_config_class    = 13
    get_config_class    = 14
    set_analyzer_class  = 15
    get_analyzer_class  = 16
    set_stash           = 17
    get_stash           = 18
    set_regex           = 19
    get_regex           = 20
PREINIT:
    SV*             stash;
    SV*             RETVAL;
    SV*             tmp;
PPCODE:
{

    stash = self->stash;
    // TODO need this?
    SvREFCNT_inc(stash);
    
    //warn("number of items %d for ix %d", items, ix);
    
    START_SET_OR_GET_SWITCH

    // set_config
    case 1:  self->config->ref_cnt--;
             //warn("set_config ref_cnt of old config = %d", self->config->ref_cnt);
             if (self->config->ref_cnt < 1) {
                if (SWISH_DEBUG) {
                    warn("freeing config on set_config");
                }
                swish_free_config(self->config);
             }
             self->config = (swish_Config*)sp_extract_ptr(ST(1));
             self->config->ref_cnt++;
             //warn("set_config ref_cnt of new config = %d", self->config->ref_cnt);
             break;
             
    // get_config   
    case 2:  self->config->ref_cnt++;
             RETVAL = sp_bless_ptr(sp_hvref_fetch_as_char(stash, CONFIG_CLASS_KEY), (IV)self->config);
             break;

    // set_analyzer
    case 3:  self->analyzer->ref_cnt--;
             //warn("set_analyzer ref_cnt of old analyzer: %d", self->analyzer->ref_cnt);
             if (self->analyzer->ref_cnt < 1) {
                if (SWISH_DEBUG) {
                    warn("freeing analyzer on set_analyzer");
                }
                swish_free_analyzer(self->analyzer);
             }
             self->analyzer = (swish_Analyzer*)sp_extract_ptr(ST(1));
             self->analyzer->ref_cnt++;
             //warn("set_analyzer ref_cnt of new analyzer: %d", self->analyzer->ref_cnt);
             break;

    // get_analyzer
    case 4:  self->analyzer->ref_cnt++;
             RETVAL = sp_bless_ptr(sp_hvref_fetch_as_char(stash, ANALYZER_CLASS_KEY), (IV)self->analyzer);
             break;

    // set_parser
    case 5:  self->parser->ref_cnt--;
             if (self->parser->ref_cnt < 1) {
                if (SWISH_DEBUG) {
                    warn("freeing parser on set_parser");
                }
                swish_free_parser(self->parser);
             }
             self->parser = (swish_Parser*)sp_extract_ptr(ST(1));
             self->parser->ref_cnt++;
             break;
           
    // get_parser  
    case 6:  self->parser->ref_cnt++;
             RETVAL = sp_bless_ptr(sp_hvref_fetch_as_char(stash, PARSER_CLASS_KEY), (IV)self->parser);
             break;

    // set_handler
    case 7:  sp_hvref_replace(stash, HANDLER_KEY, ST(1));
             break;

    // get_handler
    case 8:  RETVAL = sp_hvref_fetch(stash, HANDLER_KEY);
             break;
    
    // set_data_class
    case 9:  sp_hvref_replace(stash, DATA_CLASS_KEY, ST(1));
             break;
             
    // get_data_class
    case 10: RETVAL = sp_hvref_fetch(stash, DATA_CLASS_KEY);
             break;

    // set_parser_class
    case 11: sp_hvref_replace(stash, PARSER_CLASS_KEY, ST(1));
             break;
    
    // get_parser_class
    case 12: RETVAL = sp_hvref_fetch(stash, PARSER_CLASS_KEY);
             break;
    
    // set_config_class
    case 13: sp_hvref_replace(stash, CONFIG_CLASS_KEY, ST(1));
             break;
    
    // get_config_class
    case 14: RETVAL = sp_hvref_fetch(stash, CONFIG_CLASS_KEY);
             break;

    // set_analyzer_class
    case 15: sp_hvref_replace(stash, ANALYZER_CLASS_KEY, ST(1));
             break;
    
    // get_analyzer_class
    case 16: RETVAL = sp_hvref_fetch(stash, ANALYZER_CLASS_KEY);
             break;

    // set_stash
    case 17: self->stash = ST(1);
             break;

    // get_stash
    case 18: RETVAL = stash;
             SvREFCNT_inc( self->stash );
             break;

    // set_regex
    case 19: sp_SV_is_qr( ST(1) );             
             self->analyzer->regex = SvREFCNT_inc( ST(1) );
             break;
             
    // get_regex
    case 20: RETVAL  = self->analyzer->regex;
             break;    
        
    END_SET_OR_GET_SWITCH
}

void
DESTROY(self)
    SV *self;

    PREINIT:
        swish_3 *s3;
        
    CODE:
        s3 = (swish_3*)sp_extract_ptr(self);
        s3->ref_cnt--;
        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_3 object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, s3->ref_cnt);
        }
        
        if (s3->ref_cnt < 1) {
            swish_free_swish3( s3 );
        }
        

int
refcount(obj)
    SV* obj;
        
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL


   

# include the other .xs files
INCLUDE: XS/Config.xs
INCLUDE: XS/Analyzer.xs
INCLUDE: XS/WordList.xs
INCLUDE: XS/Word.xs
INCLUDE: XS/Doc.xs
INCLUDE: XS/Data.xs

