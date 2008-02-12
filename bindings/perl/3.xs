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

swish_3*
init(CLASS)
    char* CLASS;

    PREINIT:
        HV* stash;
        HV* analyzer_stash;

    CODE:
        stash   = newHV();
        RETVAL  = swish_init_swish3( &sp_handler, newRV_inc((SV*)stash) );
        RETVAL->ref_cnt = 1;
        
        sp_hv_store(stash, DATA_CLASS_KEY,      newSVpv(DATA_CLASS, 0));
        sp_hv_store(stash, CONFIG_CLASS_KEY,    newSVpv(CONFIG_CLASS, 0));
        sp_hv_store(stash, ANALYZER_CLASS_KEY,  newSVpv(ANALYZER_CLASS, 0));
        sp_hv_store(stash, PARSER_CLASS_KEY,    newSVpv(PARSER_CLASS, 0));

        //sp_describe_object(RETVAL->stash);
        //sp_describe_object(newRV_noinc((SV*)RETVAL->stash));

        RETVAL->analyzer->ref_cnt = 1;
        RETVAL->analyzer->tokenizer = &sp_tokenize;
        analyzer_stash  = newHV();
        RETVAL->analyzer->stash = newRV_inc((SV*)analyzer_stash);
        
        RETVAL->config->ref_cnt = 1;
        RETVAL->parser->ref_cnt = 1;
        
        //SvREFCNT_inc(RETVAL);
        
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
        RETVAL = newSVpv( (const char*)swish_slurp_file((xmlChar*)filename), 0 );
        
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
        //SvREFCNT_inc((SV*)self);
        
        //warn("parse_file %s", file);
        
        // TODO self is broken. SV = UNKNOWN.
        // and yet, handler works...
        
        //Perl_sv_dump((SV*)self);
        //sp_describe_object(self->stash);
        
# need to swap return values to make it Perlish
        RETVAL = swish_parse_file(  self, (xmlChar*)file ) ? 0 : 1;
                
        //SvREFCNT_dec((SV*)self);
                        
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
        SvREFCNT_inc((SV*)self);


# need to swap return values to make it Perlish
        RETVAL = swish_parse_buffer( self, (xmlChar*)buf ) ? 0 : 1;
                
        SvREFCNT_dec((SV*)self);
                
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
    case 1:  self->config = (swish_Config*)sp_ptr_from_object(ST(1));
             break;
             
    // get_config   
    case 2:  RETVAL = sp_ptr_to_object(sp_hvref_fetch_as_char(stash, CONFIG_CLASS_KEY), (IV)self->config);
             break;

    // set_analyzer
    case 3:  self->analyzer = (swish_Analyzer*)sp_ptr_from_object(ST(1));
             break;

    // get_analyzer
    case 4:  RETVAL = sp_ptr_to_object(sp_hvref_fetch_as_char(stash, ANALYZER_CLASS_KEY), (IV)self->analyzer);
             break;

    // set_parser
    case 5:  self->parser = (swish_Parser*)sp_ptr_from_object(ST(1));
             break;
           
    // get_parser  
    case 6:  RETVAL = sp_ptr_to_object(sp_hvref_fetch_as_char(stash, PARSER_CLASS_KEY), (IV)self->parser);
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
        s3 = (swish_3*)sp_ptr_from_object(self);
        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_3 object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, s3->ref_cnt);
        }
        
        // TODO free pointers when we figure out ref cnt
        swish_free_swish3( s3 );
        

int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        

##########################################################################################

MODULE = SWISH::3       PACKAGE = SWISH::3::Constants

PROTOTYPES: enable

