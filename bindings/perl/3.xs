/*
 * Standard XS greeting.
 */
#ifdef __cplusplus
extern "C" {
#endif
#define PERL_NO_GET_CONTEXT 
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

#include <stdarg.h>
#include <libswish3.h>

/* global debug var -- set with swish_init() */
extern int SWISH_DEBUG;


/* some nice XS macros from KS - thanks Marvin! */
#define START_SET_OR_GET_SWITCH \
    SV *RETVAL = &PL_sv_undef; \
    /* if called as a setter, make sure the extra arg is there */ \
    if (ix % 2 == 1) { \
        if (items < 2) \
            croak("usage: $object->set_xxxxxx($val)"); \
    } \
    else { \
        if (items > 2) \
            croak("usage: $object->get_xxxxx()"); \
    } \
    switch (ix) {

#define END_SET_OR_GET_SWITCH \
    default: croak("Internal error. ix: %d", ix); \
             break; /* probably unreachable */ \
    } \
    if (ix % 2 == 0) { \
        XPUSHs( RETVAL ); \
        XSRETURN(1); \
    } \
    else { \
        XSRETURN(0); \
    }



/* keep track of all subclasses in C space so we can call handlers easily 
 * NOTE this requires that when subclassing, all classes must be subclassed
 */

/* all XS stuff is prefixed with 'sp_' for Swish Perl */

#define DEFAULT_BASE_CLASS  "SWISH::3::Parser"
#define CONFIG_CLASS        "SWISH::3::Config"
#define ANALYZER_CLASS      "SWISH::3::Analyzer"
#define CONFIG_KEY          "sp_config"
#define ANALYZER_KEY        "sp_analyzer"
#define HANDLER_KEY         "sp_handler"


static HV * SubClasses       = (HV*)NULL;
static int nClasses          = 5;   /* match Classes[] ?? */
static char * Classes[]      = { 
        "Doc", 
        "Word",
        "WordList",
        "Property", 
        "Data"
         };


/* private functions */

static void sp_make_subclasses( char * class )
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

/* store SV* in a hash, incrementing its refcnt */
static SV*
sp_hv_store( HV* h, const char* key, SV* val)
{
    dTHX;
    SV** ok;
    ok = hv_store(h, key, strlen(key), SvREFCNT_inc(val), 0);
    if (ok != NULL)
    {
        //warn("stored %s ok in hash: %s", key, SvPV( *ok, PL_na ));
    }
    else
    {
        croak("failed to store %s in hash", key);
    }
    return *ok;
}

/* fetch SV* from hash */
static SV*
sp_hv_fetch( HV* h, const char* key )
{
    SV** ok;
    ok = hv_fetch(h, key, strlen(key), 0);
    if (ok != NULL)
    {
        //warn("fetched %s ok from hash: %s", key, SvPV( *ok, PL_na ));
    }
    else
    {
        //croak("failed to fetch %s from hash", key);
    }
    return *ok;
}

/* delete SV* from hash, returning the deleted SV* */
static SV*
sp_hv_delete( HV* h, const char* key )
{
    dTHX;
    SV* oldval;
    oldval = hv_delete(h, key, strlen(key), 0 );
    if (oldval != NULL)
    {
        //warn("deleted %s ok from hash: %s", key, SvPV( oldval, PL_na ));
    }
    else
    {
        croak("failed to delete %s from hash", key);
    }
    return oldval;
}


/* make a Perl blessed object from a C pointer */
static SV * sp_ptr_to_object( char* CLASS, IV data )
{
    dTHX;
    SV* obj = sv_newmortal();
    sv_setref_pv(obj, CLASS, (void*)data);
    return obj;
}

/* what class is an object blessed into ? */
static char * sp_get_objects_class( SV* object )
{
    dTHX;
    char * class = sv_reftype(SvRV(object), 1);
    //warn("object belongs to %s\n", class);
    return class;
}

static HV * sp_is_hash_object( SV* object )
{
    dTHX;
    HV * hash = NULL;
    char * class = sp_get_objects_class( object );
    if (SvROK(object) && SvTYPE(SvRV(object))==SVt_PVHV)
        hash = (HV*)SvRV(object);
    else if (SvROK(object) && SvTYPE(SvRV(object))==SVt_PVMG)
        croak("%s is a magic reference not a hash reference", class);
    else
        croak("%s is reference but not a hash reference", class);
        
    return hash;
}

static void sp_dump_hash(SV* hash_ref) 
{
        HV* hash;
        HE* hash_entry;
        int num_keys, i;
        SV* sv_key;
        SV* sv_val;
        int refcnt;
    
        if (SvTYPE(SvRV(hash_ref))!=SVt_PVHV)
            croak("hash_ref is not a hash reference");
    
        hash = (HV*)SvRV(hash_ref);
        num_keys = hv_iterinit(hash);
        for (i = 0; i < num_keys; i++) {
            hash_entry = hv_iternext(hash);
            sv_key = hv_iterkeysv(hash_entry);
            sv_val = hv_iterval(hash, hash_entry);
            refcnt = SvREFCNT(sv_val);
            warn("%s => %s  [%d]\n", SvPV(sv_key, PL_na), SvPV(sv_val, PL_na), refcnt);
        }
        return;
}

static void sp_describe_object( SV* object )
{
    dTHX;
    warn("describing object\n");
    char * str = SvPV( object, PL_na );
    if (SvROK(object))
    {
      if (SvTYPE(SvRV(object))==SVt_PVHV)
        warn("%s is a magic blessed reference\n", str);
      else if (SvTYPE(SvRV(object))==SVt_PVMG)
        warn("%s is a magic reference", str);
      else if (SvTYPE(SvRV(object))==SVt_IV)
        warn("%s is a IV reference (pointer)", str); 
      else
        warn("%s is a reference of some kind", str);
    }
    else
    {
        warn("%s is not a reference", str);
        if (sv_isobject(object))
            warn("however, %s is an object", str);
        
        
    }
    Perl_sv_dump( object );
    Perl_sv_dump( (SV*)SvRV(object) );
    sp_dump_hash( object );
}

/* return the C pointer from a Perl blessed O_OBJECT */
static IV sp_ptr_from_object( SV* object )
{
    dTHX;
    return SvIV((SV*)SvRV( object ));
}

/* lookup the class name from the global hash */
static char * sp_which_class( char * c )
{
    dTHX;
    if (SubClasses == (HV*)NULL)
        sp_make_subclasses(DEFAULT_BASE_CLASS);
        
    SV** sv = hv_fetch( SubClasses, c, strlen(c), 0 );
    if ( !sv )
        croak("could not fetch %s class from SubClasses", c);
        
    return SvPV(*sv, PL_na);
}

/* fetch a hash value from an object (i.e. a generic accessor) */
static SV * sp_get_object_key( SV* object, char * name )
{
    dTHX;
    char * class = sp_get_objects_class( object );
    //warn("looking for %s in %s\n", name, class);
    HV* hash = sp_is_hash_object( object );
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




void sp_test_handler( swish_ParseData * parse_data )
{
    dTHX;
    warn("handler called!\n");
    swish_debug_docinfo( parse_data->docinfo );
    swish_debug_wordlist( parse_data->wordlist );
    swish_debug_nb( parse_data->properties, "Property" );
    swish_debug_nb( parse_data->metanames, "MetaName" );
    warn("\n");
}

/* C wrapper for our Perl handler.
   the parser object is passed in the parse_data stash.
   we dereference it, pull out the SV* CODE ref, and execute
   the Perl code.
*/
void sp_handler( swish_ParseData* parse_data )
{
    dTHX;
    dSP;

    char*           class   = sp_which_class("Data");
    SV*             obj     = sp_ptr_to_object(class, (IV)parse_data);
    swish_Parser*   parser  = (swish_Parser*)sp_ptr_from_object(parse_data->stash);
    HV*             stash   = (HV*)SvRV((HV*)parser->stash);
    SV*             handler = sp_hv_fetch(stash, HANDLER_KEY);
    
    PUSHMARK(SP);
    XPUSHs(obj);
    PUTBACK;

    call_sv(handler, G_DISCARD);
}


/* this regex wizardry cribbed from KS - thanks Marvin! */
swish_WordList *
sp_tokenize(swish_Analyzer * analyzer, xmlChar * str, ...)
{
    dTHX;
    unsigned int wpos, offset, num_code_points;
    xmlChar *meta, *ctxt;
    SV *token_re;
    swish_WordList *list;
    swish_WordList *(*token_handler)(xmlChar * start_ptr, int tok_bytes, int start, int end, ...);
    va_list args;
    va_start(args, str);
    wpos    = va_arg(args, unsigned int);
    offset  = va_arg(args, unsigned int);    
    meta    = va_arg(args, xmlChar *);
    ctxt    = va_arg(args, xmlChar *);
    va_end(args);
    
    MAGIC      *mg              = NULL;
    REGEXP     *rx              = NULL;
    SV         *wrapper         = sv_newmortal();
    xmlChar    *str_start       = str;
    int         str_len         = strlen((char*)str);
    xmlChar    *str_end         = str_start + str_len;
    
    token_re        = analyzer->regex;
    token_handler   = analyzer->stash;
    
    /* extract regexp struct from qr// entity */
    if (SvROK(token_re)) {
        SV *sv = SvRV(token_re);
        if (SvMAGICAL(sv))
            mg = mg_find(sv, PERL_MAGIC_qr);
    }
    if (!mg)
        croak("not a qr// entity");
    rx = (REGEXP*)mg->mg_obj;

    /* fake up an SV wrapper to feed to the regex engine */
    sv_upgrade(wrapper, SVt_PV);
    SvREADONLY_on(wrapper);
    SvLEN(wrapper) = 0;
    SvUTF8_on(wrapper);     /* do UTF8 matching -- we trust str is already utf-8 encoded. */
    
    /* wrap the string in an SV to please the regex engine */
    SvPVX(wrapper) = str_start;
    SvCUR_set(wrapper, str_len);
    SvPOK_on(wrapper);

    list = swish_init_wordlist();
    num_code_points = 0;
    
    while ( pregexec(rx, str, str_end, str, 1, wrapper, 1) ) 
    {
        xmlChar * start_ptr = str + rx->startp[0];
        xmlChar * end_ptr   = str + rx->endp[0];
        int start, end, tok_bytes, tok_pts;

        /* get start and end offsets in Unicode code points */
        for( ; str < start_ptr; num_code_points++) 
        {
            str += swish_utf8_chr_len(str);
            if (str > str_end)
                croak("scanned past end of '%s'", str_start);
        }
        
        start = num_code_points;
        
        for( ; str < end_ptr; num_code_points++) 
        {
            str += swish_utf8_chr_len(str);
            if (str > str_end)
                croak("scanned past end of '%s'", str_start);
        }
            
        end = num_code_points;          /* characters (codepoints) */
            
        tok_pts   = end - start;
        tok_bytes = end_ptr - start_ptr;
                
        (*token_handler)(   start_ptr, 
                            tok_bytes, 
                            start, 
                            end, 
                            meta, 
                            ctxt, 
                            ++wpos,
                            (tok_bytes + offset - 1),
                            analyzer,
                            list
                            );
        
    } 

    return list;
}

/*
    default token handler is just to append to WordList
*/
swish_WordList*
sp_token_handler(xmlChar * start_ptr, int tok_bytes, int start, int end, ...)
{
    xmlChar *meta, *ctxt;
    int wpos, offset;
    swish_Analyzer *analyzer;
    swish_WordList *list;
    va_list args;
    va_start(args, end);
    meta        = va_arg(args, xmlChar *);
    ctxt        = va_arg(args, xmlChar *);
    wpos        = va_arg(args, unsigned int);
    offset      = va_arg(args, unsigned int);    
    analyzer    = va_arg(args, swish_Analyzer*);
    list        = va_arg(args, swish_WordList*);
    va_end(args);
    
    if ((end - start) < analyzer->minwordlen)
        return list;
            
    if ((end - start) > analyzer->maxwordlen)
        return list;
        
        
    /* TODO: lc() and stem() */
            
    swish_add_to_wordlist_len(  list, 
                                start_ptr, 
                                tok_bytes, 
                                meta, 
                                ctxt, 
                                wpos, 
                                offset
                                );

    return list;
}

/*******************************************************************************

    end our native C helpers, start the XS
    
********************************************************************************/


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
             
# MUST call this before creating a SWISH::3::Config object
void
_init_swish(class)
    char * class
    
    CODE:
        swish_init();
        
      
swish_Parser *
_init(CLASS, config, analyzer, handler)
    char * CLASS
    SV * config
    SV * analyzer
    SV * handler
       
# cache all the passed in objects in our stash
# and then just return them via accessors, rather
# than creating new objects each time. helps with ref_cnt sanity.

    PREINIT:
        HV* stash;
        
	CODE:
        stash = newHV();
	    sp_make_subclasses(CLASS);
        sp_hv_store(stash, CONFIG_KEY,    config);
        sp_hv_store(stash, ANALYZER_KEY,  analyzer);
        sp_hv_store(stash, HANDLER_KEY,   handler);
        RETVAL = swish_init_parser(
                        (swish_Config*)sp_ptr_from_object(config),
                        (swish_Analyzer*)sp_ptr_from_object(analyzer),
                        &sp_handler,
                        (void*)newRV_inc((SV*)stash));
                        
        RETVAL->ref_cnt++;
        //sp_describe_object(RETVAL->stash);
        
        
    OUTPUT:
        RETVAL

        
       
void
DESTROY(self)
    swish_Parser * self
    
    PREINIT:
        HV* stash;
           
    CODE:
        if (SWISH_DEBUG)
        {
            warn("DESTROYing parser %d", self);
            warn("freeing parser stash");
        }
        
        //sp_describe_object(self->stash);
        stash = (HV*)SvRV((HV*)self->stash);
        hv_undef(stash);
        
        self->ref_cnt--;
        if (self->ref_cnt < 1)
        {
            if (SWISH_DEBUG)
                warn("freeing parser %d", self);
                
            swish_free_parser(self);
            swish_cleanup();
        }



SV*
slurp_file(self, filename)
    SV * self;
    char * filename;
    
    PREINIT:
        xmlChar * buf;
    
    CODE:
        buf = swish_slurp_file((xmlChar*)filename);
        RETVAL = newSVpv( (const char*)buf, 0 );
        swish_xfree(buf);
        
    OUTPUT:
        RETVAL
        


int
parse_file (self, filename)
    SV* self;
	SV*	filename;
    
    PREINIT:
        char * file;
        
    CODE:
        file = SvPV(filename, PL_na);
        SvREFCNT_inc(self);
        
# need to swap return values to make it Perlish
        RETVAL = swish_parse_file(  (swish_Parser*)sp_ptr_from_object(self),
                                    (xmlChar*)file,
                                    (void*)self
                                    ) 
                ? 0 
                : 1;
                
        SvREFCNT_dec(self);
                        
    OUTPUT:
        RETVAL
        
     
int
parse_buf (self, buffer)
    SV* self;
    SV* buffer;
    
    PREINIT:
        char * buf;
        
    CODE:
        buf     = SvPV(buffer, PL_na);
        SvREFCNT_inc(self);


# need to swap return values to make it Perlish
        RETVAL = swish_parse_buffer((swish_Parser*)sp_ptr_from_object(self),
                                    (xmlChar*)buf,
                                    (void*)self
                                    ) 
                ? 0
                : 1;
                
        SvREFCNT_dec(self);
                
    OUTPUT:
        RETVAL
        
    
# parser accessor/mutators
void
_set_or_get(self, ...)
    swish_Parser * self;
ALIAS:
    set_config       = 1
    get_config       = 2
    set_analyzer     = 3
    get_analyzer     = 4
    set_handler      = 5
    get_handler      = 6
    set_stash        = 7
    get_stash        = 8
PREINIT:
    HV* stash;
    SV* oldval;
    SV* newval;
    swish_Config * conf;
    swish_Analyzer * ana;
    char * skey;
PPCODE:
{
    stash = (HV*)SvRV((HV*)self->stash);
    
    START_SET_OR_GET_SWITCH

    case 1:  
             oldval = sp_hv_delete(stash, CONFIG_KEY);
             conf = (swish_Config*)sp_ptr_from_object(oldval);
             conf->ref_cnt--;
             
             newval = sp_hv_store(stash, CONFIG_KEY, ST(1));
             self->config = (swish_Config*)sp_ptr_from_object(newval);
             break;

    case 2:  RETVAL = sp_hv_fetch(stash, CONFIG_KEY);
             break;

    case 3:  
             oldval = sp_hv_delete(stash, ANALYZER_KEY);             
             ana = (swish_Analyzer*)sp_ptr_from_object(oldval);
             ana->ref_cnt--;
             
             newval = sp_hv_store(stash, ANALYZER_KEY, ST(1));
             self->analyzer = (swish_Analyzer*)sp_ptr_from_object(newval);
             break;

    case 4:  RETVAL = sp_hv_fetch(stash, ANALYZER_KEY);
             break;

    case 5:  
             oldval = sp_hv_delete(stash, HANDLER_KEY);
             sp_hv_store(stash, HANDLER_KEY, ST(1));
             break;

    case 6:  RETVAL = sp_hv_fetch(stash, HANDLER_KEY);
             break;
    
# first val is key, second is value.         
    case 7:  
             skey   = SvPV(ST(1), PL_na);
#             oldval = sp_hv_delete(stash, skey);
             sp_hv_store(stash, skey, ST(2));
             break;

    case 8:  skey   = SvPV(ST(1), PL_na);
             RETVAL = sp_hv_fetch(stash, skey);
             break;

    
    END_SET_OR_GET_SWITCH
}


# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::Word

PROTOTYPES: enable

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

PROTOTYPES: enable

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

PROTOTYPES: enable

        

# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::WordList

PROTOTYPES: enable
        
swish_Word *
next(self)
    swish_WordList * self
    
    PREINIT:
        char * CLASS;
    
    CODE:
        CLASS = sp_which_class("Word");
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


SV*
nwords(self)
    swish_WordList * self;
    
    CODE:
        RETVAL = newSViv( self->nwords );
        
    OUTPUT:
        RETVAL
    
    

void
DESTROY(self)
    swish_WordList * self
    
    CODE:
        self->ref_cnt--;
        if (self->ref_cnt < 1)
        {
            swish_free_wordlist(self);
        }
        


# *******************************************************************************
    
MODULE = SWISH::3		PACKAGE = SWISH::3::Parser::Data

PROTOTYPES: enable


SV*
parser(self)
    swish_ParseData * self
            
    CODE:
        RETVAL = self->stash;
        
    OUTPUT:
        RETVAL
        
    CLEANUP:
        SvREFCNT_inc(RETVAL);


swish_Config *
config(self)
    swish_ParseData * self
    
	PREINIT:
        char* CLASS = CONFIG_CLASS;

    CODE:
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
        buf = xmlHashLookup(self->properties->hash, p);
        RETVAL = newSVpvn((char*)xmlBufferContent(buf), xmlBufferLength(buf));
        
    OUTPUT:
        RETVAL
        

swish_DocInfo *
doc(self)
    swish_ParseData * self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS = sp_which_class("Doc");
        RETVAL = self->docinfo;
        
    OUTPUT:
        RETVAL
        
swish_WordList *
wordlist(self)
    swish_ParseData * self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS = sp_which_class("WordList");
        
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

PROTOTYPES: enable

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
        RETVAL->ref_cnt++;

        
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
        if (SWISH_DEBUG)
        {
            warn("DESTROYing swish_Config object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
        self->ref_cnt--;
        if (self->ref_cnt < 1)
        {
            if (SWISH_DEBUG)
                warn("freeing swish_Config %d", self);
                
            swish_free_config(self);
        }
        
# *******************************************************************************

MODULE = SWISH::3       PACKAGE = SWISH::3::Analyzer

PROTOTYPES: enable

swish_Analyzer *
init(CLASS, config, regex)
    char * CLASS;
    SV * config;
    SV * regex;

    CODE:
        RETVAL = swish_init_analyzer( (swish_Config*)sp_ptr_from_object(config) );
        RETVAL->ref_cnt++;
        RETVAL->regex = (void*)SvREFCNT_inc( regex );
        RETVAL->tokenizer = &sp_tokenize;
        RETVAL->stash = &sp_token_handler;
        
    OUTPUT:
        RETVAL



void
DESTROY(self)
    swish_Analyzer * self
    
    CODE:
        if (SWISH_DEBUG)
        {
            warn("DESTROYing swish_Analyzer object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
                
        self->ref_cnt--;
        if (self->ref_cnt < 1)
        {
            if(SWISH_DEBUG)
                warn("freeing swish_Analyzer %d", self);
                
            swish_free_analyzer(self);
        }
         

# tokenize() from Perl space uses same C func as tokenizer callback
swish_WordList *
tokenize(self, str, ...)
    SV * self;
    SV * str;
    
    PREINIT:
        char * CLASS;
        xmlChar * metaname = SWISH_DEFAULT_METANAME;   
        xmlChar * context  = SWISH_DEFAULT_METANAME;
        unsigned int word_pos    = 0;
        unsigned int offset      = 0;
        xmlChar * buf = SvPV(str, PL_na);
        
    CODE:
        CLASS = sp_which_class("WordList");
        
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
                metaname = SvPV(ST(4), PL_na);
                
            if ( items > 5 )
                context = SvPV(ST(5), PL_na);
                
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


# TODO: get/set methods, including way to set tokenizer func ref


# tokenize_isw() uses native libswish3 tokenizer
swish_WordList *
tokenize_isw(self, str, ...)
    SV * self;
    SV * str;

    PREINIT:
        char * CLASS;
        xmlChar * metaname = SWISH_DEFAULT_METANAME;   
        xmlChar * context  = SWISH_DEFAULT_METANAME;
        unsigned int word_pos    = 0;
        unsigned int offset      = 0;
        xmlChar * buf = SvPV(str, PL_na);
        
    CODE:
        CLASS = sp_which_class("WordList");
        
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
                metaname = SvPV(ST(4), PL_na);
                
            if ( items > 5 )
                context = SvPV(ST(5), PL_na);
                
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
        
