MODULE = SWISH::3       PACKAGE = SWISH::3::Analyzer

PROTOTYPES: enable


swish_Analyzer *
new(CLASS, config)
    char*           CLASS;
    swish_Config*   config;
    
    PREINIT:
        HV* stash;

    CODE:
        //RETVAL = swish_init_analyzer((swish_Config*)sp_extract_ptr( (SV*)config ));
        RETVAL = swish_init_analyzer(config);
        RETVAL->ref_cnt++;
        stash = newHV();
        RETVAL->stash = newRV_inc((SV*)stash);
        
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
    case 1:  self->regex = ST(1);
             break;
             
    // TODO test refcnt
    // get_regex
    case 2:  RETVAL  = SvREFCNT_inc( self->regex );
             break;
             
    // TODO set token_handler   
        
    END_SET_OR_GET_SWITCH
}


void
DESTROY(self)
    swish_Analyzer* self
    
    CODE:
        self->ref_cnt--;
        
        if (SWISH_DEBUG) {
            warn("DESTROYing swish_Analyzer object %s  [%d] [ref_cnt = %d]", 
                SvPV(ST(0), PL_na), self, self->ref_cnt);
        }
        
        if (self->ref_cnt < 1) {
            swish_free_analyzer(self);
        }
        




# tokenize() from Perl space uses same C func as tokenizer callback
swish_WordList *
tokenize(self, str, ...)
    SV* self;
    SV* str;
    
    PREINIT:
        char* CLASS;
        xmlChar* metaname;   
        xmlChar* context;
        unsigned int word_pos;
        unsigned int offset;
        xmlChar* buf;
        
    CODE:
        CLASS       = WORDLIST_CLASS;
        metaname    = (xmlChar*)SWISH_DEFAULT_METANAME;   
        context     = (xmlChar*)SWISH_DEFAULT_METANAME;
        word_pos    = 0;
        offset      = 0;
        buf         = (xmlChar*)SvPV(str, PL_na);
        
        // TODO reimplement as hashref arg
                
        if (!SvUTF8(str))
        {
            if (swish_is_ascii(buf))
                SvUTF8_on(str);     /* flags original SV ?? */
            else
                croak("%s is not flagged as a UTF-8 string and is not ASCII", buf);
        }
        
        if ( items > 2 )
        {
            word_pos = (int)SvIV(ST(2));
            
            if ( items > 3 )
                offset = (int)SvIV(ST(3));
                
            if ( items > 4 )
                metaname = (xmlChar*)SvPV(ST(4), PL_na);
                
            if ( items > 5 )
                context = (xmlChar*)SvPV(ST(5), PL_na);
                
            //warn ("word_pos %d  offset %d  metaname %s  context %s\n", word_pos, offset, metaname, context );
                
        }
                        
        RETVAL = sp_tokenize(
                        (swish_Analyzer*)sp_extract_ptr(self),
                        buf,
                        word_pos,
                        offset,
                        metaname,
                        context
                        );
        
        RETVAL->ref_cnt++;
        
        /* TODO do we need to worry about free()ing metaname and context ?? */
                        
    OUTPUT:
        RETVAL



# tokenize_isw() uses native libswish3 tokenizer
swish_WordList *
tokenize_isw(self, str, ...)
    SV* self;
    SV* str;

    PREINIT:
        char* CLASS;
        xmlChar* metaname = (xmlChar*)SWISH_DEFAULT_METANAME;   
        xmlChar* context  = (xmlChar*)SWISH_DEFAULT_METANAME;
        unsigned int word_pos    = 0;
        unsigned int offset      = 0;
        xmlChar* buf = (xmlChar*)SvPV(str, PL_na);
        
    CODE:
        CLASS = WORDLIST_CLASS;
        
        if (!SvUTF8(str))
        {
            if (swish_is_ascii(buf))
                SvUTF8_on(str);     /* flags original SV ?? */
            else
                croak("%s is not flagged as a UTF-8 string and is not ASCII", buf);
        }
        
        if ( items > 2 )
        {
            word_pos = (int)SvIV(ST(2));
            
            if ( items > 3 )
                offset = (int)SvIV(ST(3));
                
            if ( items > 4 )
                metaname = (xmlChar*)SvPV(ST(4), PL_na);
                
            if ( items > 5 )
                context = (xmlChar*)SvPV(ST(5), PL_na);
                
        }
                
        swish_init_words(); /* in case it wasn't initialized elsewhere... */
        RETVAL = swish_tokenize(
                        (swish_Analyzer*)sp_extract_ptr(self),
                        buf,
                        word_pos,
                        offset,
                        metaname,
                        context
                        );
        
        RETVAL->ref_cnt++;
        
        /* TODO do we need to worry about free()ing metaname and context ?? */
                        
    OUTPUT:
        RETVAL

