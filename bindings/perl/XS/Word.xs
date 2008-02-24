MODULE = SWISH::3		PACKAGE = SWISH::3::Word

PROTOTYPES: enable

SV*
word (self)
	swish_Word *	self;   
    CODE:
        RETVAL = newSVpvn( (char*)self->word, strlen((char*)self->word) );
        
    OUTPUT:
        RETVAL
        

SV*
metaname (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->metaname, strlen((char*)self->metaname) );
        
    OUTPUT:
        RETVAL
        
SV*
context (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSVpvn( (char*)self->context, strlen((char*)self->context) );
        
    OUTPUT:
        RETVAL
        

SV*
position (self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->position );
        
    OUTPUT:
        RETVAL

SV*
start_offset(self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->start_offset );
        
    OUTPUT:
        RETVAL

SV*
end_offset(self)
	swish_Word *	self;
    CODE:
        RETVAL = newSViv( self->end_offset );
        
    OUTPUT:
        RETVAL

