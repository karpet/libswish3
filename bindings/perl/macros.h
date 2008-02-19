/* global debug var -- set with swish_init() */
extern int SWISH_DEBUG;

#define CONFIG_CLASS        "SWISH::3::Config"
#define CONFIG_CLASS_KEY    "sp_config_class"
#define ANALYZER_CLASS      "SWISH::3::Analyzer"
#define ANALYZER_CLASS_KEY  "sp_analyzer_class"
#define PARSER_CLASS        "SWISH::3::Parser"
#define PARSER_CLASS_KEY    "sp_parser_class"
#define DATA_CLASS          "SWISH::3::Data"
#define DATA_CLASS_KEY      "sp_data_class"
#define TOKEN_CLASS         "SWISH::3::Token"
#define WORDLIST_CLASS      "SWISH::3::WordList"
#define WORD_CLASS          "SWISH::3::Word"
#define DOC_CLASS           "SWISH::3::Doc"
#define SELF_KEY            "sp_self"
#define CONFIG_KEY          "sp_config"
#define ANALYZER_KEY        "sp_analyzer"
#define HANDLER_KEY         "sp_handler"
#define TOKENIZER_KEY       "sp_tokenizer"
#define PARSER_KEY          "sp_parser"
#define TOKEN_HANDLER_KEY   "sp_token_handler"


/* some nice XS macros from KS - thanks Marvin! */
#define START_SET_OR_GET_SWITCH \
    RETVAL = &PL_sv_undef; \
    /* if called as a setter, make sure the extra arg is there */ \
    if (ix % 2 == 1) { \
        if (items < 2) \
            croak("usage: $object->set_xxxxxx($val)"); \
    } \
    else { \
        if (items > 2) \
            croak("usage: $object->get_xxxxx()"); \
    } \
    switch (ix) {

#define END_SET_OR_GET_SWITCH \
    default: croak("Internal error. ix: %d", ix); \
             break; /* probably unreachable */ \
    } \
    if (ix % 2 == 0) { \
        XPUSHs( RETVAL ); \
        XSRETURN(1); \
    } \
    else { \
        XSRETURN(0); \
    }
