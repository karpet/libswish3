MODULE = SWISH::3       PACKAGE = SWISH::3::xml2Hash

PROTOTYPES: enable

SV*
get(self, key)
    xmlHashTablePtr self;
    xmlChar* key;
    
    PREINIT:
        xmlChar* value;
        
    CODE:
        value  = swish_hash_fetch(self, key);
        RETVAL = newSVpvn((char*)value, xmlStrlen(value));
        SvREFCNT_inc(RETVAL);
        
    OUTPUT:
        RETVAL


void
set(self,key,value)
    xmlHashTablePtr self;
    xmlChar *key;
    xmlChar *value;
    
    CODE:
        swish_hash_replace(self, key, value);
        

AV*
keys(self)
    xmlHashTablePtr self;
            
    CODE:
        RETVAL = sp_get_xml2_hash_keys(self);
        
    OUTPUT:
        RETVAL
