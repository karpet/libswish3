MODULE = SWISH::3       PACKAGE = SWISH::3::MetaName

PROTOTYPES: enable

swish_MetaName *
new(CLASS, name)
    char * CLASS;
    SV * name;
    
    CODE:
        RETVAL = swish_init_metaname(swish_xstrdup( (xmlChar*)SvPV(name, PL_na) ));
        RETVAL->ref_cnt++;
        
    OUTPUT:
        RETVAL



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
        
        if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
            warn("DESTROYing swish_MetaName object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), (int)self, self->ref_cnt);
        }
        
        
        if (self->ref_cnt < 1) {
            swish_free_metaname(self);
        }
        


