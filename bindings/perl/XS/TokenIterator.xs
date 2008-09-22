MODULE = SWISH::3		PACKAGE = SWISH::3::TokenIterator

PROTOTYPES: enable


swish_Token*
next(self)
    swish_TokenIterator *self;
    
    PREINIT:
        char* CLASS;
        
    CODE:
        CLASS  = TOKEN_CLASS;
        //warn("calling next token");
        RETVAL = swish_next_token( self );
        //warn("got next token %d", RETVAL);
        if (RETVAL)
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
        
