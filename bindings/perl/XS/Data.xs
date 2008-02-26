MODULE = SWISH::3		PACKAGE = SWISH::3::Data

PROTOTYPES: enable


SV*
s3(self)
    swish_ParserData *self;
    
    PREINIT:
        char    *class;
        swish_3 *s3;

    CODE:
        self->s3->ref_cnt++;
        class  = sp_hvref_fetch_as_char((SV*)self->s3->stash, SELF_CLASS_KEY);
        warn("s3 class = %s\n", class);
        RETVAL = sp_bless_ptr( class, (IV)self->s3 );
        
    OUTPUT:
        RETVAL
        
    CLEANUP:
        SvREFCNT_inc( RETVAL );


swish_Config*
config(self)
    swish_ParserData* self
    
	PREINIT:
        char* CLASS;

    CODE:
        CLASS  = sp_hvref_fetch_as_char(self->s3->stash, CONFIG_CLASS_KEY);
        self->s3->config->ref_cnt++;
        RETVAL = self->s3->config;
        
    OUTPUT:
        RETVAL
        
        
SV*
property(self, p)
    swish_ParserData* self;
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
    swish_ParserData* self;
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
    swish_ParserData* self
    
    CODE:
        RETVAL = sp_nb_to_hash( self->properties );
        
    OUTPUT:
        RETVAL
        

HV*
metanames(self)
    swish_ParserData* self
    
    CODE:
        RETVAL = sp_nb_to_hash( self->metanames );
        
    OUTPUT:
        RETVAL
       


swish_DocInfo *
doc(self)
    swish_ParserData* self
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS  = DOC_CLASS;
        RETVAL = self->docinfo;
        
    OUTPUT:
        RETVAL


swish_WordList *
wordlist(self)
    swish_ParserData* self
    
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
        
