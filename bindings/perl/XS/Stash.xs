MODULE = SWISH::3       PACKAGE = SWISH::3::Stash

PROTOTYPES: enable

void
DESTROY(self)
    SV *self;
    
    CODE:

        if (SWISH_DEBUG) {
            warn("DESTROYing Stash object %s  [%d]", 
                SvPV(ST(0), PL_na), self);
            
        }

