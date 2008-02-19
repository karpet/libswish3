MODULE = SWISH::3		PACKAGE = SWISH::3::Config	

PROTOTYPES: enable

AV*
keys(self)
    swish_Config* self
    
    CODE:
        RETVAL = sp_get_xml2_hash_keys(self->conf);
    
    OUTPUT:
        RETVAL


# translate xml2 hashes into Perl hashes -- NOTE these are effectively read-only hashes
# you must use add() and delete() to actually write to the active config in memory
HV*
properties(self)
    swish_Config* self
    
    CODE:
        RETVAL = sp_get_config_subconfig( self, SWISH_PROP );

    OUTPUT:
        RETVAL

HV*
metanames(self)
    swish_Config *self
    
    CODE:
        RETVAL = sp_get_config_subconfig( self, SWISH_META );
        
    OUTPUT:
        RETVAL

HV*
mimes(self)
    swish_Config* self;
    
    CODE:
        RETVAL = sp_get_config_subconfig( self, SWISH_MIME );
        
    OUTPUT:
        RETVAL


HV*
parsers(self)
    swish_Config* self;
    
    CODE:
        RETVAL = sp_get_config_subconfig( self, SWISH_PARSERS );
        
    OUTPUT:
        RETVAL
 

int
debug(self)
    swish_Config* self
    
    CODE:
        RETVAL = swish_debug_config(self);
        
    OUTPUT:
        RETVAL


swish_Config *
new(CLASS)
    char* CLASS;
    
    CODE:
        RETVAL = sp_new_config();
        
    OUTPUT:
        RETVAL



void
add(self, conf_file)
    swish_Config* self
	char *	conf_file
    
    CODE:
        swish_add_config((xmlChar*)conf_file, self);
      
      
void
delete(self, key)
    swish_Config* self
    char* key
    
    CODE:
        warn("delete() not yet implemented\n");
        
void
subconfig_delete(self, key, subconf)
    swish_Config* self
    char* key
    xmlHashTablePtr subconf
    
    CODE:
        warn("subconfig_delete() not yet implemented\n");

void
DESTROY(self)
    swish_Config* self
    
    CODE:
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_Config object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }

        if (self->stash != NULL) {
            if (SWISH_DEBUG) {
                warn("decreasing refcnt on config->stash [currently %d]",
                SvREFCNT((SV*)SvRV((SV*)self->stash))
                );
            }
            SvREFCNT_dec(self->stash);
            self->stash = NULL;
        }

        
int
refcount(obj)
    SV* obj;
    
    CODE:
        RETVAL = SvREFCNT((SV*)SvRV(obj));
    
    OUTPUT:
        RETVAL
        
        
