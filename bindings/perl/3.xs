/*
 * This file is part of libswish3
 * Copyright (C) 2010 Peter Karman
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

/* all XS stuff is prefixed with 'sp_' for Swish Perl */

#include "xs_boiler.h"
#include "libswish3.h"
#include "macros.h"
#include "xs_helpers.c"


MODULE = SWISH::3       PACKAGE = SWISH::3

PROTOTYPES: enable


void
_setup(CLASS)
    char* CLASS;
    
    CODE:
        swish_setup();



swish_3*
_init(CLASS)
    char* CLASS;

    PREINIT:
        swish_3* s3;

    CODE:
        s3 = swish_3_init( &sp_handler, NULL );
        s3->ref_cnt++;
        s3->stash = sp_Stash_new();
        
        sp_Stash_set_char( s3->stash, DATA_CLASS_KEY,     DATA_CLASS );
        sp_Stash_set_char( s3->stash, CONFIG_CLASS_KEY,   CONFIG_CLASS );
        sp_Stash_set_char( s3->stash, ANALYZER_CLASS_KEY, ANALYZER_CLASS );
        sp_Stash_set_char( s3->stash, PARSER_CLASS_KEY,   PARSER_CLASS );
        sp_Stash_set_char( s3->stash, SELF_CLASS_KEY,     CLASS );
        
        //warn("new() stash refcnt = %d\n", SvREFCNT((SV*)SvRV((SV*)s3->stash)));
        
        //sp_describe_object( (SV*)s3->stash );
        
        s3->analyzer->tokenizer = (&sp_tokenize3);

        s3->analyzer->stash  = sp_Stash_new();
        sp_Stash_set_char( s3->analyzer->stash, SELF_CLASS_KEY, ANALYZER_CLASS );
        
        s3->config->stash    = sp_Stash_new();
        sp_Stash_set_char( s3->config->stash, SELF_CLASS_KEY, CONFIG_CLASS );
                
        RETVAL = s3;
                
    OUTPUT:
        RETVAL


void
_show_sizes(self)
    SV* self;
 
    CODE:
        warn("sizeof pointer: %ld\n", sizeof(SV*));
        warn("sizeof long: %ld\n", sizeof(long));
        warn("sizeof int: %ld\n", sizeof(int));
        warn("sizeof IV: %ld\n", sizeof(IV));



char*
xml2_version(self)
    SV* self;
    
    CODE:
        RETVAL = (char*)swish_libxml2_version();
        
    OUTPUT:
        RETVAL
        
              
char*
version(self)
    SV* self;
    
    CODE:
        RETVAL = (char*)swish_lib_version();
        
    OUTPUT:
        RETVAL

void
wc_report(self, codepoint)
    SV *self;
    IV codepoint;
    
    CODE:
        sp_isw_report(codepoint);



SV*
error(self)
    swish_3 *self;
    
    PREINIT:
        IV err_code;
        SV *msg;
        
    CODE:
        msg = newSV(0);
        err_code = SvIV( sp_Stash_get(self->stash, "error") );
        if (err_code == 0) {
            msg = &PL_sv_undef;
        }
        else {
            sv_setpv( msg, swish_err_msg( (int)err_code ) );
        }
        RETVAL = msg;
        
    OUTPUT:
        RETVAL


SV*
slurp(self, filename, ...)
    SV*    self;
    char*  filename;
    
    PREINIT:
        xmlChar* buf;
        struct stat info;
        boolean binmode;
        off_t buflen;
         
    CODE:
        binmode = SWISH_FALSE;
        if ( items > 2 ) {
            binmode = SvIV(ST(2));
        }
        if (stat((char *)filename, &info)) {
            croak("Can't stat %s: %s\n", filename, strerror(errno));
        }
        buflen = info.st_size;
        if (swish_fs_looks_like_gz( (xmlChar*)filename )) {
            //warn("%s looks like gz\n", filename);
            buf = swish_io_slurp_gzfile_len((xmlChar*)filename, &buflen, binmode);
        }
        else {
            buf = swish_io_slurp_file_len((xmlChar*)filename, buflen, binmode);
        }
        RETVAL  = newSV(0);
        //warn("%s re-using SV with strlen %d\n", filename, (int)buflen);
        sv_usepvn_mg(RETVAL, (char*)buf, buflen);
        swish_memcount_dec(); // must do manually since Perl will free() it.

    OUTPUT:
        RETVAL

