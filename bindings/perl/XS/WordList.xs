MODULE = SWISH::3		PACKAGE = SWISH::3::WordList

PROTOTYPES: enable
        
swish_Word *
next(self)
    swish_WordList* self
    
    PREINIT:
        char* CLASS;
    
    CODE:
        CLASS = WORD_CLASS;
        
        if (self->current == NULL) {
            self->current = self->head;
        }
        else {
            self->current = self->current->next;
        }
        RETVAL = self->current;
        
    OUTPUT:
        RETVAL


SV*
nwords(self)
    swish_WordList* self;
    
    CODE:
        RETVAL = newSViv( self->nwords );
        
    OUTPUT:
        RETVAL
    
    
void
debug(self)
    swish_WordList* self;
    
    CODE:
        swish_debug_wordlist( self );
        


void
DESTROY(self)
    swish_WordList* self
    
    CODE:
        self->ref_cnt--;
        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_WordList object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
        if (self->ref_cnt < 1) {
            swish_free_wordlist(self);
        }
