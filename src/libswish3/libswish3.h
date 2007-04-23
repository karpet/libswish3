/*
 * This file is part of libswish3
 * Copyright (C) 2007 Peter Karman
 *
 *  libswish3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libswish3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libswish3; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef __LIBSWISH3_H__
#define __LIBSWISH3_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/hash.h>


#define SWISH_LIB_VERSION         "0.1.0"
#define SWISH_VERSION             "3.0.0"
#define SWISH_BUFFER_CHUNK_SIZE   10000
#define SWISH_MAXSTRLEN           2000
#define SWISH_MAX_HEADERS         6
#define SWISH_RD_BUFFER_SIZE      65356
#define SWISH_MAX_WORD_LEN        256
#define SWISH_MIN_WORD_LEN        1

#define SWISH_STACK_SIZE          255  /* starting size for metaname/tag stack */


#define SWISH_CONTRACTIONS         1

#define SWISH_SPECIAL_ARG         1
#define SWISH_MAX_SORT_STRING_LEN 100

#define SWISH_DATE_FORMAT_STRING   "%Y-%m-%d %H:%M:%S %Z"
#define SWISH_URL_LENGTH           255

/* default config hash key names */
#define SWISH_INCLUDE_FILE   "IncludeConfigFile"
#define SWISH_PROP           "PropertyNames"
#define SWISH_PROP_MAX       "PropertyNamesMaxLength"
#define SWISH_PROP_SORT      "PropertyNamesSortKeyLength"
#define SWISH_META           "MetaNames"
#define SWISH_MIME           "MIME"
#define SWISH_PARSERS        "Parsers"
#define SWISH_INDEX          "Index"
#define SWISH_ALIAS          "TagAlias"
#define SWISH_WORDS          "Words"
#define SWISH_DEFAULT_PARSER "default"
#define SWISH_PARSER_TXT     "TXT"
#define SWISH_PARSER_XML     "XML"
#define SWISH_PARSER_HTML    "HTML"
#define SWISH_DEFAULT_PARSER_TYPE      "HTML"
#define SWISH_INDEX_FORMAT   "Format"
#define SWISH_INDEX_NAME     "Name"
#define SWISH_INDEX_LOCALE   "Locale"
#define SWISH_DEFAULT_VALUE  "1"
#define SWISH_PARSE_WORDS    "Tokenize"

/* tags */
#define SWISH_DEFAULT_METANAME    "swishdefault"
#define SWISH_TITLE_METANAME      "swishtitle"
#define SWISH_TITLE_TAG           "title"
#define SWISH_BODY_TAG            "body"

/* mimes */
#define SWISH_DEFAULT_MIME        "text/plain"

/* indexes */
#define SWISH_INDEX_FILENAME      "index.swish3"
#define SWISH_XAPIAN_FORMAT       "xapian"
#define SWISH_SWISH_FORMAT        "swish"
#define SWISH_HYPERE_FORMAT       "hypere"
#define SWISH_INDEX_FILEFORMAT    "swish"

/* properties */
#define SWISH_PROP_STRING          1
#define SWISH_PROP_DATE            2
#define SWISH_PROP_INT             3

#define SWISH_PROP_RECCNT          "swishreccount"
#define SWISH_PROP_RANK            "swishrank"
#define SWISH_PROP_DOCID           "swishfilenum"
#define SWISH_PROP_DOCPATH         "swishdocpath"
#define SWISH_PROP_DBFILE          "swishdbfile"
#define SWISH_PROP_TITLE           "swishtitle"
#define SWISH_PROP_SIZE            "swishdocsize"
#define SWISH_PROP_MTIME           "swishlastmodified"
#define SWISH_PROP_DESCRIPTION     "swishdescription"
#define SWISH_PROP_CONNECTOR       " | "

/* utils */
#define SWISH_MAX_WORD_LEN        256
#define SWISH_MAX_FILE_LEN        102400000 /* ~100 mb */

