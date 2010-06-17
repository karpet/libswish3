MODULE = SWISH::3       PACKAGE = SWISH::3::MetaNameHash

PROTOTYPES: enable

SV*
get(self, key)
    xmlHashTablePtr self;
    xmlChar* key;
    
    PREINIT:
        swish_MetaName* meta;
        
    CODE:
        meta    = swish_hash_fetch(self, key);
        meta->ref_cnt++;
        RETVAL  = sp_bless_ptr(METANAME_CLASS, meta);
        SvREFCNT_inc(RETVAL);
        
    OUTPUT:
        RETVAL


void
set(self, meta)
    xmlHashTablePtr self;
    swish_MetaName* meta;
    
    CODE:
        swish_hash_replace(self, meta->name, meta);


AV*
keys(self)
    xmlHashTablePtr self;
            
    CODE:
        RETVAL = sp_get_xml2_hash_keys(self);
        
    OUTPUT:
        RETVAL
