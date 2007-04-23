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
 
#define TEST_SAX 0

/*
 * parse XML doc from memory using libxml2 SAX2 based on tutorial at
 * http://www.jamesh.id.au/articles/libxml-sax/libxml-sax.html
 *
 * save all character() data to buffer, flushing on new metanames
 * flush should split buffer into words, skipping nonwordchars/space, and
 * lowercase all
 *
 * see iswlower(3) man page, etc.
 *
 * all the mb*() functions rely on locale to recognize multi-byte strings
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <stdarg.h>
#include <err.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <errno.h>
#include <libxml/parserInternals.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>

#include "libswish3.h"


extern int      errno;
extern int      SWISH_DEBUG;

int             SWISH_PARSER_ERROR     = 0;
int             SWISH_PARSER_WARNING   = 0;
int             SWISH_PARSER_FATAL     = 0;

static void     get_env_vars();

static void     flush_buffer(swish_ParseData * parse_data, xmlChar * metaname);
static void     add_to_prop_buf(xmlBufferPtr buf_ptr, 
                                xmlHashTablePtr propHash, 
                                xmlChar * propName);
static void     tokenize(       swish_ParseData * parse_data, 
                                xmlChar * string, 
                                int len, 
                                xmlChar * metaname,
                                xmlChar * content
                                );

static xmlChar *flatten_tag_stack(xmlChar * tag, swish_TagStack * stack);
static void     add_stack_to_prop_buf(xmlChar * tag, swish_ParseData * parse_data);
static swish_TagStack *
                push_tag_stack(swish_TagStack * stack, xmlChar * tag);
static int      pop_tag_stack(swish_TagStack * stack);
static xmlChar *pop_tag_stack_on_match(swish_TagStack * stack, xmlChar * tag);

static void     mystartDocument(void *parse_data);
static void     myendDocument(void *parse_data);
static void     mystartElement(void *parse_data, const xmlChar * name, const xmlChar ** atts);
static void     myendElement(void *parse_data, const xmlChar * name);

/* SAX2 support */
static void 
mystartElementNs(
         void *parse_data,
         const xmlChar * localname,
         const xmlChar * prefix,
         const xmlChar * URI,
         int nb_namespaces,
         const xmlChar ** namespaces,
         int nb_attributes,
         int nb_defaulted,
         const xmlChar ** attributes);

static void 
myendElementNs(
           void *ctx ATTRIBUTE_UNUSED,
           const xmlChar * localname,
           const xmlChar * prefix,
           const xmlChar * URI);

static void     chars_to_words(swish_ParseData * parse_data, const xmlChar * ch, int len);
static void     mycharacters(void *parse_data, const xmlChar * ch, int len);
static void     mycomments(void *parse_data, const xmlChar * ch);
static void     myerr(void *user_data, xmlChar * msg,...);


static void     open_tag(void *data, const xmlChar * tag, const xmlChar ** atts);
static void     close_tag(void *data, const xmlChar * tag);
static xmlChar *build_tag(swish_ParseData * parse_data, xmlChar * tag, xmlChar ** atts);

static int      docparser(swish_ParseData * parse_data, xmlChar * filename, xmlChar * buffer, int size);
static int      xml_parser(xmlSAXHandlerPtr sax, void *user_data, xmlChar * buffer, int size);
static int      html_parser(xmlSAXHandlerPtr sax, void *user_data, xmlChar * buffer, int size);
static int      txt_parser(swish_ParseData * parse_data, xmlChar * buffer, int size);

static swish_ParseData *
                init_parse_data(swish_Config * config, swish_Analyzer * analyzer, void * stash);
static void     free_parse_data(swish_ParseData * parse_data);

/* parsing stdin/buffer headers */
typedef struct
{
    xmlChar       **lines;
    int             body_start;
    int             nlines;
} HEAD;

static HEAD *           buf_to_head(xmlChar * buf);
static void             free_head(HEAD * h);
static swish_DocInfo * head_to_docinfo(HEAD * h);

static xmlChar *document_encoding(xmlParserCtxtPtr ctxt);

static void     set_encoding(swish_ParseData * parse_data, xmlChar * buffer);



#if   TEST_SAX
#include "testsax.c"
#endif

swish_Parser *
swish_init_parser( 
    swish_Config * config, 
    swish_Analyzer * analyzer, 
    void (*handler) (swish_ParseData *),
    void * stash
)
{   
    swish_Parser * p = (swish_Parser*) swish_xmalloc(sizeof(swish_Parser));
    p->config   = config;
    p->analyzer = analyzer;
    p->stash    = stash;
    p->handler  = handler;
    p->ref_cnt  = 0;
    
    /* libxml2 stuff */
    xmlInitParser();        
    xmlSubstituteEntitiesDefault(1);    /* resolve text entities */
    
    /* debugging help */
    get_env_vars();
    
    return p;
}

void
swish_free_parser( swish_Parser * p )
{        
    xmlCleanupParser();
    xmlMemoryDump();
    swish_xfree(p);
}

/* turn the literal xml/html tag into a swish tag for matching against metanames and
 * properties */
static xmlChar *
build_tag(swish_ParseData * parse_data, xmlChar * tag, xmlChar ** atts)
{
    int             i, is_html_tag;
    xmlChar        *swishtag, *alias, *metaname, *metacontent;

    metaname = NULL;
    metacontent = NULL;
    
    /* normalize all tags */
    swishtag = swish_str_tolower(tag);


    /* html tags */
    if (parse_data->is_html)
    {

        if (xmlStrEqual(swishtag, (xmlChar *) "br")
            ||
            xmlStrEqual(swishtag, (xmlChar *) "img")
            )
        {
            parse_data->bump_word = 1;
        }
        else
        {
            const htmlElemDesc *element = htmlTagLookup(swishtag);

            if (!element)
                is_html_tag = 0;    /* flag that this might be a meta name */

            else if (!element->isinline)
            {
                /* need to bump word_pos so we don't match across block
                 * elements */

            }
        }
        
    /* is this an HTML <meta> tag? treat 'name' attribute as a tag
     * and 'content' attribute as the tag content
     * we assume 'name' and 'content' are always in english.
     */
        
        if (atts != 0)
        {
            for (i = 0; (atts[i] != 0); i++)
            {
            
                if ( SWISH_DEBUG > 3 )
                    swish_debug_msg("%d HTML attr: %s", i, atts[i]);
                
                if( xmlStrEqual(atts[i], (xmlChar*)"name"))
                {
                    //swish_debug_msg("found name: %s", atts[i+1]);
                    metaname = (xmlChar*)atts[i+1];
                }
                
                else if ( xmlStrEqual(atts[i], (xmlChar*)"content"))
                {
                    //swish_debug_msg("found content: %s", atts[i+1]);
                    metacontent = (xmlChar*)atts[i+1];
                }
                
            }
        }
     
        if (metaname != NULL && metacontent != NULL)
        {
            if (SWISH_DEBUG > 3)
                swish_debug_msg("found HTML meta: %s => %s", metaname, metacontent);
                
            /* do not match across metas */
            parse_data->bump_word = 1;
            open_tag(parse_data, metaname, NULL);
            chars_to_words(parse_data, metacontent, xmlStrlen(metacontent));
            close_tag(parse_data, metaname);
            swish_xfree(swishtag);
            return NULL;
        }
    
 
    }

    /* xml tags */
    else
    {

        /* TODO make this configurable ala swish2 */

        parse_data->bump_word = 1;

    }


    if (SWISH_DEBUG > 2)
    {
        fprintf(stderr, " >>> startElement(%s (%s) ", tag, parse_data->tag);
        if (atts != 0)
        {
            for (i = 0; (atts[i] != 0); i++)
            {
                fprintf(stderr, ", %s='", atts[i++]);
                if (atts[i] != 0)
                {
                    fprintf(stderr, "%s'", atts[i]);
                }
            }
        }
        fprintf(stderr, ")\n");

    }


    /* change our internal name for this tag if it is aliased in config */
    alias = swish_get_config_value(parse_data->config, (xmlChar*)SWISH_ALIAS, parse_data->tag);
    if (alias)
    {
        swish_xfree(swishtag);
        swishtag = swish_xstrdup(alias);
    }

    return swishtag;

}

