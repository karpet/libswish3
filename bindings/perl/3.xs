/*
 * Standard XS greeting.
 */
#ifdef __cplusplus
extern "C" {
#endif
#define PERL_NO_GET_CONTEXT 1
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "ppport.h"
#ifdef __cplusplus
}
#endif

#ifdef EXTERN
  #undef EXTERN
#endif

#define EXTERN static

/* libxml2 headers */
#include <libxml/xmlversion.h>
#include <libxml/parser.h>
#include <libxml/hash.h>
#include <libxml/parserInternals.h>
#include <libxml/HTMLparser.h>
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlIO.h>

#include <libswish3.h>


/* keep track of all subclasses in C space so we can call handlers easily 
 * NOTE this requires that when subclassing, all classes must be subclassed
 */

/* private package vars */

static char * callback_method = "handler";
static HV * SubClasses       = (HV*)NULL;
static int nClasses          = 5;   /* match Classes[] ?? */
static char * Classes[]      = { 
        "Doc", 
        "Word",
        "WordList",
        "Property", 
        "Data"
         };

static SV * callback_handler = (SV*)NULL;


/* private functions */

static void _remember_handler(SV* handler)
{
    dTHX;
    if (callback_handler == (SV*)NULL)
        /* First time, so create a new SV */
        callback_handler = newSVsv(handler);
    else
        /* Been here before, so overwrite */
        SvSetSV(callback_handler, handler);
}

static void _make_subclasses( char * class )
{
    /* form() is the Perl API call that will "join" our strings */
    dTHX;
    char * sc;
    int i;
    
    /* create hash */
    if (SubClasses == (HV*)NULL)
        SubClasses = newHV();
        
        
    for (i=0; i < nClasses; i++)
    {
        sc = form("%s::%s",class,Classes[i]);
        
        //warn("adding %s to SubClasses\n", sc);
                
        hv_store(SubClasses,
                Classes[i], 
                strlen(Classes[i]),
                newSVpvn( sc, strlen(sc) ),
                0 );
    }
    
    
}

static SV * _make_object( char* CLASS, IV data )
{
    dTHX;
    SV* obj = sv_newmortal();
    sv_setref_pv(obj, CLASS, (void*)data);
    return obj;
}

static void _example_of_call_method( char* class, IV data )
{
    dTHX;
    dSP;    /* !!! do NOT use dXSARGS -- for some reason causes panic memory wrap */
    SV*o = _make_object(class,data);
    
    PUSHMARK(SP);
    XPUSHs(o);
    PUTBACK;
    
    call_method(callback_method, G_DISCARD);
}


static char * _which_class( char * c )
{
    dTHX;
    SV** sv = hv_fetch( SubClasses, c, strlen(c), 0 );
    if ( !sv )
        croak("could not fetch %s class from SubClasses", c);
        
    return SvPV(*sv, PL_na);
}

static SV * _get_object_key( SV* object, char * name )
{
    dTHX;
    char * class = sv_reftype(SvRV(object), 1);
    HV* hash;
    //printf("looking for %s in %s\n", name, class);
    
    if (SvROK(object) && SvTYPE(SvRV(object))==SVt_PVHV)
        hash = (HV*)SvRV(object);
    else if (SvROK(object) && SvTYPE(SvRV(object))==SVt_PVMG)
        croak("%s is a reference but is a magic reference", class);
    else
        croak("%s is a reference but is not a hash reference", class);
    
         
    SV** sv = hv_fetch(hash, name, strlen(name), 0);
    
    if (!sv)
        croak("no %s in %s object!", name, class);
        
    return *sv;
}


static void _store_xml2_pair_in_perl_hash(xmlChar * val, HV * perl_hash, xmlChar * key)
{
    dTHX;
    hv_store(perl_hash, key, strlen(key), newSVpvn(val, strlen(val)), 0);
}

