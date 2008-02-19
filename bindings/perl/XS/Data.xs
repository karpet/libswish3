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
        
SV*
metaname(self, m)
    swish_ParseData* self;
    xmlChar* m;
    
	PREINIT:
        xmlBufferPtr buf;
        
    CODE:
        buf = xmlHashLookup(self->properties->hash, m);
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
        CLASS  = DOC_CLASS;
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
        
# MUST increment refcnt 2x so that SWISH::3::WordList::DESTROY
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
        