void
swish_append_buffer(xmlBufferPtr buf, const xmlChar * txt, int txtlen)
{
    int ret;
    
    if (txtlen == 0)
        /* shouldn't happen */
        return;
        
    if (buf == NULL)
    {
        swish_fatal_err("bad news. buf ptr is NULL");
        
    }

    ret = xmlBufferAdd( buf, txt, txtlen );
    if (ret)
    {
        swish_fatal_err("problem adding \n>>%s<<\n length %d to buffer. Err: %d", 
                        txt, txtlen, ret);
    }
    
}

static void
add_to_prop_buf(xmlBufferPtr buf, xmlHashTablePtr propHash, xmlChar * propName)
{

    xmlChar *    nowhitesp;
    xmlBufferPtr propBuf = xmlHashLookup(propHash, propName);

    if (propBuf && xmlBufferLength(propBuf))
    {
        /* swish_debug_msg("adding %s to propBuf", propName); */

        /* if the propBuf already exists and we're about to add more, append the
         * connect string */
        if (xmlBufferLength(buf))
        {
            swish_append_buffer(propBuf,
                      (const xmlChar *) SWISH_PROP_CONNECTOR,
                      xmlStrlen((xmlChar *) SWISH_PROP_CONNECTOR));
        }

        nowhitesp = swish_str_skip_ws((xmlChar *)xmlBufferContent(buf));
        swish_str_trim_ws(nowhitesp);

        swish_append_buffer(propBuf, (const xmlChar *) nowhitesp, xmlStrlen(nowhitesp));
    }

}

static void
flush_buffer(swish_ParseData * parse_data, xmlChar * metaname)
{

    if (SWISH_DEBUG > 10)
        swish_debug_msg("buffer is >>%s<< before flush, word_pos = %d", 
            xmlBufferContent(parse_data->buf_ptr), parse_data->word_pos);

    /* since we only flush the buffer when metaname changes, and
     * we do not want to match across metanames, bump the word_pos here
     * before parsing the string and making the tmp wordlist
     */
    if (parse_data->word_pos)
        parse_data->word_pos++;


    tokenize(   parse_data, 
                (xmlChar *)xmlBufferContent(parse_data->buf_ptr), 
                xmlBufferLength(parse_data->buf_ptr),
                parse_data->metastack->head->name,
                metaname
                );

    xmlBufferEmpty(parse_data->buf_ptr);

}


/* SAX2 callback */
static void
mystartDocument(void *data)
{
    /* swish_ParseData *parse_data = (swish_ParseData *) data; */

    if (SWISH_DEBUG > 2)
        swish_debug_msg("startDocument()");

}


/* SAX2 callback */
static void
myendDocument(void *parse_data)
{

    if (SWISH_DEBUG > 2)
        swish_debug_msg("endDocument()");

    flush_buffer(parse_data, NULL);    /* whatever's left */

}


/* SAX1 callback */
static void
mystartElement(void *data, const xmlChar * name, const xmlChar ** atts)
{
    open_tag(data, name, atts);
}


/* SAX1 callback */
static void
myendElement(void *data, const xmlChar * name)
{
    close_tag(data, name);
}

/* SAX2 handler */
static void
mystartElementNs(
         void *data,
         const xmlChar * localname,
         const xmlChar * prefix,
         const xmlChar * URI,
         int nb_namespaces,
         const xmlChar ** namespaces,
         int nb_attributes,
         int nb_defaulted,
         const xmlChar ** attributes)
{
    open_tag(data, localname, attributes);
}

/* SAX2 handler */
static void
myendElementNs(void *data,
           const xmlChar * localname,
           const xmlChar * prefix,
           const xmlChar * URI)
{
    close_tag(data, localname);
}

static void
open_tag(void *data, const xmlChar * tag, const xmlChar ** atts)
{
    swish_ParseData *parse_data = (swish_ParseData *) data;

    if (parse_data->tag != NULL)
        swish_xfree(parse_data->tag);

    parse_data->tag = build_tag(parse_data, (xmlChar *) tag, (xmlChar **) atts);



    if (SWISH_DEBUG > 8)
        swish_debug_msg("checking config for '%s' in watched tags", parse_data->tag);


    /* set property if this tag is configured for it */
    if (swish_config_value_exists(parse_data->config, (xmlChar *) SWISH_PROP, parse_data->tag))
    {
        if (SWISH_DEBUG > 8)
            swish_debug_msg(" %s = new property", parse_data->tag);

        add_stack_to_prop_buf(NULL, parse_data);
        xmlBufferEmpty(parse_data->prop_buf);

        parse_data->propstack = push_tag_stack(parse_data->propstack, parse_data->tag);

        /* swish_debug_msg("%s pushed ok unto propstack", parse_data->tag);  */
    }

    /* likewise for metastack */
    if (swish_config_value_exists(parse_data->config, (xmlChar *) SWISH_META, parse_data->tag))
    {
        if (SWISH_DEBUG > 8)
            swish_debug_msg(" %s = new metaname", parse_data->tag);

        flush_buffer(parse_data, NULL);

        parse_data->metastack = push_tag_stack(parse_data->metastack, parse_data->tag);
    }
    
    if (SWISH_DEBUG > 8)
        swish_debug_msg("config check for '%s' done", parse_data->tag);


}