# TODO more from libswish3.h               
# constants           
BOOT:
        {
        HV *stash;
  
        stash = gv_stashpv("SWISH::3::Constants",       TRUE);
        newCONSTSUB(stash, "SWISH_PROP",           newSVpv(SWISH_PROP, 0));
        newCONSTSUB(stash, "SWISH_PROP_MAX",       newSVpv(SWISH_PROP_MAX, 0));
        newCONSTSUB(stash, "SWISH_PROP_SORT",      newSVpv(SWISH_PROP_SORT, 0));
        newCONSTSUB(stash, "SWISH_META",           newSVpv(SWISH_META, 0));
        newCONSTSUB(stash, "SWISH_MIME",           newSVpv(SWISH_MIME, 0));
        newCONSTSUB(stash, "SWISH_PARSERS",        newSVpv(SWISH_PARSERS, 0));
        newCONSTSUB(stash, "SWISH_INDEX",          newSVpv(SWISH_INDEX, 0));
        newCONSTSUB(stash, "SWISH_ALIAS",          newSVpv(SWISH_ALIAS, 0));
        newCONSTSUB(stash, "SWISH_WORDS",          newSVpv(SWISH_WORDS, 0));
        newCONSTSUB(stash, "SWISH_PROP_RECCNT",    newSVpv(SWISH_PROP_RECCNT, 0));
        newCONSTSUB(stash, "SWISH_PROP_RANK",      newSVpv(SWISH_PROP_RANK, 0));
        newCONSTSUB(stash, "SWISH_PROP_DOCID",     newSVpv(SWISH_PROP_DOCID, 0));
        newCONSTSUB(stash, "SWISH_PROP_DOCPATH",   newSVpv(SWISH_PROP_DOCPATH, 0));
        newCONSTSUB(stash, "SWISH_PROP_DBFILE",    newSVpv(SWISH_PROP_DBFILE, 0));
        newCONSTSUB(stash, "SWISH_PROP_TITLE",     newSVpv(SWISH_PROP_TITLE, 0));
        newCONSTSUB(stash, "SWISH_PROP_SIZE",      newSVpv(SWISH_PROP_SIZE, 0));
        newCONSTSUB(stash, "SWISH_PROP_MTIME",     newSVpv(SWISH_PROP_MTIME, 0));
        newCONSTSUB(stash, "SWISH_PROP_DESCRIPTION",newSVpv(SWISH_PROP_DESCRIPTION, 0));
        newCONSTSUB(stash, "SWISH_PROP_CONNECTOR", newSVpv(SWISH_PROP_CONNECTOR, 0));
        newCONSTSUB(stash, "SWISH_PROP_STRING",    newSViv(SWISH_PROP_STRING));
        newCONSTSUB(stash, "SWISH_PROP_DATE",      newSViv(SWISH_PROP_DATE));
        newCONSTSUB(stash, "SWISH_PROP_INT",       newSViv(SWISH_PROP_INT));
        }


# *************************************************************************************/

MODULE = SWISH::3		PACKAGE = SWISH::3::Config	

PROTOTYPES: enable

AV*
keys(self)
    swish_Config* self
    
    CODE:
        RETVAL = sp_get_xml2_hash_keys(self->conf);
    
    OUTPUT:
        RETVAL


# translate xml2 hashes into Perl hashes -- NOTE these are effectively read-only hashes
# you must use add() and delete() to actually write to the active config in memory
HV*
subconfig(self,key)
    swish_Config* self
    char* key
    
    PREINIT:
        xmlHashTablePtr sc;
        
    CODE:
        sc      = swish_subconfig_hash(self, (xmlChar*)key);
        RETVAL  = sp_xml2_hash_to_perl_hash(sc);

    OUTPUT:
        RETVAL

 

int
debug(self)
    swish_Config* self
    
    CODE:
        RETVAL = swish_debug_config(self);
        
    OUTPUT:
        RETVAL


swish_Config *
new(CLASS)
    char* CLASS;
    
    PREINIT:
        HV* stash;

    CODE:
        RETVAL = swish_init_config();
        RETVAL->ref_cnt++;
        stash = newHV();
        RETVAL->stash = newRV_inc((SV*)stash);
        
    OUTPUT:
        RETVAL



void
add(self, conf_file)
    swish_Config* self
	char *	conf_file
    
    CODE:
        swish_add_config((xmlChar*)conf_file, self);
      
      
void
delete(self, key)
    swish_Config* self
    char* key
    
    CODE:
        warn("delete() not yet implemented\n");
        
void
subconfig_delete(self, key, subconf)
    swish_Config* self
    char* key
    xmlHashTablePtr subconf
    
    CODE:
        warn("subconfig_delete() not yet implemented\n");

void
DESTROY(self)
    swish_Config* self
    
    CODE:
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_Config object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        
        
# *******************************************************************************

MODULE = SWISH::3       PACKAGE = SWISH::3::Analyzer

PROTOTYPES: enable


swish_Analyzer *
new(CLASS, config)
    char*           CLASS;
    swish_Config*   config;
    
    PREINIT:
        HV* stash;

    CODE:
        //RETVAL = swish_init_analyzer((swish_Config*)sp_ptr_from_object( (SV*)config ));
        RETVAL = swish_init_analyzer(config);
        RETVAL->ref_cnt = 1;
        stash = newHV();
        RETVAL->stash = newRV_inc((SV*)stash);
        
    OUTPUT:
        RETVAL


# accessors/mutators
void
_set_or_get(self, ...)
    swish_Analyzer* self;
ALIAS:
    set_regex           = 1
    get_regex           = 2
    set_token_handler   = 3
    get_token_handler   = 4
PREINIT:
    SV*             stash;
    SV*             RETVAL;