#if defined(WIN32) && !defined (__CYGWIN__)
#define SWISH_PATH_SEP             '\\'
#define SWISH_EXT_SEP              "\\."
#else
#define SWISH_PATH_SEP             "/"
#define SWISH_EXT_SEP              "/."
#endif

#define SWISH_EXT_CH               '.'

/* encodings */
#define SWISH_DEFAULT_ENCODING    "UTF-8"
#define SWISH_LOCALE              "en_US.UTF-8"


/* debugging levels */
#define SWISH_DEBUG_MEMORY      13
#define SWISH_DEBUG_CONFIG      11
#define SWISH_DEBUG_DOCINFO     1
#define SWISH_DEBUG_WORDLIST    7
#define SWISH_DEBUG_TOKENIZER   5
#define SWISH_DEBUG_PARSER      9

#ifdef __cplusplus
extern "C" {
#endif


/* global init and cleanup functions -- call these in every linking program */
void        swish_init();
void        swish_cleanup();


/* utils */
typedef struct swish_StringList        swish_StringList;


struct swish_StringList
{
    int             n;
    xmlChar **      word;
};


/* hash functions */
int         swish_hash_add( xmlHashTablePtr hash, xmlChar *key, void * value );
int         swish_hash_replace( xmlHashTablePtr hash, xmlChar *key, void *value );
int         swish_hash_delete( xmlHashTablePtr hash, xmlChar *key );
xmlHashTablePtr swish_new_hash(int size);

/* IO */
xmlChar *   swish_slurp_stdin( long flen );
xmlChar *   swish_slurp_file_len( xmlChar *filename, long flen );
xmlChar *   swish_slurp_file( xmlChar *filename );
xmlChar *   swish_get_file_ext( xmlChar *url );

/* memory functions */
void        swish_init_memory();
void *      swish_xrealloc(void *ptr, size_t size);
void *      swish_xmalloc( size_t size );
void        swish_xfree( void *ptr );
void        swish_mem_debug();
xmlChar *   swish_xstrdup( const xmlChar * ptr );

/* time functions */
double      swish_time_elapsed(void);
double      swish_time_cpu(void);
char *      swish_print_time(double time);
char *      swish_print_fine_time(double time);

/* error functions */
void        swish_set_error_handle( FILE *where );
void        swish_fatal_err(char *msg,...);
void        swish_warn_err(char *msg,...);
void        swish_debug_msg(char *msg,...);

/* string functions */
void                swish_verify_utf8_locale();
int                 swish_is_ascii( xmlChar *str );
int                 swish_utf8_chr_len( xmlChar *utf8 );
wchar_t *           swish_locale_to_wchar(xmlChar * str);
xmlChar *           swish_wchar_to_locale(wchar_t * str);
wchar_t *           swish_wstr_tolower(wchar_t *s);
xmlChar *           swish_str_tolower(xmlChar *s );
xmlChar *           swish_str_skip_ws(xmlChar *s);
void                swish_str_trim_ws(xmlChar *string);
void                swish_debug_wchars( const wchar_t * widechars );
int                 swish_wchar_t_comp(const void *s1, const void *s2);
int                 swish_sort_wchar(wchar_t *s);
swish_StringList *  swish_make_StringList(xmlChar * line);
swish_StringList *  swish_init_StringList();
void                swish_free_StringList(swish_StringList * sl);

typedef struct  swish_Config       swish_Config;
typedef struct  swish_ConfigValue  swish_ConfigValue;

struct swish_Config
{
    int    ref_cnt;    /* for scripting languages */
    void *          stash;      /* also for scripting languages */
    xmlHashTablePtr conf;       /* the meat */
};

struct swish_ConfigValue
{
    int             ref_cnt;
    unsigned int    multi;      /* indicates whether value is a string or hashref */
    unsigned int    equal;      /* indicates whether key/value pairs are equal */
    void            *value;     /* xmlHashTablePtr or xmlChar *str */
    xmlChar         *key;       /* should be same as the key that points at this object */
};


/* configuration functions */
swish_Config * swish_init_config();
swish_Config * swish_add_config( xmlChar * conf, swish_Config * config );
swish_Config * swish_parse_config( xmlChar * conf, swish_Config * config );
swish_Config * swish_parse_config_new( xmlChar * conf, swish_Config * config );
int             swish_debug_config( swish_Config * config );
xmlHashTablePtr swish_subconfig_hash( swish_Config * config, xmlChar *key);
int             swish_config_value_exists( swish_Config * config, xmlChar *key, xmlChar *value );
xmlChar *       swish_get_config_value(swish_Config * config, xmlChar * key, xmlChar * value);
void            swish_free_config(swish_Config * config);

/* accessors */
swish_ConfigValue * swish_keys( swish_Config * config, ... );
swish_ConfigValue * swish_value( swish_Config * config, xmlChar * key, ... );
swish_ConfigValue * swish_init_ConfigValue();
void                swish_free_ConfigValue( swish_ConfigValue * cv );

/* TODO read_swishheader() and write_swishheader() */

/* MIME type and parser functions */
xmlHashTablePtr swish_mime_hash();
xmlChar *       swish_get_mime_type( swish_Config * config, xmlChar * fileext );
xmlChar *       swish_get_parser( swish_Config * config, xmlChar *mime );



/* we declare structs this 2-step way to make some C++ compilers happy */

typedef struct swish_DocInfo           swish_DocInfo;
typedef struct swish_MetaStackElement  swish_MetaStackElement;
typedef struct swish_MetaStackElement  *swish_MetaStackElementPtr;
typedef struct swish_MetaStack         swish_MetaStack;
typedef struct swish_Word              swish_Word;
typedef struct swish_WordList          swish_WordList;
typedef struct swish_ParseData         swish_ParseData;
typedef struct swish_Tag               swish_Tag;
typedef struct swish_TagStack          swish_TagStack;
typedef struct swish_Analyzer          swish_Analyzer;
typedef struct swish_Parser            swish_Parser;

struct swish_DocInfo
{
    time_t              mtime;
    off_t               size;
    xmlChar *           mime;
    xmlChar *           encoding;
    xmlChar *           uri;
    unsigned int        nwords;
    xmlChar *           ext;
    xmlChar *           parser;
    xmlChar *           update;
};


struct swish_Word
{
    unsigned int        position;       // word position in doc
    xmlChar             *metaname;      // immediate metaname
    xmlChar             *context;       // metaname ancestry
    xmlChar             *word;          // the word itself (NOTE stored as multibyte not wchar)
    unsigned int        start_offset;   // start byte
    unsigned int        end_offset;     // end byte   
    struct swish_Word  *next;          // pointer to next swish_Word
    struct swish_Word  *prev;          // pointer to prev swish_Word
};

struct swish_WordList
{
    swish_Word    *head;
    swish_Word    *tail;
    swish_Word    *current;        // for iterating
    unsigned int   nwords;
    unsigned int   ref_cnt;        // for scripting languages
};

struct swish_Tag
{
    xmlChar            *name;
    struct swish_Tag   *next;
    unsigned int       n;
};

struct swish_TagStack
{
    swish_Tag         *head;
    swish_Tag         *temp;
    unsigned int       count;
    char              *name;       // debugging aid -- name of the stack
    xmlChar           *flat;       // all the stack item names as a string for convenience
};

struct swish_Analyzer
{
    unsigned int           maxwordlen;         // max word length
    unsigned int           minwordlen;         // min word length
    unsigned int           tokenize;           // should we parse into WordList
    swish_WordList*      (*tokenizer) (swish_Analyzer*, xmlChar*, ...);
    xmlChar*             (*stemmer)   (xmlChar*);
    unsigned int           lc;                 // should tokens be lowercased
    void                  *stash;              // for script bindings
    void                  *regex;              // optional regex
    int           ref_cnt;            // for script bindings
};

struct swish_Parser
{
    int                    ref_cnt;             // for script bindings
    swish_Config          *config;              // config object
    swish_Analyzer        *analyzer;            // analyzer object
    void                 (*handler)(swish_ParseData*); // handler reference
    void                  *stash;               // for script bindings
};

// TODO maybe store swish_Parser * here instead of separate config and analyzer
struct swish_ParseData
{
    xmlBufferPtr           buf_ptr;            // text buffer
    xmlBufferPtr           prop_buf;           // Property buffer
    xmlChar               *tag;                // current tag name
    swish_DocInfo         *docinfo;            // document-specific properties
    swish_Config          *config;             // global config
    unsigned int           no_index;           // toggle flag for special comments
    unsigned int           is_html;            // shortcut flag for html parser
    unsigned int           bump_word;          // boolean for moving word position/adding space
    unsigned int           word_pos;           // word position in document
    unsigned int           offset;             // current offset position
    swish_TagStack        *metastack;          // stacks for tracking the tag => metaname
    swish_TagStack        *propstack;          // stacks for tracking the tag => property
    xmlParserCtxtPtr       ctxt;
    swish_WordList        *wordlist;           // linked list of words
    xmlHashTablePtr        propHash;           // hash of Props, one for each property
    swish_Analyzer        *analyzer;           // Analyzer struct
    void                  *stash;          // for script bindings
};

/* public functions */

/* parser styles -- main entry points */
swish_Parser *  swish_init_parser(  swish_Config * config, 
                                    swish_Analyzer * analyzer, 
                                    void (*handler) (swish_ParseData *),
                                    void *stash                                 
                                    );
void  swish_free_parser( swish_Parser * parser );
int swish_parse_file(   swish_Parser * parser,
                        xmlChar *filename,
                        void * stash );
int swish_parse_stdin(  swish_Parser * parser,
                        void * stash  );
int swish_parse_buffer( swish_Parser * parser,
                        xmlChar * buf, 
                        void * stash  );


/* utility buffers */
void                swish_append_buffer(xmlBufferPtr buf, const xmlChar * txt, int txtlen);



/* word functions */
void                swish_init_words();
swish_WordList *    swish_init_WordList();
void                swish_free_WordList(swish_WordList * list);
swish_WordList *    swish_tokenize( swish_Analyzer * analyzer, xmlChar * str, ... );

swish_WordList *    swish_tokenize_utf8_string(
                                      swish_Analyzer * analyzer,  
                                      xmlChar * str,
                                      unsigned int offset,
                                      unsigned int word_pos,
                                      xmlChar * metaname, 
                                      xmlChar * context
                                      );

swish_WordList *    swish_tokenize_ascii_string(   
                                      swish_Analyzer * analyzer, 
                                      xmlChar * str,
                                      unsigned int offset,
                                      unsigned int word_pos,
                                      xmlChar * metaname, 
                                      xmlChar * context
                                      );

swish_WordList *    swish_tokenize_regex(
                                      swish_Analyzer * analyzer, 
                                      xmlChar * str,
                                      unsigned int offset,
                                      unsigned int word_pos,
                                      xmlChar * metaname, 
                                      xmlChar * context
                                      );
                                        
size_t              swish_add_to_wordlist(  swish_WordList * list, 
                                            xmlChar * word,
                                            xmlChar * metaname,
                                            xmlChar * context,
                                            int word_pos, 
                                            int offset );
                                            
void                swish_debug_wordlist( swish_WordList * list );

swish_Analyzer *    swish_init_analyzer( swish_Config * config );
void                swish_free_analyzer( swish_Analyzer * analyzer );


/* DocInfo struct functions */
swish_DocInfo *     swish_init_docinfo();
void                swish_free_docinfo( swish_DocInfo * ptr );
int                 swish_check_docinfo(swish_DocInfo * docinfo, swish_Config * config);
int                 swish_docinfo_from_filesystem( xmlChar *filename, swish_DocInfo * i, swish_ParseData *parse_data );
void                swish_debug_docinfo( swish_DocInfo * docinfo );


/* Property functions */
xmlHashTablePtr swish_init_PropHash( swish_Config * config);
void            swish_free_PropHash( xmlHashTablePtr prophash);
void            swish_debug_PropHash(xmlHashTablePtr propHash);




#ifdef __cplusplus
}
#endif
#endif /* ! __LIBSWISH3_H__ */