static void
close_tag(void *data, const xmlChar * tag)
{
    xmlChar        *metaname;
    swish_ParseData *parse_data;
    parse_data = (swish_ParseData *) data;

    /* lowercase all names for comparison against metanames (which are also
     * lowercased) */
    if (parse_data->tag != NULL)
        swish_xfree(parse_data->tag);

    parse_data->tag = build_tag(parse_data, (xmlChar *) tag, NULL);

    if (SWISH_DEBUG > 2)
        swish_debug_msg(" endElement(%s) (%s)", (xmlChar *) tag, parse_data->tag);

    if ((metaname = pop_tag_stack_on_match(parse_data->propstack, parse_data->tag)) != NULL)
    {
        //swish_debug_msg("popped %s from propstack", parse_data->tag);
        add_stack_to_prop_buf(parse_data->tag, parse_data);
        xmlBufferEmpty(parse_data->prop_buf);
        swish_xfree(metaname);
    }

    if ((metaname = pop_tag_stack_on_match(parse_data->metastack, parse_data->tag)) != NULL)
    {
        /* swish_debug_msg("popped %s from metastack", parse_data->tag); */
        flush_buffer(parse_data, metaname);
        swish_xfree(metaname);
    }

    /* turn flag off so next open_tag() can evaluate */
    parse_data->bump_word = 0;

}

/* handle all characters in doc */
static void
chars_to_words(swish_ParseData * parse_data, const xmlChar * ch, int len)
{
    int             i;
    xmlChar         output[len];
    xmlBufferPtr    buf = parse_data->buf_ptr;
    /*
     * why not wchar_t ? len is number of bytes, not number of
     * characters, so xmlChar (i.e., char) works
     */
    

    /*
     * swish_debug_msg( "sizeof output buf is %d; len was %d\n", sizeof(output),
     * len );
     */

    /* swish_debug_msg( "characters"); */

    for (i = 0; i < len; i++)
    {
        /* fprintf(stderr, "%c", ch[i]); */
        output[i] = ch[i];
    }
    output[i] = (xmlChar) NULL;

    if (parse_data->bump_word && xmlBufferLength(buf))
        swish_append_buffer(buf, (xmlChar *) " ", 1);

    swish_append_buffer(buf, output, len);

    if (parse_data->bump_word && xmlBufferLength(parse_data->prop_buf))
        swish_append_buffer(parse_data->prop_buf, (xmlChar *) " ", 1);

    swish_append_buffer(parse_data->prop_buf, output, len);


}


/* SAX2 callback */
static void
mycharacters(void *parse_data, const xmlChar * ch, int len)
{
    if (SWISH_DEBUG > 2)
        swish_debug_msg(" >> mycharacters()");

    chars_to_words(parse_data, ch, len);
}


/* SAX2 callback */
static void
mycomments(void *parse_data, const xmlChar * ch)
{
    int             len = strlen((char *) (char *) ch);

    /* TODO: make comments indexing optional */
    /* TODO: enable noindex option */
    return;

    chars_to_words(parse_data, ch, len);
}


/* SAX2 callback */
static void
myerr(void *user_data, xmlChar * msg, ...)
{
    if (!SWISH_PARSER_ERROR)
        return;

    if (!SWISH_PARSER_FATAL)
        return;
        
    swish_warn_err("libxml2 error:");

    va_list         args;
    char            str[1000];
    swish_ParseData *parse_data = (swish_ParseData *) user_data;

    va_start(args, msg);
    vsnprintf((char *) str, 1000, (char *) msg, args);
    xmlParserError(parse_data->ctxt, (char *) str);
    va_end(args);
    
    //swish_warn_err("end libxml2 error");
}


/* SAX2 callback */
static void
mywarn(void *user_data, xmlChar * msg, ...)
{
    if (!SWISH_PARSER_WARNING)
        return;
        
    swish_warn_err("libxml2 warning:");

    va_list         args;
    char            str[1000];
    swish_ParseData *parse_data = (swish_ParseData *) user_data;

    va_start(args, msg);
    vsnprintf((char *) str, 1000, (char *) msg, args);
    xmlParserWarning(parse_data->ctxt, (char *) str);
    va_end(args);
}


/* SAX2 handler struct for html and xml parsing */

xmlSAXHandler   my_parser =
{
    NULL,            /* internalSubset */
    NULL,            /* isStandalone */
    NULL,            /* hasInternalSubset */
    NULL,            /* hasExternalSubset */
    NULL,            /* resolveEntity */
    NULL,            /* getEntity */
    NULL,            /* entityDecl */
    NULL,            /* notationDecl */
    NULL,            /* attributeDecl */
    NULL,            /* elementDecl */
    NULL,            /* unparsedEntityDecl */
    NULL,            /* setDocumentLocator */
    mystartDocument,    /* startDocument */
    myendDocument,        /* endDocument */
    mystartElement,        /* startElement */
    myendElement,        /* endElement */
    NULL,            /* reference */
    mycharacters,        /* characters */
    NULL,            /* ignorableWhitespace */
    NULL,            /* processingInstruction */
    mycomments,        /* comment */
    (warningSAXFunc) & mywarn,    /* xmlParserWarning */
    (errorSAXFunc) & myerr,    /* xmlParserError */
    (fatalErrorSAXFunc) & myerr,    /* xmlfatalParserError */
    NULL,            /* getParameterEntity */
    NULL,            /* cdataBlock; *//* should we handle this too  ?? */
    NULL,            /* externalSubset; */
    XML_SAX2_MAGIC,
    NULL,
    mystartElementNs,    /* startElementNs */
    myendElementNs,        /* endElementNs */
    NULL            /* xmlStructuredErrorFunc */
};

xmlSAXHandlerPtr my_parser_ptr = &my_parser;

static int
docparser(
      swish_ParseData * parse_data,
      xmlChar * filename,
      xmlChar * buffer,
      int size)
{

    int             ret;
    xmlChar        *mime = (xmlChar *) parse_data->docinfo->mime;
    xmlChar        *parser = (xmlChar *) parse_data->docinfo->parser;
    
    if (!size && !xmlStrlen(buffer) && !parse_data->docinfo->size)
    {
        swish_warn_err("%s appears to be empty -- can't parse it",
                        parse_data->docinfo->uri);
                        
        return 1;
    }
    

    if (SWISH_DEBUG)
        swish_debug_msg("%s -- using %s parser", parse_data->docinfo->uri, parser);


    /* slurp file if not already in memory */
    if (filename && !buffer)
    {
        buffer = swish_slurp_file_len(filename, (long) parse_data->docinfo->size);
        size = parse_data->docinfo->size;
    }

    if (parser[0] == 'H')
    {
        parse_data->is_html = 1;
        ret = html_parser(my_parser_ptr, parse_data, buffer, size);
    }

    else if (parser[0] == 'X')
#if TEST_SAX
        ret = xmlSAXUserParseMemory(debugSAX2Handler, NULL, (const char *) buffer, size);
#else
        ret = xml_parser(my_parser_ptr, parse_data, buffer, size);
#endif

    else if (parser[0] == 'T')
        ret = txt_parser(parse_data, (xmlChar *) buffer, size);

    else
        swish_fatal_err("no parser known for MIME '%s'", mime);


    if (filename)
    {
        /* swish_debug_msg( "freeing buffer"); */
        swish_xfree(buffer);
    }

    return ret;

}