static HV * _xml2_hash_to_perl_hash( xmlHashTablePtr xml2_hash )
{
    dTHX;
    HV * perl_hash = newHV();
    /* perl bug means we must increm ref count manually */
    sv_2mortal((SV*)perl_hash);
    xmlHashScan(xml2_hash, (xmlHashScanner)_store_xml2_pair_in_perl_hash, perl_hash);
    return perl_hash;
}

static void _add_key_to_array(xmlChar * val, AV * mykeys, xmlChar * key)
{
    dTHX;
    av_push(mykeys, newSVpvn(key, strlen(key)));
}

static AV * _get_xml2_hash_keys( xmlHashTablePtr xml2_hash )
{
    dTHX;
    AV * mykeys = newAV();
    sv_2mortal((SV*)mykeys); /* needed?? */
    xmlHashScan(xml2_hash, (xmlHashScanner)_add_key_to_array, mykeys);
    return mykeys;
}




void swish_perl_test_handler(swish_ParseData * parse_data)
{
  warn("handler called!\n");
  swish_debug_docinfo( parse_data->docinfo );
  swish_debug_wordlist( parse_data->wordlist );
  swish_debug_PropHash( parse_data->propHash );
  warn("\n");
}

void swish_perl_handler( swish_ParseData* parse_data )
{
    dTHX;
    dSP;

    char * class = _which_class("Data");
    SV   * obj   = _make_object(class, (IV)parse_data);
    

    PUSHMARK(SP);
    XPUSHs(obj);
    PUTBACK;

    call_sv(callback_handler, G_DISCARD);
}

/*
swish_WordList *
swish_perl_re_tokenizer(xmlChar * string,
                        xmlChar * metaname,
                        xmlChar * context,
                        int maxwordlen,
                        int minwordlen,
                        int word_pos,
                        int offset)
{




}
*/


/*************************************************************************************/
MODULE = SWISH::3       PACKAGE = SWISH::3

PROTOTYPES: enable

SV*
xml2_version(self)
    SV* self;
    
    CODE:
        RETVAL = newSVpvn( LIBXML_DOTTED_VERSION, strlen(LIBXML_DOTTED_VERSION) );
        
    OUTPUT:
        RETVAL
        
        
SV*
swish_version(self)
    SV* self;
    
    CODE:
        RETVAL = newSVpvn( SWISH_VERSION, strlen(SWISH_VERSION) );
        
    OUTPUT:
        RETVAL    
      
SV*
libswish3_version(self)
    SV* self;
    
    CODE:
        RETVAL = newSVpvn( SWISH_LIB_VERSION, strlen(SWISH_LIB_VERSION) );
        
    OUTPUT:
        RETVAL   
   

# *********************************************************************************
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
  
# *********************************************************************************

MODULE = SWISH::3		PACKAGE = SWISH::3::Parser	

PROTOTYPES: enable
                    
void
_make_subclasses (self)
    SV * self
    
	PREINIT:
        char* class;
        
	CODE:
        class = sv_reftype(SvRV(self), 1);
        //printf("parent class is %s\n", class);
	    _make_subclasses(class);


void
_cleanup(self)
    SV* self;
   
    CODE:
        /* TODO */



SV*
slurp_file(self, filename)
    SV* self;
    char* filename;
    
    CODE:
        RETVAL = newSVpv( (const char*)swish_slurp_file((xmlChar*)filename), 0 );
        
    OUTPUT:
        RETVAL
        

void
_init_parser(self)
    SV* self;
    
    CODE:
        swish_init_parser();
        _remember_handler(_get_object_key(self,callback_method));
        
        
void
_free(self)
    SV* self;
    
    CODE:
        swish_free_parser();
        
#
#   TODO: pass our own _word_tokenizer() callback so we can use Perl regexp
#


int
parse_file (self, filename)
    SV* self;
	SV*	filename;
    
    PREINIT:
        char * file;
        SV *   config;
        
    CODE:
        file = SvPV(filename, PL_na);
        config = _get_object_key(self,"config");

