MODULE = SWISH::3       PACKAGE = SWISH::3::Token

PROTOTYPES: enable

SV*
value (self)
	swish_Token *	self;
    
    PREINIT:
        xmlChar *value;
          
    CODE:
        value = swish_get_token_value(self);
        RETVAL = newSVpvn( (char*)value, strlen((char*)value) );
        
    OUTPUT:
        RETVAL
        

swish_MetaName*
meta (self)
	swish_Token *	self;
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS  = METANAME_CLASS;
        RETVAL = self->meta;
        
    OUTPUT:
        RETVAL
        
SV*
context (self)
	swish_Token *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->context, strlen((char*)self->context) );
        
    OUTPUT:
        RETVAL
        

SV*
pos (self)
	swish_Token *	self;
    CODE:
        RETVAL = newSViv( self->pos );
        
    OUTPUT:
        RETVAL

SV*
start_byte (self)
	swish_Token *	self;
    CODE:
        RETVAL = newSViv( self->start_byte );
        
    OUTPUT:
        RETVAL

SV*
len(self)
	swish_Token *	self;
    CODE:
        RETVAL = newSViv( self->len );
        
    OUTPUT:
        RETVAL


void
DESTROY(self)
    swish_Token* self
    
    CODE:
        self->ref_cnt--;
                        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_Token object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
        if (self->ref_cnt < 1) {
            swish_free_token(self);
        }
        