static swish_ParseData *
init_parse_data(swish_Config * config, swish_Analyzer * analyzer, void * stash)
{

    if (SWISH_DEBUG > 9)
        swish_debug_msg("init parse_data");

    swish_ParseData *ptr = (swish_ParseData *) swish_xmalloc(sizeof(swish_ParseData));
    
    ptr->stash = stash;
    
    ptr->buf_ptr  = xmlBufferCreateSize(SWISH_BUFFER_CHUNK_SIZE);
    ptr->prop_buf = xmlBufferCreateSize(SWISH_BUFFER_CHUNK_SIZE);
    
    ptr->config = config;
    ptr->analyzer = analyzer;
    
    ptr->tag = NULL;
    ptr->wordlist = swish_init_WordList();
    ptr->propHash = swish_init_PropHash(config);

    /* prime the stacks */
    ptr->metastack = (swish_TagStack *) swish_xmalloc(sizeof(swish_TagStack));
    ptr->metastack->name = "MetaStack";
    ptr->metastack->head = NULL;
    ptr->metastack->temp = NULL;
    ptr->metastack->flat = NULL;
    ptr->metastack->count = 0;
    ptr->metastack = push_tag_stack(ptr->metastack, (xmlChar *) SWISH_DEFAULT_METANAME);

    ptr->propstack = (swish_TagStack *) swish_xmalloc(sizeof(swish_TagStack));
    ptr->propstack->name = "PropStack";
    ptr->propstack->head = NULL;
    ptr->propstack->temp = NULL;
    ptr->propstack->flat = NULL;
    ptr->propstack->count = 0;
    ptr->propstack = push_tag_stack(ptr->propstack, (xmlChar *) "_");    /* no such property --
                                         * just to seed stack */

    /* gets toggled per-tag */
    ptr->bump_word = 1;
            
    /* toggle */
    ptr->no_index = 0;
    
    /* shortcut rather than looking parser up in hash for each tag event */
    ptr->is_html = 0;
    
    /* must be zero so that ++ works ok on first word */
    ptr->word_pos = 0;
    
    /* always start at first byte */
    ptr->offset = 0;


    /* pointer to the xmlParserCtxt since we want to free it only after we're
       completely done with it.
       NOTE this is a change per libxml2 vers > 2.6.16 
    */
    ptr->ctxt = NULL;
    
    if (SWISH_DEBUG > 9)
        swish_debug_msg("init done for parse_data");


    return ptr;

}


static void
free_parse_data(swish_ParseData * ptr)
{

    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData");

    /* Pop the stacks */
    while (pop_tag_stack(ptr->metastack))
    {
        if (SWISH_DEBUG > 9)
            swish_debug_msg("head of stack is %d %s", ptr->metastack->count, ptr->metastack->head->name);

    }

    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData metastack");

    swish_xfree(ptr->metastack);

    while (pop_tag_stack(ptr->propstack))
    {
        if (SWISH_DEBUG > 9)
            swish_debug_msg("head of stack is %d %s", ptr->propstack->count, ptr->propstack->head->name);

    }

    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData propstack");

    swish_xfree(ptr->propstack);


    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData propHash");

    swish_free_PropHash(ptr->propHash);

    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData xmlBuffer");

    xmlBufferFree( ptr->buf_ptr );


    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData prop xmlBuffer");

    xmlBufferFree( ptr->prop_buf );


    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData tag");

    if (ptr->tag != NULL)
        swish_xfree(ptr->tag);


    if (ptr->ctxt != NULL)
    {

        if (SWISH_DEBUG > 9)
            swish_debug_msg("freeing swish_ParseData libxml2 parser ctxt");

        if (xmlStrEqual(ptr->docinfo->parser, (xmlChar *) SWISH_PARSER_XML))
            xmlFreeParserCtxt(ptr->ctxt);

        if (xmlStrEqual(ptr->docinfo->parser, (xmlChar *) SWISH_PARSER_HTML))
            htmlFreeParserCtxt(ptr->ctxt);
    }
    else
    {
        if (SWISH_DEBUG > 9)
            swish_debug_msg("swish_ParseData libxml2 parser ctxt already freed");

    }

    if (ptr->wordlist != NULL)
    {

        if (SWISH_DEBUG > 9)
            swish_debug_msg("free swish_ParseData wordList");

        swish_free_WordList(ptr->wordlist);
    }

    if (ptr->docinfo != NULL)
    {

        if (SWISH_DEBUG > 9)
            swish_debug_msg("free swish_ParseData docinfo");

        swish_free_docinfo(ptr->docinfo);

    }
    
    if (SWISH_DEBUG > 9)
        swish_debug_msg("freeing swish_ParseData ptr");

    swish_xfree(ptr);

    if (SWISH_DEBUG > 9)
        swish_debug_msg("PARSE_DATA all freed");
}



static HEAD    *
buf_to_head(xmlChar * buf)
{
    int             i, j, k;
    xmlChar        *line;
    HEAD           *h;
    
    if (SWISH_DEBUG > 3)
        swish_debug_msg("parsing buffer into head: %s", buf);

    h           = swish_xmalloc(sizeof(HEAD));
    h->lines    = swish_xmalloc(SWISH_MAX_HEADERS * sizeof(line));
    h->nlines   = 0;
    h->body_start = 0;
    line        = swish_xmalloc(SWISH_MAXSTRLEN + 1);
    i = 0;
    j = 0;
    k = 0;

    while (j < SWISH_MAX_HEADERS && i <= SWISH_MAXSTRLEN)
    {
        /* swish_debug_msg( "i = %d   j = %d   k = %d", i, j, k); */

        if (buf[k] == '\n')
        {
            swish_fatal_err("illegal newline to start doc header");
        }
        line[i] = buf[k];
        /* fprintf(stderr, "%c", line[i]); */
        i++;
        k++;

        if (buf[k] == '\n')
        {

            line[i] = '\0';
            h->lines[j++] = line;
            h->nlines++;
            k++;    /* get to the next char no matter what, then check if ==
                 * '\n' */

            if (buf[k] == '\n')
            {
                /* fprintf(stderr, "found blank line at byte %d\n", k); */
                h->body_start = k + 1;
                break;
            }
            i = 0;
            line = swish_xmalloc(SWISH_MAXSTRLEN + 1);

            continue;
        }
    }

    return h;
}


