MODULE = SWISH::3       PACKAGE = SWISH::3::Analyzer

PROTOTYPES: enable


swish_Analyzer *
new(CLASS, config)
    char*           CLASS;
    swish_Config*   config;
    
    CODE:
        RETVAL = swish_init_analyzer(config);
        RETVAL->ref_cnt++;
        RETVAL->stash = sp_Stash_new();
        
    OUTPUT:
        RETVAL


# accessors/mutators
void
_set_or_get(self, ...)
    swish_Analyzer* self;
ALIAS:
    set_regex           = 1
    get_regex           = 2
    set_token_handler   = 3
    get_token_handler   = 4
PREINIT:
    SV*             stash;
    SV*             RETVAL;
PPCODE:
{
    
    //warn("number of items %d for ix %d", items, ix);
    
    START_SET_OR_GET_SWITCH

    // set_regex
    case 1:  sp_SV_is_qr(ST(1));
             self->regex = ST(1);
             break;
             
    // get_regex
    case 2:  RETVAL  = SvREFCNT_inc( self->regex );
             break;
             
    // set token handler
    case 3:  sp_Stash_replace(self->stash, TOKEN_HANDLER_KEY, ST(1));
             break;
             
    // get token handler
    case 4:  if (!sp_hvref_exists(self->stash, TOKEN_HANDLER_KEY)) {
                croak("no token handler set");
             }
    
             RETVAL = sp_Stash_get(self->stash, TOKEN_HANDLER_KEY);
             break;
        
    END_SET_OR_GET_SWITCH
}


void
DESTROY(self)
    swish_Analyzer* self
    
    CODE:
        self->ref_cnt--;
        
        if (sp_hvref_exists(self->stash, TOKEN_HANDLER_KEY)) {
            //warn("token handler set");
        }
                
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_Analyzer object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
        if (self->ref_cnt < 1) {
            sp_Stash_destroy( self->stash );
            swish_free_analyzer(self);
        }
        