# need to swap return values to make it Perlish
        RETVAL = swish_parse_file((xmlChar*)file, 
                                   (swish_Config*)SvIV((SV*)SvRV( config )),
                                    &swish_perl_handler,
                                    NULL,
                                    (void*)SvREFCNT_inc( self )
                                    ) 
                ? 0 
                : 1;
                        
    OUTPUT:
        RETVAL
        
     
int
parse_buf (self, buffer)
    SV* self;
    SV* buffer;
    
    PREINIT:
        SV* config;
        char* buf;
        
    CODE:
        config =  _get_object_key(self,"config");
        buf    = SvPV(buffer, PL_na);
                
        RETVAL = swish_parse_buffer((xmlChar*)buf,
                                     (swish_Config*)SvIV((SV*)SvRV( config )),
                                     &swish_perl_handler,
                                     NULL,
                                    (void*)SvREFCNT_inc( self )
                                    )
                ? 0
                : 1;
                
                
    OUTPUT:
        RETVAL
        

        
swish_WordList *
tokenize(self, str, ...)
    SV* self;
    SV* str;
    
    PREINIT:
        char * CLASS;
        char * metaname = SWISH_DEFAULT_METANAME;   
        char * context  = SWISH_DEFAULT_METANAME;
        int maxwordlen  = SWISH_MAX_WORD_LEN;
        int minwordlen  = SWISH_MIN_WORD_LEN;
        int word_pos    = 0;
        int offset      = 0;
        
    CODE:
        CLASS = _which_class("WordList");

        if ( items > 2 )
        {
            metaname = SvPV(ST(2), PL_na);
            
            if ( items > 3 )
                context = SvPV(ST(3), PL_na);
                
            if ( items > 4 )
                maxwordlen = (int)SvIV(ST(4));
                
            if ( items > 5 )
                minwordlen = (int)SvIV(ST(5));
                
            if ( items > 6 )
                word_pos = (int)SvIV(ST(6));
                
            if ( items > 7 )
                offset = (int)SvIV(ST(7));
                
        }
                
        RETVAL = swish_tokenize(
                        (xmlChar*)SvPV(str, PL_na),
                        (xmlChar*)metaname,
                        (xmlChar*)context,
                        maxwordlen,
                        minwordlen,
                        word_pos,
                        offset);
        
        RETVAL->ref_cnt++;
                        
    OUTPUT:
        RETVAL
        


# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::Word

PROTOTYPES: disable

SV *
word (self)
	swish_Word *	self;   
    CODE:
        RETVAL = newSVpvn( (char*)self->word, strlen((char*)self->word) );
        
    OUTPUT:
        RETVAL
        

SV *
metaname (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->metaname, strlen((char*)self->metaname) );
        
    OUTPUT:
        RETVAL
        
SV *
context (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->context, strlen((char*)self->context) );
        
    OUTPUT:
        RETVAL
        

SV *
position (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->position );
        
    OUTPUT:
        RETVAL

SV *
start_offset(self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->start_offset );
        
    OUTPUT:
        RETVAL

SV *
end_offset(self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->end_offset );
        
    OUTPUT:
        RETVAL



# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::Doc

PROTOTYPES: disable

SV*
mtime(self)
    swish_DocInfo * self;
    
    CODE:
        RETVAL = newSViv( self->mtime );
        
    OUTPUT:
        RETVAL
        
SV*
size(self)
    swish_DocInfo * self;
    
    CODE:
        RETVAL = newSViv( self->size );
        
    OUTPUT:
        RETVAL
        
SV*
nwords(self)
    swish_DocInfo * self;
    
    CODE:
        RETVAL = newSViv( self->nwords );
        
    OUTPUT:
        RETVAL


SV *
encoding(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->encoding, strlen((char*)self->encoding) );
        
    OUTPUT:
        RETVAL

SV *
uri(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->uri, strlen((char*)self->uri) );
        
    OUTPUT:
        RETVAL

SV *
ext(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->ext, strlen((char*)self->ext) );
        
    OUTPUT:
        RETVAL
        
SV *
mime(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->mime, strlen((char*)self->mime) );
        
    OUTPUT:
        RETVAL
        