static swish_DocInfo *
head_to_docinfo(HEAD * h)
{
    int             i;
    xmlChar        *val, *line;

    swish_DocInfo *info = swish_init_docinfo();

    if (SWISH_DEBUG > 5)
        swish_debug_msg("preparing to parse %d header lines", h->nlines);

    for (i = 0; i < h->nlines; i++)
    {

        line = h->lines[i];
        val = (xmlChar *) xmlStrchr(line, ':');
        val = swish_str_skip_ws(++val);

        if (SWISH_DEBUG > 2)
        {
            swish_debug_msg("%d parsing header line: %s", i, line);
            
        }

        if (!xmlStrncasecmp(line, (const xmlChar *) "Content-Length", 14))
        {
            if (!val)
                swish_warn_err("Failed to parse Content-Length header '%s'", line);

            info->size = strtol((char *) val, NULL, 10);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Last-Modified", 13))
        {

            if (!val)
                swish_warn_err("Failed to parse Last-Modified header '%s'", line);

            info->mtime = strtol((char *) val, NULL, 10);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Last-Mtime", 10))
        {

            swish_warn_err("Last-Mtime is deprecated in favor of Last-Modified");

            if (!val)
                swish_warn_err("Failed to parse Last-Mtime header '%s'", line);

            info->mtime = strtol((char *) val, NULL, 10);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Content-Location", 16))
        {

            if (!val)
                swish_warn_err("Failed to parse Content-Location header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find path name in Content-Location header '%s'", line);

            if (info->uri != NULL)
                swish_xfree(info->uri);

            info->uri = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Path-Name", 9))
        {

            swish_warn_err("Path-Name is deprecated in favor of Content-Location");

            if (!val)
                swish_warn_err("Failed to parse Path-Name header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find path name in Path-Name header '%s'", line);

            if (info->uri != NULL)
                swish_xfree(info->uri);

            info->uri = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Document-Type", 13))
        {
        
            swish_warn_err("Document-Type is deprecated in favor of Parser-Type");

            if (!val)
                swish_warn_err("Failed to parse Document-Type header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find path name in Document-Type header '%s'", line);

            if (info->parser != NULL)
                swish_xfree(info->parser);

            info->parser = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Parser-Type", 11))
        {

            if (!val)
                swish_warn_err("Failed to parse Parser-Type header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find path name in Parser-Type header '%s'", line);

            if (info->parser != NULL)
                swish_xfree(info->parser);

            info->parser = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Content-Type", 12))
        {

            if (!val)
                swish_warn_err("Failed to parse Content-Type header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find path name in Content-Type header '%s'", line);

            /*
                     * TODO: get encoding out of this line too if
                     * present. example:   text/xml; charset=ISO-8859-1
                     */
                     
            
            if (info->mime != NULL)
                swish_xfree(info->mime);

            info->mime = swish_xstrdup(val);            
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *) "Encoding", 8)
            ||
            !xmlStrncasecmp(line, (const xmlChar *) "Charset", 7)
        )
        {

            if (!val)
                swish_warn_err("Failed to parse Encoding or Charset header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find value in Encoding or Charset header '%s'", line);

            if (info->encoding != NULL)
                swish_xfree(info->encoding);

            info->encoding = swish_xstrdup(val);
            continue;
        }
        /*
             * TODO update mode is a vers2 btree feature. still unclear if
             * we'll actually support it
             */
        if (!xmlStrncasecmp(line, (const xmlChar *) "Update-Mode", 11))
        {

            if (!val)
                swish_warn_err("Failed to parse Update-Mode header '%s'", line);

            if (!*val)
                swish_warn_err("Failed to find value in Update-Mode header '%s'", line);

            if (info->update != NULL)
                swish_xfree(info->update);

            info->update = swish_xstrdup(val);
            continue;
        }
        /* if we get here, unrecognized header line */
        swish_warn_err("Unknown header line: '%s'\n", line);

    }

    if (SWISH_DEBUG > 5)
    {
        swish_debug_msg("returning %d header lines\n", h->nlines);
        swish_debug_docinfo(info);
    }


    return info;
}

static void
get_env_vars()
{
    /* init the global env vars, but don't override if already set */

    setenv("SWISH_PARSER_ERROR", "0", 0);
    SWISH_PARSER_ERROR = (int)strtol(getenv("SWISH_PARSER_ERROR"), (char**)NULL, 10);

    setenv("SWISH_PARSER_WARNING", "0", 0);
    SWISH_PARSER_WARNING = (int)strtol(getenv("SWISH_PARSER_WARNING"), (char**)NULL, 10);

    setenv("SWISH_PARSER_FATAL", "0", 0);
    SWISH_PARSER_FATAL = (int)strtol(getenv("SWISH_PARSER_FATAL"), (char**)NULL, 10);
    
    if (SWISH_DEBUG)
    {
        SWISH_PARSER_ERROR      = SWISH_DEBUG;
        SWISH_PARSER_WARNING    = SWISH_DEBUG;
        SWISH_PARSER_FATAL      = SWISH_DEBUG;
    }
}


/* TODO there's a memory leak somewhere in here. one more malloc than free */
int
swish_parse_stdin(
    swish_Parser * parser,
    void * stash 
)
{
    xmlChar             *ln;
    HEAD                *head;
    int                 i;
    xmlChar             *read_buffer;
    xmlBufferPtr        head_buf;
    swish_ParseData    *parse_data;
    int                 xmlErr;
    int                 min_headers, nheaders;
    double              curTime;
    char                *etime;
    int                 file_cnt;

    i = 0;
    file_cnt = 0;
    nheaders = 0;
    min_headers = 2;
    
    swish_mem_debug();

    ln = swish_xmalloc(SWISH_MAXSTRLEN + 1);
    head_buf = xmlBufferCreateSize((SWISH_MAX_HEADERS * SWISH_MAXSTRLEN) + SWISH_MAX_HEADERS);

    swish_mem_debug();
    
    /* based on extprog.c */
    while (fgets((char *) ln, SWISH_MAXSTRLEN, stdin) != 0)
    {            
    
    /* we don't use fgetws() because we don't care about
     * indiv characters yet */

        xmlChar        *end;
        xmlChar        *line;

        line = swish_str_skip_ws(ln);    /* skip leading white space */
        end = (xmlChar *) strrchr((char *) line, '\n');    
        
        /* trim any white space at end of doc, including \n */
        if (end)
        {
            while (end > line && isspace((int) *(end - 1)))
                end--;

            *end = '\0';
        }

        swish_mem_debug();
        
        if (nheaders >= min_headers && xmlStrlen(line) == 0)
        {        
        
        /* blank line indicates body */
            curTime      = swish_time_elapsed();
            parse_data   = init_parse_data(parser->config, parser->analyzer, stash);
            head         = buf_to_head( (xmlChar*)xmlBufferContent(head_buf) );
            parse_data->docinfo = head_to_docinfo(head);
            swish_check_docinfo(parse_data->docinfo, parser->config);

            if (SWISH_DEBUG > 9)
                swish_debug_msg("reading %ld bytes from stdin\n", 
                                (long int) parse_data->docinfo->size);

            read_buffer = swish_slurp_stdin(parse_data->docinfo->size);

            /* parse */
            xmlErr = docparser(parse_data, NULL, read_buffer, parse_data->docinfo->size);


            if (xmlErr)
                swish_warn_err("parser returned error %d\n", xmlErr);

            if (SWISH_DEBUG > 3)
            {
                swish_debug_msg("\n===============================================================\n");
                swish_debug_docinfo(parse_data->docinfo);
                swish_debug_msg("  word buffer length: %d bytes", 
                                    xmlBufferLength(parse_data->buf_ptr));
                swish_debug_msg(" (%d words)", parse_data->docinfo->nwords);
            }
            if (SWISH_DEBUG > 9)
                swish_debug_msg("passing to handler");

            /* pass to callback function */
            (*parser->handler)(parse_data);

            if (SWISH_DEBUG > 9)
                swish_debug_msg("handler done");

            /* reset everything for next time */

            swish_xfree(read_buffer);
            free_parse_data(parse_data);
            free_head(head);
            xmlBufferEmpty(head_buf);
            nheaders = 0;

            /* count the file */
            file_cnt++;

            if (SWISH_DEBUG)
            {
                etime = swish_print_fine_time(swish_time_elapsed() - curTime);
                swish_debug_msg("%s elapsed time", etime);
                swish_xfree(etime);
            }
            /* timer */
            curTime = swish_time_elapsed();


            if (SWISH_DEBUG)
                swish_debug_msg("\n================ stdin done with file ===================\n");


        }
        else if (xmlStrlen(line) == 0)
        {
            swish_fatal_err("Not enough header lines reading from stdin");


        }
        else
        {
        
            swish_mem_debug();
            
        /* we are reading headers */
            if( xmlBufferAdd( head_buf, line, -1 ) )
                swish_fatal_err("error adding header to buffer");
                
            if( xmlBufferCCat( head_buf, "\n" ) )
                swish_fatal_err("can't add newline to end of header buffer");
                
            nheaders++;
        }

    }
    
    swish_mem_debug();

    if (xmlBufferLength(head_buf))
    {
        swish_fatal_err("Some unparsed header lines remaining");
    }

    swish_xfree(ln);
    xmlBufferFree(head_buf);
    
    swish_mem_debug();

    return file_cnt;
}


static void
free_head(HEAD * h)
{
    int             i;
    for (i = 0; i < h->nlines; i++)
    {
        swish_xfree(h->lines[i]);
    }
    swish_xfree(h->lines);
    swish_xfree(h);
}

/* PUBLIC */

/*
 * pass in a string including headers. like parsing stdin, but only for one
 * doc
 */
int
swish_parse_buffer(
        swish_Parser * parser,
        xmlChar * buf, 
        void * stash 
)
{

    int             res;
    double          curTime = swish_time_elapsed();
    HEAD           *head;
    char           *etime;


    head = buf_to_head(buf);

    if (SWISH_DEBUG > 9)
        swish_debug_msg("number of headlines: %d", head->nlines);

    swish_ParseData *parse_data    = init_parse_data(parser->config, parser->analyzer, stash);
    parse_data->docinfo            = head_to_docinfo(head);
    swish_check_docinfo(parse_data->docinfo, parser->config);

    /* reposition buf pointer at start of body (just past head) */

    buf += head->body_start;

    res = docparser(parse_data, 0, buf, xmlStrlen(buf));

    /* pass to callback function */
    (*parser->handler)(parse_data);

    if (SWISH_DEBUG > 1)
    {
        swish_debug_docinfo(parse_data->docinfo);
        swish_debug_msg("  word buffer length: %d bytes", 
                        xmlBufferLength(parse_data->buf_ptr));
        swish_debug_msg(" (%d words)", parse_data->docinfo->nwords);
    }
    /* free buffers */
    free_head(head);
    free_parse_data(parse_data);


    if (SWISH_DEBUG)
    {
        etime = swish_print_fine_time(swish_time_elapsed() - curTime);
        swish_debug_msg("%s elapsed time", etime);
        swish_xfree(etime);
    }
    curTime = swish_time_elapsed();

    return res;


}

/* PUBLIC */
int
swish_parse_file(
        swish_Parser * parser,
        xmlChar * filename,
        void * stash 
)
{
    int             res;
    double          curTime = swish_time_elapsed();
    char           *etime;

    swish_ParseData *parse_data = init_parse_data(parser->config, parser->analyzer, stash);
    parse_data->docinfo         = swish_init_docinfo();

    if (!swish_docinfo_from_filesystem(filename, parse_data->docinfo, parse_data))
    {
        swish_warn_err("Skipping %s", filename);
        free_parse_data(parse_data);
        return 1;
    }

    res = docparser(parse_data, filename, 0, 0);

    /* pass to callback function */
    (*parser->handler) (parse_data);

    if (SWISH_DEBUG > 1)
    {
        swish_debug_docinfo(parse_data->docinfo);
        swish_debug_msg("  word buffer length: %d bytes", 
                            xmlBufferLength(parse_data->buf_ptr));
        swish_debug_msg(" (%d words)", parse_data->docinfo->nwords);
    }

    /* free buffers */
    free_parse_data(parse_data);

    if (SWISH_DEBUG)
    {
        etime = swish_print_fine_time(swish_time_elapsed() - curTime);
        swish_debug_msg("%s elapsed time", etime);
        swish_xfree(etime);
    }

    return res;

}


/**
 * based on libxml2 xmlSAXUserParseMemory in parser.c
 * which we don't use directly so that we can get encoding
 */
static int
xml_parser(
       xmlSAXHandlerPtr sax,
       void *user_data,
       xmlChar * buffer,
       int size
)
{
    int             ret = 0;
    xmlParserCtxtPtr ctxt;
    swish_ParseData *parse_data = (swish_ParseData *) user_data;
    xmlSAXHandlerPtr oldsax = NULL;

    if (sax == NULL)
        return -1;
    ctxt = xmlCreateMemoryParserCtxt((const char *) buffer, size);
    if (ctxt == NULL)
        return -1;
    oldsax = ctxt->sax;
    ctxt->sax = sax;
    ctxt->sax2 = 1;

    /* always use sax2 -- this pulled from xmlDetextSAX2() */
    ctxt->str_xml = xmlDictLookup(ctxt->dict, BAD_CAST "xml", 3);
    ctxt->str_xmlns = xmlDictLookup(ctxt->dict, BAD_CAST "xmlns", 5);
    ctxt->str_xml_ns = xmlDictLookup(ctxt->dict, XML_XML_NAMESPACE, 36);
    if ((ctxt->str_xml == NULL) || (ctxt->str_xmlns == NULL) ||
        (ctxt->str_xml_ns == NULL))
    {
        /* xmlErrMemory is/was not a public func but is in parserInternals.h
         * basically, this is a bad, fatal error, so we'll just die */
        /* xmlErrMemory(ctxt, NULL); */
        swish_fatal_err("Fatal libxml2 memory error");
    }


    if (user_data != NULL)
        ctxt->userData = user_data;

    xmlParseDocument(ctxt);

    if (ctxt->wellFormed)
        ret = 0;
    else
    {
        if (ctxt->errNo != 0)
            ret = ctxt->errNo;
        else
            ret = -1;
    }
    ctxt->sax = oldsax;
    if (ctxt->myDoc != NULL)
    {
        xmlFreeDoc(ctxt->myDoc);
        ctxt->myDoc = NULL;
    }

    if (parse_data->docinfo->encoding != NULL)
        swish_xfree(parse_data->docinfo->encoding);

    parse_data->docinfo->encoding = document_encoding(ctxt);

    xmlFreeParserCtxt(ctxt);

    return ret;
}


static int
html_parser(
        xmlSAXHandlerPtr sax,
        void *user_data,
        xmlChar * buffer,
        int size
)
{
    htmlDocPtr      ret;
    htmlParserCtxtPtr ctxt;
    htmlSAXHandlerPtr oldsax = 0;
    swish_ParseData *parse_data = (swish_ParseData *) user_data;

    xmlInitParser();

    ctxt = htmlCreateMemoryParserCtxt((const char *) buffer, xmlStrlen(buffer));

    parse_data->ctxt = ctxt;

    if (parse_data->docinfo->encoding != NULL)
        swish_xfree(parse_data->docinfo->encoding);

    parse_data->docinfo->encoding = document_encoding(ctxt);

    if (parse_data->docinfo->encoding == NULL)
        set_encoding(parse_data, (xmlChar *) buffer);

    if (ctxt == 0)
        return (0);
    if (sax != 0)
    {
        oldsax = ctxt->sax;
        ctxt->sax = (htmlSAXHandlerPtr) sax;
        ctxt->userData = parse_data;
    }
    htmlParseDocument(ctxt);

    if (sax != 0)
    {
        ctxt->sax = oldsax;
        ctxt->userData = 0;
    }

    return ret ? 0 : 1;
}

static int
txt_parser(
       swish_ParseData * parse_data,
       xmlChar * buffer,
       int size
)
{
    int             err = 0;
    xmlChar         *out, *enc;
    int             outlen;
    
    out = NULL;
    enc = getenv("SWISH_ENCODING");
        
    /* TODO better encoding detection. for now we assume unknown text files are latin1 */
    set_encoding(parse_data, buffer);
    
    if (SWISH_DEBUG > 3)
        swish_debug_msg("txt parser encoding: %s", parse_data->docinfo->encoding);
    
    if (parse_data->docinfo->encoding != (xmlChar*)SWISH_DEFAULT_ENCODING)
    {
        if (!xmlStrncasecmp(parse_data->docinfo->encoding, (xmlChar*)"iso-8859-1", 10))
        {
            out = swish_xmalloc(size * 2);

            if(!isolat1ToUTF8(out, &outlen, buffer, &size))
            {
                swish_warn_err("could not convert buf from iso-8859-1");
            }
            
            size   = outlen;
            buffer = out;
        }
        
        else if (xmlStrEqual(parse_data->docinfo->encoding, (xmlChar*)"unknown"))
        {
            if (SWISH_DEBUG > 3)
                swish_debug_msg("default env encoding -> %s", enc);


            if (xmlStrncasecmp(enc, (xmlChar*)"iso-8859-1", 10))
            {
                swish_warn_err(
                    "%s encoding is unknown (not %s) but LC_CTYPE is %s -- assuming file is %s",
                    parse_data->docinfo->uri, SWISH_DEFAULT_ENCODING, enc, "iso-8859-1");
                    
            }

            out = swish_xmalloc(size * 2);

            if(!isolat1ToUTF8(out, &outlen, buffer, &size))
            {
                swish_warn_err("could not convert buf from iso-8859-1");
            }
            
            size   = outlen;
            buffer = out;        
        
        }
    }

    /*
         * we obviously haven't any tags on which to trigger our metanames,
         * so set default
         * TODO get title somehow?
         * TODO check config to determine if we should buffer swish_prop_description etc
         */

    parse_data->metastack = push_tag_stack( parse_data->metastack, 
                                            (xmlChar *) SWISH_DEFAULT_METANAME);

    if (SWISH_DEBUG > 2)
        swish_debug_msg("stack pushed for %s", parse_data->metastack->flat);

    chars_to_words(parse_data, buffer, size);
    flush_buffer(parse_data, NULL);
    
    if (out != NULL)
    {
        if (SWISH_DEBUG > 3)
            swish_debug_msg("tmp text buffer being freed");
            
        swish_xfree(out);
    }
        

    return err;
}


static void
set_encoding(swish_ParseData * parse_data, xmlChar * buffer)
{
    /* this feels like it doesn't work ... would iconv() be better ? */

    swish_xfree(parse_data->docinfo->encoding);

    if (xmlCheckUTF8((const xmlChar *) buffer) != 0)
        parse_data->docinfo->encoding = swish_xstrdup((xmlChar *) SWISH_DEFAULT_ENCODING);
    else
        parse_data->docinfo->encoding = swish_xstrdup((xmlChar *) "unknown");

}


static xmlChar *
document_encoding(xmlParserCtxtPtr ctxt)
{
    xmlChar        *enc;

    if (ctxt->encoding != NULL)
    {
        enc = swish_xstrdup(ctxt->encoding);

    }
    else if (ctxt->inputTab[0]->encoding != NULL)
    {
        enc = swish_xstrdup(ctxt->inputTab[0]->encoding);

    }
    else
    {
        /*
             * if we get here, we didn't error with bad encoding via SAX,
             * so assume it's UTF-8
             */
        enc = swish_xstrdup((xmlChar *) SWISH_DEFAULT_ENCODING);
    }

    return enc;
}


static void
tokenize(
    swish_ParseData * parse_data,
    xmlChar * string, 
    int len, 
    xmlChar * metaname,
    xmlChar * context
    )
{

    if (parse_data->analyzer->tokenize == 0)
        return;

    if (len == 0)
        return;
        
        
    if (metaname == NULL)
        metaname = parse_data->metastack->head->name;

    if (context == NULL)
        context = parse_data->metastack->flat;


    swish_WordList *tmplist;
    
    if (parse_data->analyzer->tokenizer == NULL)
    {
    
    /* use default internal tokenizer */
    
        tmplist = swish_tokenize(
                              parse_data->analyzer,
                              string,
                              parse_data->offset,
                              parse_data->word_pos,
                              metaname,
                              context
                              );


    }
    else
    {
    
    /* user-defined tokenizer */
    
        tmplist = (*parse_data->analyzer->tokenizer) (
                              parse_data->analyzer,
                              string,
                              parse_data->offset,
                              parse_data->word_pos,
                              metaname,
                              context
                              );
                              
    }

    if (tmplist->nwords == 0)
    {
        swish_free_WordList(tmplist);
        return;
    }

    /* append tmplist to master list */
    parse_data->word_pos        += tmplist->nwords;
    parse_data->docinfo->nwords += tmplist->nwords;

    if (parse_data->wordlist->head == 0)
    {
        swish_xfree(parse_data->wordlist);
        parse_data->wordlist = tmplist;
    }
    else
    {

        /* point tmp list first word's prev at current last word */
        tmplist->head->prev = parse_data->wordlist->tail;

        /* point current last word's 'next' at first word of tmp list */
        parse_data->wordlist->tail->next = tmplist->head;

        /* point current last word at last word of tmp list */
        parse_data->wordlist->tail = tmplist->tail;

        parse_data->wordlist->nwords += tmplist->nwords;

        swish_xfree(tmplist);
    }
    
    /* global offset is now the same as the tail end_offset */
    parse_data->offset = parse_data->wordlist->tail->end_offset;

}


static void 
_debug_stack(swish_TagStack * stack)
{
    int             i = 0;

    swish_debug_msg("%s '%s' stack->count: %d", stack->name, stack->flat, stack->count);

    for (stack->temp = stack->head; stack->temp != NULL; stack->temp = stack->temp->next)
    {
        swish_debug_msg("  %d: count %d  tagstack: %s",
                 i++, stack->temp->n, stack->temp->name);

    }

    if (i != stack->count)
    {
        swish_warn_err("stack count appears wrong (%d items, but count=%d)", i, stack->count);

    }
    else
    {
        swish_debug_msg("tagstack looks ok");
    }
}

/* return stack as single string of space-separated names */
static xmlChar *
flatten_tag_stack(xmlChar * tag, swish_TagStack * stack)
{
    xmlChar        *tmp;
    xmlChar        *flat;
    int             size;
    int             i;

    i = 0;
    stack->temp = stack->head;

    if (tag != NULL)
    {
        flat = swish_xstrdup(tag);
    }
    else
    {
        flat = swish_xstrdup(stack->head->name);
        stack->temp = stack->temp->next;
    }


    for (; stack->temp != NULL; stack->temp = stack->temp->next)
    {
        size = ((xmlStrlen(flat) + (xmlStrlen(stack->temp->name)) * sizeof(xmlChar))) + 2;
        tmp = swish_xmalloc(size);
        if (sprintf((char *) tmp, "%s %s", (char *) flat, (char *) stack->temp->name) > 0)
        {
            if (flat != NULL)
                swish_xfree(flat);

            flat = tmp;
        }
        else
        {
            swish_fatal_err("sprintf failed to concat %s -> %s", stack->temp->name, flat);
        }

    }

    return flat;

}
static void
add_stack_to_prop_buf(xmlChar * tag, swish_ParseData * parse_data)
{
    swish_TagStack *s = parse_data->propstack;

    if (tag != NULL)
        add_to_prop_buf(parse_data->prop_buf, parse_data->propHash, tag);

    for (s->temp = s->head; s->temp != NULL; s->temp = s->temp->next)
    {
        add_to_prop_buf(parse_data->prop_buf, parse_data->propHash, s->temp->name);
    }

}

static swish_TagStack *
push_tag_stack(swish_TagStack * stack, xmlChar * tag)
{

    swish_Tag     *thistag = swish_xmalloc(sizeof(swish_Tag));

    if (SWISH_DEBUG > 3)
    {
        swish_debug_msg(" >>>>>>> before push: tag = '%s'", tag);
        _debug_stack(stack);

    }

    /* assign this tag to the struct */
    thistag->name = swish_xstrdup(tag);

    /* increment counter */
    thistag->n = stack->count++;

    /* add to stack */
    thistag->next = stack->head;
    stack->head = thistag;

    /* cache the flattened value */
    if (stack->flat != NULL)
        swish_xfree(stack->flat);

    stack->flat = flatten_tag_stack(NULL, stack);


    if (SWISH_DEBUG > 3)
    {
        swish_debug_msg(" >>> stack size: %d  thistag count: %d  current head tag = '%s'",
                 stack->count, thistag->n, stack->head->name);

        _debug_stack(stack);

    }

    return stack;
}

static int 
pop_tag_stack(swish_TagStack * stack)
{

    if (SWISH_DEBUG > 3)
    {
        swish_debug_msg(" pop_tag_stack: %s from %s", stack->head->name, stack->name);
        _debug_stack(stack);

    }



    if (stack->count > 1)
    {
        if (SWISH_DEBUG > 3)
        {
            swish_debug_msg("  >>>  %d: popping '%s' from tagstack <<<",
                     stack->head->n, stack->head->name);

        }

        stack->temp = stack->head;
        stack->head = stack->head->next;

        /* free the memory for the popped meta */
        swish_xfree(stack->temp->name);
        stack->temp->n = 0;
        swish_xfree(stack->temp);

        stack->count--;

    }
    else
    {

        if (SWISH_DEBUG > 3)
        {
            swish_debug_msg("  >>>  %d: popping '%s' from tagstack will leave stack empty (flat: %s) <<<",
                     stack->head->n, stack->head->name, stack->flat);

        }


        stack->temp = stack->head;
        stack->head = NULL;
        swish_xfree(stack->flat);
        stack->flat = NULL;

        swish_xfree(stack->temp->name);
        stack->temp->n = 0;
        swish_xfree(stack->temp);
        stack->count--;

        return 0;
    }

    /* cache the flattened value */
    if (stack->flat != NULL)
        swish_xfree(stack->flat);

    stack->flat = flatten_tag_stack(NULL, stack);



    if (SWISH_DEBUG > 3)
    {
        swish_debug_msg("  >> stack size = %d   head of stack = %s <<", stack->count, stack->head->name);
        _debug_stack(stack);
    }

    return stack->count;

}

/* returns previous ->flat if the current tag matches the top of the stack and gets
 * popped */
static xmlChar *
pop_tag_stack_on_match(swish_TagStack * stack, xmlChar * tag)
{

    xmlChar        *prev_flat;

    prev_flat = swish_xstrdup(stack->flat);

    if (SWISH_DEBUG > 3)
    {
        swish_debug_msg("pop_tag_stack_on_match() for %s", stack->name);
        swish_debug_msg("comparing '%s' against '%s'", tag, stack->head->name);
        _debug_stack(stack);
    }


    if (xmlStrEqual(stack->head->name, tag))
    {

        if (SWISH_DEBUG > 3)
        {
            swish_debug_msg(" >>>>>>>>>>>>>>>>>>>  current tag = '%s' matches top of tagstack", tag);

        }

        /* more than default meta */
        if (pop_tag_stack(stack))
        {

            if (SWISH_DEBUG > 3)
            {
                swish_debug_msg("stack popped. tag = %s   stack->head = %s", tag, stack->head->name);
                _debug_stack(stack);
            }



        }

        /* only tag on stack */
        else if (stack->count)
        {
            if (SWISH_DEBUG > 3)
                swish_debug_msg("  using stack->head %s", stack->head->name);


        }

        return prev_flat;
    }
    else
    {
        if (SWISH_DEBUG > 3)
            swish_debug_msg("no match for '%s'", tag);

    }

    swish_xfree(prev_flat);

    return 0;
}
