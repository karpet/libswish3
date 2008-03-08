MODULE = SWISH::3       PACKAGE = SWISH::3::MetaName

PROTOTYPES: enable

SV*
id (self)
	swish_MetaName *self;
       
    CODE:
        RETVAL = newSViv( self->id );
        
    OUTPUT:
        RETVAL
        

SV*
name (self)
	swish_MetaName *self;
    
    CODE:
        RETVAL = newSVpvn( (char*)self->name, strlen((char*)self->name) );
        
    OUTPUT:
        RETVAL
        
SV*
bias (self)
	swish_MetaName *self;
    
    CODE:
        RETVAL = newSViv( self->bias );
        
    OUTPUT:
        RETVAL
        
        
SV*
alias_for (self)
    swish_MetaName *self;
    
    CODE:
        RETVAL = newSVpvn( (char*)self->alias_for, strlen((char*)self->alias_for) );

    OUTPUT:
        RETVAL
        

void
DESTROY (self)
    swish_MetaName *self;
    
    CODE:
        self->ref_cnt--;
        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_MetaName object %s  [0x%x] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), (int)self, self->ref_cnt);
        }
        
        
        if (self->ref_cnt < 1) {
            swish_free_metaname(self);
        }
        