int
parse_file(self, filename)
    swish_3* self;
	SV*	     filename;
    
    PREINIT:
        char* file;
        int ret;
        
    CODE:
        file = SvPV(filename, PL_na);
                
# need to swap return values to make it Perlish
        ret = swish_parse_file( self, (xmlChar*)file );
        sp_Stash_set_int( self->stash, "error", ret );
        RETVAL = ret ? 0 : 1;
        
    OUTPUT:
        RETVAL
        
     
int
parse_buffer(self, buffer)
    swish_3*    self;
    SV*         buffer;
    
    PREINIT:
        char* buf;
        int ret;
        
    CODE:
        buf = SvPV(buffer, PL_na);
 
# need to swap return values to make it Perlish
        ret = swish_parse_buffer( self, (xmlChar*)buf );
        sp_Stash_set_int( self->stash, "error", ret );
        RETVAL = ret ? 0 : 1;
                
    OUTPUT:
        RETVAL


# TODO
int
parse_fh(self, fh)
    swish_3*    self;
    SV*         fh;
    
    CODE:
        croak("parse_fh() is not yet implemented.");
        RETVAL = 0;
        
    OUTPUT:
        RETVAL


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
    set_regex           = 17
    get_regex           = 18
    _x_placeholder      = 19  
    get_stash           = 20
PREINIT:
    SV  *RETVAL;
    char *class;