PPCODE:
{
    
    //warn("number of items %d for ix %d", items, ix);
    
    START_SET_OR_GET_SWITCH

    // set_regex
    case 1:  self->regex = ST(1);
             break;
             
    // TODO test refcnt
    // get_regex
    case 2:  RETVAL  = SvREFCNT_inc( self->regex );
             break;
             
    // TODO set token_handler   
        
    END_SET_OR_GET_SWITCH
}


void
DESTROY(self)
    swish_Analyzer* self
    
    CODE:
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_Analyzer object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }




# tokenize() from Perl space uses same C func as tokenizer callback
swish_WordList *
tokenize(self, str, ...)
    SV* self;
    SV* str;
    
    PREINIT:
        char* CLASS;
        xmlChar* metaname = (xmlChar*)SWISH_DEFAULT_METANAME;   
        xmlChar* context  = (xmlChar*)SWISH_DEFAULT_METANAME;
        unsigned int word_pos    = 0;
        unsigned int offset      = 0;
        xmlChar* buf = (xmlChar*)SvPV(str, PL_na);
        
    CODE:
        CLASS = WORDLIST_CLASS;
        
        // TODO reimplement as hashref arg
                
        if (!SvUTF8(str))
        {
            if (swish_is_ascii(buf))
                SvUTF8_on(str);     /* flags original SV ?? */
            else
                croak("%s is not flagged as a UTF-8 string and is not ASCII", buf);
        }
        
        if ( items > 2 )
        {
            word_pos = (int)SvIV(ST(2));
            
            if ( items > 3 )
                offset = (int)SvIV(ST(3));
                
            if ( items > 4 )
                metaname = (xmlChar*)SvPV(ST(4), PL_na);
                
            if ( items > 5 )
                context = (xmlChar*)SvPV(ST(5), PL_na);
                
            //warn ("word_pos %d  offset %d  metaname %s  context %s\n", word_pos, offset, metaname, context );
                
        }
                        
        RETVAL = sp_tokenize(
                        (swish_Analyzer*)sp_ptr_from_object(self),
                        buf,
                        word_pos,
                        offset,
                        metaname,
                        context
                        );
        
        RETVAL->ref_cnt++;
        
        /* TODO do we need to worry about free()ing metaname and context ?? */
                        
    OUTPUT:
        RETVAL



# tokenize_isw() uses native libswish3 tokenizer
swish_WordList *
tokenize_isw(self, str, ...)
    SV* self;
    SV* str;

    PREINIT:
        char* CLASS;
        xmlChar* metaname = (xmlChar*)SWISH_DEFAULT_METANAME;   
        xmlChar* context  = (xmlChar*)SWISH_DEFAULT_METANAME;
        unsigned int word_pos    = 0;
        unsigned int offset      = 0;
        xmlChar* buf = (xmlChar*)SvPV(str, PL_na);
        
    CODE:
        CLASS = WORDLIST_CLASS;
        
        if (!SvUTF8(str))
        {
            if (swish_is_ascii(buf))
                SvUTF8_on(str);     /* flags original SV ?? */
            else
                croak("%s is not flagged as a UTF-8 string and is not ASCII", buf);
        }
        
        if ( items > 2 )
        {
            word_pos = (int)SvIV(ST(2));
            
            if ( items > 3 )
                offset = (int)SvIV(ST(3));
                
            if ( items > 4 )
                metaname = (xmlChar*)SvPV(ST(4), PL_na);
                
            if ( items > 5 )
                context = (xmlChar*)SvPV(ST(5), PL_na);
                
        }
                
        swish_init_words(); /* in case it wasn't initialized elsewhere... */
        RETVAL = swish_tokenize(
                        (swish_Analyzer*)sp_ptr_from_object(self),
                        buf,
                        word_pos,
                        offset,
                        metaname,
                        context
                        );
        
        RETVAL->ref_cnt++;
        
        /* TODO do we need to worry about free()ing metaname and context ?? */
                        
    OUTPUT:
        RETVAL

int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        
        
# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::WordList

PROTOTYPES: enable
        
swish_Word *
next(self)
    swish_WordList* self
    
    PREINIT:
        char* CLASS;
    
    CODE:
        CLASS = WORD_CLASS;
        
        if (self->current == NULL) {
            self->current = self->head;
        }
        else {
            self->current = self->current->next;
        }
        RETVAL = self->current;
        
    OUTPUT:
        RETVAL


SV*
nwords(self)
    swish_WordList* self;
    
    CODE:
        RETVAL = newSViv( self->nwords );
        
    OUTPUT:
        RETVAL
    
    
void
debug(self)
    swish_WordList* self;
    
    CODE:
        swish_debug_wordlist( self );
        


