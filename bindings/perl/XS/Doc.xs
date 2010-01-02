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

   
SV*
action(self)
	swish_DocInfo *	self;
    CODE:
        if (self->action != NULL) {
            RETVAL = newSVpvn( (char*)self->action, strlen((char*)self->action) );
        }
        else {
            RETVAL = newSVpvn( "", 0 );
        }        
    OUTPUT:
        RETVAL


void
DESTROY (self)
    swish_DocInfo * self;

    CODE:
        self->ref_cnt--;

        if (SWISH_DEBUG) {
            warn("DESTROY %s [%ld] [ref_cnt = %d]",
                SvPV(ST(0), PL_na), (long)self, self->ref_cnt);
        }

        // freed by parser_data