PPCODE:
{
    
    //warn("number of items %d for ix %d", items, ix);
    
    START_SET_OR_GET_SWITCH

    // set_config
    case 1:  if (!sv_derived_from(ST(1), CONFIG_CLASS)) {
                croak("usage: must be a %s object", CONFIG_CLASS);
             }
    
             self->config->ref_cnt--;
             //warn("set_config ref_cnt of old config = %d", self->config->ref_cnt);
             if (self->config->ref_cnt < 1) {
                if (SWISH_DEBUG) {
                    warn("freeing config on set_config");
                }
                sp_Stash_destroy(self->config->stash);
                if (self->config->stash != NULL) {
                    //SWISH_WARN("set config stash to NULL");
                    self->config->stash = NULL;
                }
                swish_config_free(self->config);
             }
             self->config = (swish_Config*)sp_extract_ptr(ST(1));
             self->config->ref_cnt++;
             //warn("set_config ref_cnt of new config = %d", self->config->ref_cnt);
             break;
             
    // get_config   
    case 2:  if (gimme != G_VOID)
                self->config->ref_cnt++;

             class = sp_Stash_get_char(self->stash, CONFIG_CLASS_KEY);
             sp_Stash_set_char( self->config->stash, SELF_CLASS_KEY, class );
             RETVAL = sp_bless_ptr(class, self->config);
             break;

    // set_analyzer
    case 3:  if (!sv_derived_from(ST(1), ANALYZER_CLASS)) {
                croak("usage: must be a %s object", ANALYZER_CLASS);
             }
             
             self->analyzer->ref_cnt--;
             //warn("set_analyzer ref_cnt of old analyzer: %d", self->analyzer->ref_cnt);
             if (self->analyzer->ref_cnt < 1) {
                if (SWISH_DEBUG) {
                    warn("freeing analyzer on set_analyzer");
                }
                sp_Stash_destroy(self->analyzer->stash);
                swish_analyzer_free(self->analyzer);
             }
             self->analyzer = (swish_Analyzer*)sp_extract_ptr(ST(1));
             self->analyzer->ref_cnt++;
             //warn("set_analyzer ref_cnt of new analyzer: %d", self->analyzer->ref_cnt);
             break;

    // get_analyzer
    case 4:  if (gimme != G_VOID)
                self->analyzer->ref_cnt++;
             
             class = sp_Stash_get_char(self->stash, ANALYZER_CLASS_KEY);
             sp_Stash_set_char( self->analyzer->stash, SELF_CLASS_KEY, class );
             RETVAL = sp_bless_ptr(class, self->analyzer);
             break;

    // set_parser
    case 5:  if (!sv_derived_from(ST(1), PARSER_CLASS)) {
                croak("usage: must be a %s object", PARSER_CLASS);
             }
             
             self->parser->ref_cnt--;
             if (self->parser->ref_cnt < 1) {
                if (SWISH_DEBUG) {
                    warn("freeing parser on set_parser");
                }
                swish_parser_free(self->parser);
             }
             self->parser = (swish_Parser*)sp_extract_ptr(ST(1));
             self->parser->ref_cnt++;
             break;
           
    // get_parser  
    case 6:  if (gimme != G_VOID)
                self->parser->ref_cnt++;

             class = sp_Stash_get_char(self->stash, PARSER_CLASS_KEY);
             RETVAL = sp_bless_ptr(class, self->parser);
             break;

    // set_handler
    case 7:  sp_Stash_replace(self->stash, HANDLER_KEY, ST(1));
             break;

    // get_handler
    case 8:  RETVAL = sp_Stash_get(self->stash, HANDLER_KEY);
             break;
    
    // set_data_class
    case 9:  sp_Stash_replace(self->stash, DATA_CLASS_KEY, ST(1));
             break;
             
    // get_data_class
    case 10: RETVAL = sp_Stash_get(self->stash, DATA_CLASS_KEY);
             break;

    // set_parser_class
    case 11: sp_Stash_replace(self->stash, PARSER_CLASS_KEY, ST(1));
             break;
    
    // get_parser_class
    case 12: RETVAL = sp_Stash_get(self->stash, PARSER_CLASS_KEY);
             break;
    
    // set_config_class
    case 13: sp_Stash_replace(self->stash, CONFIG_CLASS_KEY, ST(1));
             break;
    
    // get_config_class
    case 14: RETVAL = sp_Stash_get(self->stash, CONFIG_CLASS_KEY);
             break;

    // set_analyzer_class
    case 15: sp_Stash_replace(self->stash, ANALYZER_CLASS_KEY, ST(1));
             break;
    
    // get_analyzer_class
    case 16: RETVAL = sp_Stash_get(self->stash, ANALYZER_CLASS_KEY);
             break;

    // set_regex
    case 17: sp_SV_is_qr( ST(1) );             
             self->analyzer->regex = SvREFCNT_inc( ST(1) );
             break;
             
    // get_regex
    case 18: RETVAL  = self->analyzer->regex;
             break;    

    // get_stash
    case 20: RETVAL  = self->stash;
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
            warn("DESTROY %s [0x%lx] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), (long)s3, s3->ref_cnt);
        }

        if (SWISH_DEBUG) {
          warn("s3->ref_cnt == %d\n", s3->ref_cnt);
          warn("s3->config->ref_cnt == %d\n", s3->config->ref_cnt);
          warn("s3->analyzer->ref_cnt == %d\n", s3->analyzer->ref_cnt);
          /*
          warn("s3->stash refcnt = %d\n", 
            sp_Stash_inner_refcnt(s3->stash) );
          warn("s3->config->stash refcnt = %d\n", 
            sp_Stash_inner_refcnt( s3->config->stash) );
          warn("s3->analyzer->stash refcnt = %d\n", 
            sp_Stash_inner_refcnt( s3->analyzer->stash) );
          */
            
        }        
        
        if (s3->ref_cnt < 1) {
            sp_Stash_destroy( s3->stash );
            if ( s3->config->ref_cnt == 1 ) {
                sp_Stash_destroy( s3->config->stash );
                //SWISH_WARN("set config stash to NULL");
                s3->config->stash = NULL;
            }
            if ( s3->analyzer->ref_cnt == 1 ) {
                sp_Stash_destroy( s3->analyzer->stash );
                s3->analyzer->stash = NULL;
                SvREFCNT_dec( s3->analyzer->regex );
                s3->analyzer->regex = NULL;
            }
            
            swish_3_free( s3 );
        }
        

int
refcount(obj)
    SV* obj;
        
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL


int
ref_cnt(s3)
    swish_3 *s3;
            
    CODE:
        RETVAL = s3->ref_cnt;
    
    OUTPUT:
        RETVAL


void
describe(self, obj)
    SV* self;
    SV* obj;
    
    CODE:
        sp_describe_object(obj);
    
    

void
mem_debug(CLASS)
    char* CLASS;
    
    CODE:
        swish_mem_debug();
        

int
debug(CLASS, ...)
    char * CLASS;
    
    CODE:
        RETVAL = SWISH_DEBUG;
        if ( items > 1 ) {
            SWISH_DEBUG = SvIV( ST(1) );
            warn("SWISH_DEBUG set to %d", SWISH_DEBUG);
        }
        
    OUTPUT:
        RETVAL
   

int
get_memcount(CLASS)
    char* CLASS;
    
    CODE:
        RETVAL = swish_memcount_get();
        
    OUTPUT:
        RETVAL



# tokenize() from Perl space uses same C func as tokenizer callback
swish_TokenIterator *
tokenize(self, str, ...)
    swish_3* self;
    SV* str;
    
    PREINIT:
        char* CLASS;
        swish_TokenIterator* ti;
        swish_MetaName* metaname;
        xmlChar* context;
        xmlChar* buf;

    CODE:
        CLASS           = TOKENITERATOR_CLASS;
        ti              = swish_token_iterator_init(self->analyzer);
        ti->ref_cnt++;
        context         = (xmlChar*)SWISH_DEFAULT_METANAME;
        buf             = (xmlChar*)SvPV(str, PL_na);
        
        // TODO reimplement as hashref arg
               
        // TODO why this check?? 
        if (!SvUTF8(str))
        {
            if (swish_is_ascii(buf))
                SvUTF8_on(str);     /* flags original SV ?? */
            else
                croak("%s is not flagged as a UTF-8 string and is not ASCII", buf);
        }
        
        if ( items > 2 )
        {            
            metaname = (swish_MetaName*)sp_extract_ptr(ST(2));
                            
            if ( items > 3 )
                context = (xmlChar*)SvPV(ST(3), PL_na);
                
            //warn ("metaname %s  context %s\n", metaname, context );
                
        }
        else {
            metaname = swish_metaname_init(swish_xstrdup((xmlChar*)SWISH_DEFAULT_METANAME));
        }   
        
        sp_tokenize3( ti, buf, metaname, context );
        RETVAL = ti;

    OUTPUT:
        RETVAL



# native libswish3 tokenizer
swish_TokenIterator *
tokenize_native(self, str, ...)
    swish_3* self;
    SV* str;
    
    PREINIT:
        char* CLASS;
        swish_TokenIterator* ti;
        swish_MetaName* metaname;
        xmlChar* context;
        xmlChar* buf;

    CODE:
        CLASS           = TOKENITERATOR_CLASS;
        ti              = swish_token_iterator_init(self->analyzer);
        ti->ref_cnt++;
        context         = (xmlChar*)SWISH_DEFAULT_METANAME;
        buf             = (xmlChar*)SvPV(str, PL_na);
        
        // TODO reimplement as hashref arg
               
        // TODO why this check?? 
        if (!SvUTF8(str))
        {
            if (swish_is_ascii(buf))
                SvUTF8_on(str);     /* flags original SV ?? */
            else
                croak("%s is not flagged as a UTF-8 string and is not ASCII", buf);
        }
        
        if ( items > 2 )
        {            
            metaname = (swish_MetaName*)sp_extract_ptr(ST(2));
                            
            if ( items > 3 )
                context = (xmlChar*)SvPV(ST(3), PL_na);
                
            //warn ("metaname %s  context %s\n", metaname, context );
                
        }
        else {
            metaname = swish_metaname_init((xmlChar*)SWISH_DEFAULT_METANAME);
            metaname->ref_cnt++;
        }   
        
        swish_tokenize( ti, buf, metaname, context );
        RETVAL = ti;

    OUTPUT:
        RETVAL



# include the other .xs files
INCLUDE: XS/Constants.xs
INCLUDE: XS/Config.xs
INCLUDE: XS/Analyzer.xs
INCLUDE: XS/Doc.xs
INCLUDE: XS/Data.xs
INCLUDE: XS/Stash.xs
INCLUDE: XS/Property.xs
INCLUDE: XS/MetaName.xs
INCLUDE: XS/PropertyHash.xs
INCLUDE: XS/MetaNameHash.xs
INCLUDE: XS/xml2Hash.xs
INCLUDE: XS/Token.xs
INCLUDE: XS/TokenIterator.xs