void
DESTROY(self)
    swish_WordList* self
    
    CODE:
        self->ref_cnt--;
        if (self->ref_cnt < 1) {
            swish_free_wordlist(self);
        }
        
int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        

# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Word

PROTOTYPES: enable

SV*
word (self)
	swish_Word *	self;   
    CODE:
        RETVAL = newSVpvn( (char*)self->word, strlen((char*)self->word) );
        
    OUTPUT:
        RETVAL
        

SV*
metaname (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->metaname, strlen((char*)self->metaname) );
        
    OUTPUT:
        RETVAL
        
SV*
context (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->context, strlen((char*)self->context) );
        
    OUTPUT:
        RETVAL
        

SV*
position (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->position );
        
    OUTPUT:
        RETVAL

SV*
start_offset(self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->start_offset );
        
    OUTPUT:
        RETVAL

SV*
end_offset(self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->end_offset );
        
    OUTPUT:
        RETVAL


int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        

# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Doc

PROTOTYPES: enable

SV*
mtime(self)
    swish_DocInfo* self;
    
    CODE:
        RETVAL = newSViv( self->mtime );
        
    OUTPUT:
        RETVAL
        
SV*
size(self)
    swish_DocInfo* self;
    
    CODE:
        RETVAL = newSViv( self->size );
        
    OUTPUT:
        RETVAL
        
SV*
nwords(self)
    swish_DocInfo* self;
    
    CODE:
        RETVAL = newSViv( self->nwords );
        
    OUTPUT:
        RETVAL


SV*
encoding(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->encoding, strlen((char*)self->encoding) );
        
    OUTPUT:
        RETVAL

SV*
uri(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->uri, strlen((char*)self->uri) );
        
    OUTPUT:
        RETVAL

SV*
ext(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->ext, strlen((char*)self->ext) );
        
    OUTPUT:
        RETVAL
        
SV*
mime(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->mime, strlen((char*)self->mime) );
        
    OUTPUT:
        RETVAL
        

SV*
parser(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->parser, strlen((char*)self->parser) );
        
    OUTPUT:
        RETVAL

int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        

# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Data

PROTOTYPES: enable


swish_3*
s3(self)
    swish_ParseData* self
    
    PREINIT:
        char* CLASS;

    CODE:
        CLASS  = sp_get_objects_class((SV*)self->s3);
        RETVAL = self->s3;
        
    OUTPUT:
        RETVAL
        
    CLEANUP:
        SvREFCNT_inc(RETVAL);


swish_Config*
config(self)
    swish_ParseData* self
    
	PREINIT:
        char* CLASS;

    CODE:
        CLASS  = sp_hvref_fetch_as_char(self->s3->stash, CONFIG_CLASS_KEY);
        RETVAL = self->s3->config;
        RETVAL->ref_cnt++;
        
    OUTPUT:
        RETVAL
        
        
SV*
property(self, p)
    swish_ParseData* self;
    xmlChar* p;
    
	PREINIT:
        xmlBufferPtr buf;
        
    CODE:
        buf = xmlHashLookup(self->properties->hash, p);
        RETVAL = newSVpvn((char*)xmlBufferContent(buf), xmlBufferLength(buf));
        
    OUTPUT:
        RETVAL
        
HV*
properties(self)
    swish_ParseData* self
    
    CODE:
        RETVAL = sp_nb_to_hash( self->properties );
        
    OUTPUT:
        RETVAL
        

HV*
metanames(self)
    swish_ParseData* self
    
    CODE:
        RETVAL = sp_nb_to_hash( self->metanames );
        
    OUTPUT:
        RETVAL
       


swish_DocInfo *
doc(self)
    swish_ParseData* self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS = "SWISH::3::Doc";
        RETVAL = self->docinfo;
        
    OUTPUT:
        RETVAL


swish_WordList *
wordlist(self)
    swish_ParseData* self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS = WORDLIST_CLASS;
        
# MUST increment refcnt 2x so that SWISH::3::Parser::WordList::DESTROY
# does not free it.
        self->wordlist->ref_cnt += 2;
        RETVAL = self->wordlist;
        
    OUTPUT:
        RETVAL
        
    
# must decrement refcount for stashed SWISH::3::Parser object
# since we increment it in parse_buf() and parse_file()
# TODO: this way of doing it doesn't work.
# but isn't it a potential mem leak to just _inc in parse_*() without
# _dec somewhere else? just means that the SWISH::3::Parser object
# may never get garbage collected.
#void
#DESTROY(self)
#    swish_ParseData* self;
#    
#    CODE:
#        SvREFCNT_dec( self->user_data );
        
int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        