SV *
parser(self)
	swish_DocInfo *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->parser, strlen((char*)self->parser) );
        
    OUTPUT:
        RETVAL



# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::Property

PROTOTYPES: disable

        

# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::WordList

PROTOTYPES: disable
        
swish_Word *
next(self)
    swish_WordList * self
    
    PREINIT:
        char * CLASS;
    
    CODE:
        CLASS = _which_class("Word");
        if (self->current == NULL) 
        {
            self->current = self->head;
        }
        else 
        {
            self->current = self->current->next;
        }
        RETVAL = self->current;
        
    OUTPUT:
        RETVAL



void
DESTROY(self)
    swish_WordList * self
    
    CODE:
        self->ref_cnt--;
        if (!self->ref_cnt)
        {
            swish_free_WordList(self);
        }
        


# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::Data

PROTOTYPES: disable

SV*
parser(self)
    swish_ParseData * self
    
    PREINIT:
        SV* parser;
        
    CODE:
        RETVAL = self->user_data;
        
    OUTPUT:
        RETVAL


swish_Config *
config(self)
    swish_ParseData * self
    
	PREINIT:
        char* CLASS;

    CODE:
        CLASS = "SWISH::3::Config";
        RETVAL = self->config;
        RETVAL->ref_cnt++;
        
    OUTPUT:
        RETVAL
        
        
SV*
property(self,p)
    swish_ParseData * self;
    xmlChar * p;
    
	PREINIT:
        xmlBufferPtr buf;
        
    CODE:
        buf = xmlHashLookup(self->propHash,p);
        RETVAL = newSVpvn((char*)xmlBufferContent(buf), xmlBufferLength(buf));
        
    OUTPUT:
        RETVAL
        

swish_DocInfo *
doc(self)
    swish_ParseData * self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS = _which_class("Doc");
        RETVAL = self->docinfo;
        
    OUTPUT:
        RETVAL
        
swish_WordList *
wordlist(self)
    swish_ParseData * self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS = _which_class("WordList");
        
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
#    swish_ParseData * self;
#    
#    CODE:
#        SvREFCNT_dec( self->user_data );
        


# *************************************************************************************/

MODULE = SWISH::3		PACKAGE = SWISH::3::Config	

PROTOTYPES: disable

AV*
keys(self)
    swish_Config * self
    
    CODE:
        RETVAL = _get_xml2_hash_keys(self->conf);
    
    OUTPUT:
        RETVAL


# translate xml2 hashes into Perl hashes -- NOTE these are effectively read-only hashes
# you must use add() and delete() to actually write to the active config in memory
HV*
subconfig(self,key)
    swish_Config * self
    char * key
    
    PREINIT:
        xmlHashTablePtr sc;
        
    CODE:
        sc = swish_subconfig_hash(self, (xmlChar*)key);
        RETVAL = _xml2_hash_to_perl_hash(sc);

    OUTPUT:
        RETVAL

        

int
debug(self)
    swish_Config * self
    
    CODE:
        RETVAL = swish_debug_config(self);
        
    OUTPUT:
        RETVAL


# TODO: make ->stash into a HV*
swish_Config *
init(CLASS)
    char * CLASS;

    CODE:
        RETVAL = swish_init_config();
#        RETVAL->ref_cnt++;
        
    OUTPUT:
        RETVAL



void
add(self, conf_file)
    swish_Config * self
	char *	conf_file
    
    CODE:
        swish_add_config((xmlChar*)conf_file, self);
      
      
void
delete(self, key)
    swish_Config * self
    char * key
    
    CODE:
        warn("delete() not yet implemented\n");
        
void
subconfig_delete(self, key, subconf)
    swish_Config * self
    char * key
    xmlHashTablePtr subconf
    
    CODE:
        warn("subconfig_delete() not yet implemented\n");

void
DESTROY(self)
    swish_Config * self
    
    CODE:
        self->ref_cnt--;
        if (!self->ref_cnt)
        {
            swish_free_config(self);
        }
        
