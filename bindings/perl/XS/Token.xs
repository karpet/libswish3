MODULE = SWISH::3       PACKAGE = SWISH::3::Token

PROTOTYPES: enable

void
debug(self)
    sp_Token* self;
    
    CODE:
        sp_debug_token(self);
        
        
