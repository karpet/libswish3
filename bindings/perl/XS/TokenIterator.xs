MODULE = SWISH::3		PACKAGE = SWISH::3::TokenIterator

PROTOTYPES: enable


swish_Token*
next(self)
    swish_TokenIterator *self;
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS  = TOKEN_CLASS;
        RETVAL = swish_next_token( self );
        RETVAL->ref_cnt++;
        
    OUTPUT:
        RETVAL
        
void
DESTROY(self)
    swish_TokenIterator* self
    
    CODE:
        self->ref_cnt--;
                        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_TokenIterator object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
        if (self->ref_cnt < 1) {
            swish_free_token_iterator(self);
        }
        
