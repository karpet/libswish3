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

extern int errno;
extern int SWISH_DEBUG;

// should we pass on libxml2 via SWISH_WARN()
int SWISH_PARSER_WARNINGS = 0;

static void get_env_vars(
);

static void flush_buffer(
    swish_ParserData *parser_data,
    xmlChar *metaname,
    xmlChar *context
);

static void tokenize(
    swish_ParserData *parser_data,
    xmlChar *string,
    int len,
    xmlChar *metaname,
    xmlChar *content
);

static void mystartDocument(
    void *parser_data
);
static void myendDocument(
    void *parser_data
);
static void mystartElement(
    void *parser_data,
    const xmlChar *name,
    const xmlChar **atts
);
static void myendElement(
    void *parser_data,
    const xmlChar *name
);

/* 
* SAX2 support 
*/
static void mystartElementNs(
    void *parser_data,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes
);

static void myendElementNs(
    void *ctx ATTRIBUTE_UNUSED,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI
);

static void buffer_characters(
    swish_ParserData *parser_data,
    const xmlChar *ch,
    int len
);
static void mycharacters(
    void *parser_data,
    const xmlChar *ch,
    int len
);
static void mycomments(
    void *parser_data,
    const xmlChar *ch
);
static void myerr(
    void *user_data,
    xmlChar *msg,
    ...
);

static void open_tag(
    void *data,
    const xmlChar *tag,
    xmlChar **atts
);
static void close_tag(
    void *data,
    const xmlChar *tag
);
static xmlChar *build_tag(
    swish_ParserData *parser_data,
    xmlChar *tag,
    xmlChar **atts
);

static int docparser(
    swish_ParserData *parser_data,
    xmlChar *filename,
    xmlChar *buffer,
    int size
);
static int xml_parser(
    xmlSAXHandlerPtr sax,
    void *user_data,
    xmlChar *buffer,
    int size
);
static int html_parser(
    xmlSAXHandlerPtr sax,
    void *user_data,
    xmlChar *buffer,
    int size
);
static int txt_parser(
    swish_ParserData *parser_data,
    xmlChar *buffer,
    int size
);

static swish_ParserData *init_parser_data(
    swish_3 *s3
);
static void free_parser_data(
    swish_ParserData *parser_data
);

/* 
* parsing fh/buffer headers 
*/
typedef struct
{
    xmlChar **lines;
    int body_start;
    int nlines;
} HEAD;

static HEAD *buf_to_head(
    xmlChar *buf
);
static void free_head(
    HEAD * h
);
static swish_DocInfo *head_to_docinfo(
    HEAD * h
);

static xmlChar *document_encoding(
    xmlParserCtxtPtr ctxt
);

static void set_encoding(
    swish_ParserData *parser_data,
    xmlChar *buffer
);

/* tag tracker */
static xmlChar *flatten_tag_stack(
    xmlChar *baked,
    swish_TagStack *stack
);
static void add_stack_to_prop_buf(
    xmlChar *baked,
    swish_ParserData *parser_data
);
static void push_tag_stack(
    swish_TagStack *stack,
    xmlChar *raw,
    xmlChar *baked
);
static swish_Tag *pop_tag_stack(
    swish_TagStack *stack
);
static swish_Tag *pop_tag_stack_on_match(
    swish_TagStack *stack,
    xmlChar *raw
);
static void free_swishTag(
    swish_Tag * st
);

/***********************************************************************
*                end prototypes
***********************************************************************/

swish_Parser *
swish_init_parser(
    void (*handler) (swish_ParserData *)
)
{
    swish_Parser *p = (swish_Parser *)swish_xmalloc(sizeof(swish_Parser));

    p->handler = handler;
    p->ref_cnt = 0;

/*
* libxml2 stuff 
*/
    xmlInitParser();
    xmlSubstituteEntitiesDefault(1);    /* resolve text entities */

/*
* debugging help 
*/
    get_env_vars();

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("parser ptr 0x%x", (int)p);
    }

    return p;
}

void
swish_free_parser(
    swish_Parser *p
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY) {
        SWISH_DEBUG_MSG("freeing parser");
        swish_mem_debug();
    }
    if (p->ref_cnt != 0) {
        SWISH_WARN("parser ref_cnt != 0: %d\n", p->ref_cnt);
    }
    xmlCleanupParser();
    xmlMemoryDump();
    swish_xfree(p);
}

/* 
* turn the literal xml/html tag into a swish tag for matching against
* metanames and properties 
*/
static xmlChar *
build_tag(
    swish_ParserData *parser_data,
    xmlChar *tag,
    xmlChar **atts
)
{
    int i, j, is_html_tag, size;
    xmlChar *swishtag, *attr_lower, *attr_val_lower, *alias, *metaname, *metacontent;
    swish_StringList *strlist;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG(" tag: %s (%s) ", tag, parser_data->tag);
        if (atts != NULL) {
            SWISH_DEBUG_MSG(" has attributes [%d]", xmlStrlen((xmlChar *)atts));
            for (i = 0; (atts[i] != NULL); i += 2) {
                SWISH_DEBUG_MSG(" att: %s=", atts[i]);
                if (atts[i + 1] != NULL) {
                    SWISH_DEBUG_MSG(" '%s'", atts[i + 1]);
                }
            }
        }
    }

    metaname = NULL;
    metacontent = NULL;

/*
* normalize all tags 
*/
    swishtag = swish_str_tolower(tag);

/*
* html tags 
*/
    if (parser_data->is_html) {

/*
           TODO config features about img tags and a/href tags 
*/
        if (xmlStrEqual(swishtag, (xmlChar *)"br")
            || xmlStrEqual(swishtag, (xmlChar *)"img")) {
            
            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("found html tag '%s' ... bump_word = 1", swishtag);
            parser_data->bump_word = 1;
        }
        else {
            const htmlElemDesc *element = htmlTagLookup(swishtag);

            if (!element)
                is_html_tag = 0;        /* flag that this might be a meta * name */

            else if (!element->isinline) {

/*
* need to bump word_pos so we don't match across block *
* elements 
*/
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("found html !inline tag '%s' ... bump_word = 1", swishtag);
                parser_data->bump_word = 1;

            }
            else {
            
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("found html inline tag '%s' ... bump_word = 0", swishtag);
                parser_data->bump_word = 0;
            
            }
        }

/*
* is this an HTML <meta> tag? treat 'name' attribute as a tag *
* and 'content' attribute as the tag content * we assume 'name'
* and 'content' are always in english. 
*/

        if (atts != 0) {
            for (i = 0; (atts[i] != 0); i++) {

                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("%d HTML attr: %s", i, atts[i]);

                if (xmlStrEqual(atts[i], (xmlChar *)"name")) {

/*
* SWISH_DEBUG_MSG("found name: %s", atts[i+1]); 
*/
                    metaname = (xmlChar *)atts[i + 1];
                }

                else if (xmlStrEqual(atts[i], (xmlChar *)"content")) {

/*
* SWISH_DEBUG_MSG("found content: %s", atts[i+1]); 
*/
                    metacontent = (xmlChar *)atts[i + 1];
                }

            }
        }

        if (metaname != NULL) {
            if (metacontent != NULL) {
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("found HTML meta: %s => %s", metaname, metacontent);

/*
* do not match across metas 
*/
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("found html meta tag '%s' ... bump_word = 1", metaname);
                parser_data->bump_word = 1;
                open_tag(parser_data, metaname, NULL);
                buffer_characters(parser_data, metacontent, xmlStrlen(metacontent));
                close_tag(parser_data, metaname);
                swish_xfree(swishtag);
                return NULL;

            }
            else {
                SWISH_WARN("No content for meta tag '%s'", metaname);
            }
        }

    }

/*
* xml tags 
*/
    else {

/*
* TODO make this configurable ala swish2 
*/

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("found xml tag '%s' ... bump_word = 1", swishtag);
        parser_data->bump_word = 1;

        if (atts != NULL
            && swish_hash_exists(parser_data->s3->config->stringlists,
                                 (xmlChar *)SWISH_CLASS_ATTRIBUTES)) {
            strlist =
                swish_hash_fetch(parser_data->s3->config->stringlists,
                                 (xmlChar *)SWISH_CLASS_ATTRIBUTES);

            for (i = 0; (atts[i] != NULL); i += 2) {

                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG(" %d XML attr: %s=%s [%d]", i, atts[i], atts[i + 1],
                                    xmlStrlen(atts[i + 1]));

                attr_lower = swish_str_tolower(atts[i]);
                attr_val_lower = swish_str_tolower(atts[i + 1]);

/*
                   is it one of ours? 
*/
                for (j = 0; j < strlist->n; j++) {
                    if (xmlStrEqual(strlist->word[j], attr_lower)) {
                        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                            SWISH_DEBUG_MSG("found %s: %s", attr_lower, attr_val_lower);

/*  eligible attribute name */
                        size = xmlStrlen(swishtag) + xmlStrlen(attr_val_lower) + 2;     /*  dot + NULL */
                        metaname = swish_xmalloc(size + 1);
                        snprintf((char *)metaname, size, "%s.%s", (char *)swishtag,
                                 (char *)attr_val_lower);

                        swish_xfree(swishtag);
                        swishtag = metaname;
                    }
                }

                swish_xfree(attr_lower);
                swish_xfree(attr_val_lower);

            }
        }

    }

/*
* change our internal name for this tag if it is aliased in config 
*/
    alias = swish_hash_fetch(parser_data->s3->config->tag_aliases, swishtag);
    if (alias) {

/*
* SWISH_DEBUG_MSG("%s alias -> %s", swishtag, alias); 
*/
        swish_xfree(swishtag);
        swishtag = swish_xstrdup(alias);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG(" swishtag = %s", swishtag);
    }

    return swishtag;
}

static void
flush_buffer(
    swish_ParserData *parser_data,
    xmlChar *metaname,
    xmlChar *context
)
{
    swish_TagStack *s = parser_data->metastack;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("buffer is >>%s<< before flush, word_pos = %d",
                        xmlBufferContent(parser_data->meta_buf), parser_data->word_pos);

/*
* since we only flush the buffer when metaname changes, and we do
* not want to match across metanames, bump the word_pos here before 
* parsing the string and making the tmp wordlist 
*/
    if (parser_data->word_pos)
        parser_data->word_pos++;

/*
* add meta_buf as-is to metanames buffer under current tag. this
* gives us both tokens and raw text de-tagged but organized by
* metaname. 
*/
    swish_add_buf_to_nb(parser_data->metanames, metaname, parser_data->meta_buf,
                        (xmlChar *)SWISH_TOKENPOS_BUMPER, 0, 1);

/*
*  add to every metaname on the stack.
*  Disabling this for now, as it ought to be up the handler() to decide
*  to index a token under multiple metanames, and we associate context
*  with the WordList
*/

    if (parser_data->s3->config->flags->context_as_meta) {
        for (s->temp = s->head; s->temp != NULL; s->temp = s->temp->next) {
            if (xmlStrEqual(s->temp->baked, metaname))  /*  already added */
                continue;

            swish_add_buf_to_nb(parser_data->metanames, s->temp->baked,
                                parser_data->meta_buf, (xmlChar *)SWISH_TOKENPOS_BUMPER,
                                0, 1);
        }
    }

    if (parser_data->s3->analyzer->tokenize) {
        tokenize(parser_data, (xmlChar *)xmlBufferContent(parser_data->meta_buf),
                 xmlBufferLength(parser_data->meta_buf), metaname, context);
    }

    xmlBufferEmpty(parser_data->meta_buf);

}

/* 
* SAX2 callback 
*/
static void
mystartDocument(
    void *data
)
{

/*
* swish_ParserData *parser_data = (swish_ParserData *) data; 
*/

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("startDocument()");

}

/* 
* SAX2 callback 
*/
static void
myendDocument(
    void *parser_data
)
{

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("endDocument()");

/*
* whatever's left 
*/
    flush_buffer(parser_data, (xmlChar *)SWISH_DEFAULT_METANAME,
                 (xmlChar *)SWISH_DEFAULT_METANAME);

}

/* 
* SAX1 callback 
*/
static void
mystartElement(
    void *data,
    const xmlChar *name,
    const xmlChar **atts
)
{
    open_tag(data, name, (xmlChar **)atts);
}

/* 
* SAX1 callback 
*/
static void
myendElement(
    void *data,
    const xmlChar *name
)
{
    close_tag(data, name);
}

/* 
* SAX2 handler 
*/
static void
mystartElementNs(
    void *data,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes
)
{
    int i, j, len;
    xmlChar **atts;
    atts = NULL;

    if (nb_attributes > 0) {
        atts = swish_xmalloc(((nb_attributes * 2) + 1) * sizeof(xmlChar *));
        j = 0;
        for (i = 0; i < nb_attributes * 5; i += 5) {
            atts[j] = (xmlChar *)attributes[i];
            len = (int)(attributes[i + 4] - attributes[i + 3]);
            if (len > 0) {
                atts[j + 1] = xmlStrsub(attributes[i + 3], 0, len);
            }
            else {
                atts[j] = NULL;
            }
            j += 2;
        }
        atts[j] = NULL;
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG(" tag: %s nb_attributes %d", localname, nb_attributes);
        if (atts != NULL) {
            for (i = 0; (atts[i] != NULL); i += 2) {
                SWISH_DEBUG_MSG(" att: %s=%s", atts[i], atts[i + 1]);
/* SWISH_DEBUG_MSG(" att: %s=", atts[i++], atts[i] || ""); */
            }
        }
    }

    open_tag(data, localname, atts);

    if (atts != NULL) {
        swish_xfree(atts);
    }
}

/* 
* SAX2 handler 
*/
static void
myendElementNs(
    void *data,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI
)
{
    close_tag(data, localname);
}

static void
open_tag(
    void *data,
    const xmlChar *tag,
    xmlChar **atts
)
{
    swish_ParserData *parser_data;
    parser_data = (swish_ParserData *)data;

    if (parser_data->tag != NULL)
        swish_xfree(parser_data->tag);

    parser_data->tag = build_tag(parser_data, (xmlChar *)tag, (xmlChar **)atts);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("checking config for '%s' in watched tags", parser_data->tag);

/*
* set property if this tag is configured for it 
*/
    if (swish_hash_exists(parser_data->s3->config->properties, parser_data->tag)) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG(" %s = new property", parser_data->tag);

        add_stack_to_prop_buf(NULL, parser_data);       /* NULL means all properties in the stack are added */
        xmlBufferEmpty(parser_data->prop_buf);

        push_tag_stack(parser_data->propstack, (xmlChar *)tag, parser_data->tag);

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("%s pushed ok unto propstack", parser_data->tag);
    }

/*
* likewise for metastack 
*/
    if (swish_hash_exists(parser_data->s3->config->metanames, parser_data->tag)) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG(" %s = new metaname", parser_data->tag);

        flush_buffer(parser_data, parser_data->metastack->head->baked,
                     parser_data->metastack->head->context);

        push_tag_stack(parser_data->metastack, (xmlChar *)tag, parser_data->tag);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("config check for '%s' done", parser_data->tag);

}

static void
close_tag(
    void *data,
    const xmlChar *tag
)
{
    swish_ParserData *parser_data;
    swish_Tag *st;

    parser_data = (swish_ParserData *)data;

/*
* lowercase all names for comparison against metanames (which are
* also * lowercased) 
*/
    if (parser_data->tag != NULL)
        swish_xfree(parser_data->tag);

    parser_data->tag = build_tag(parser_data, (xmlChar *)tag, NULL);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG(" endElement(%s) (%s)", (xmlChar *)tag, parser_data->tag);

    if ((st = pop_tag_stack_on_match(parser_data->propstack, (xmlChar *)tag)) != NULL) {

        add_stack_to_prop_buf(st->baked, parser_data);
        xmlBufferEmpty(parser_data->prop_buf);
        free_swishTag(st);
    }

    if ((st = pop_tag_stack_on_match(parser_data->metastack, (xmlChar *)tag)) != NULL) {

        flush_buffer(parser_data, st->baked, st->context);
        free_swishTag(st);
    }

}

/* 
* handle all characters in doc 
*/
static void
buffer_characters(
    swish_ParserData *parser_data,
    const xmlChar *ch,
    int len
)
{
    int i;
    xmlChar output[len];
    xmlBufferPtr buf = parser_data->meta_buf;

/*
* why not wchar_t ? len is number of bytes, not number of
* characters, so xmlChar (i.e., char) works
*/

/*
* SWISH_DEBUG_MSG( "sizeof output buf is %d; len was %d\n", sizeof(output),
* len );
*/

/*
* SWISH_DEBUG_MSG( "characters"); 
*/

    for (i = 0; i < len; i++) {
        output[i] = ch[i];
    }
    output[i] = (xmlChar)NULL;

    if (parser_data->bump_word && xmlBufferLength(buf)) {
        swish_append_buffer(buf, (xmlChar *)SWISH_TOKENPOS_BUMPER, 1);
    }
    
    swish_append_buffer(buf, output, len);

    if (parser_data->bump_word && xmlBufferLength(parser_data->prop_buf)) {
        swish_append_buffer(parser_data->prop_buf, (xmlChar *)SWISH_TOKENPOS_BUMPER, 1);
    }
    else if (xmlBufferLength(parser_data->prop_buf)) {
        swish_append_buffer(parser_data->prop_buf, (xmlChar*)" ", 1);
    }

    swish_append_buffer(parser_data->prop_buf, output, len);
}

/* 
* SAX2 callback 
*/
static void
mycharacters(
    void *parser_data,
    const xmlChar *ch,
    int len
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG(" >> mycharacters()");

    buffer_characters(parser_data, ch, len);
}

/* 
* SAX2 callback 
*/
static void
mycomments(
    void *parser_data,
    const xmlChar *ch
)
{
    int len = strlen((char *)(char *)ch);

/*
* TODO: make comments indexing optional 
*/

/*
* TODO: enable noindex option 
*/
    return;

    buffer_characters(parser_data, ch, len);
}

/* 
* SAX2 callback 
*/
static void
myerr(
    void *data,
    xmlChar *msg,
    ...
)
{
    swish_ParserData *parser_data;
    va_list args;
    char str[1000];

    if (!SWISH_PARSER_WARNINGS)
        return;

    parser_data = (swish_ParserData *)data;

    SWISH_WARN("libxml2 error for %s:", parser_data->docinfo->uri);

    va_start(args, msg);
    vsnprintf((char *)str, 1000, (char *)msg, args);
    xmlParserError(parser_data->ctxt, (char *)str);
    va_end(args);
}

/* 
* SAX2 callback 
*/
static void
mywarn(
    void *user_data,
    xmlChar *msg,
    ...
)
{
    swish_ParserData *parser_data;
    va_list args;
    char str[1000];

    if (!SWISH_PARSER_WARNINGS)
        return;

    parser_data = (swish_ParserData *)user_data;

    SWISH_WARN("libxml2 warning for %s:", parser_data->docinfo->uri);

    va_start(args, msg);
    vsnprintf((char *)str, 1000, (char *)msg, args);
    xmlParserWarning(parser_data->ctxt, (char *)str);
    va_end(args);
}

/* 
* SAX2 handler struct for html and xml parsing 
*/

xmlSAXHandler my_parser = {
    NULL,                       /* internalSubset */
    NULL,                       /* isStandalone */
    NULL,                       /* hasInternalSubset */
    NULL,                       /* hasExternalSubset */
    NULL,                       /* resolveEntity */
    NULL,                       /* getEntity */
    NULL,                       /* entityDecl */
    NULL,                       /* notationDecl */
    NULL,                       /* attributeDecl */
    NULL,                       /* elementDecl */
    NULL,                       /* unparsedEntityDecl */
    NULL,                       /* setDocumentLocator */
    mystartDocument,            /* startDocument */
    myendDocument,              /* endDocument */
    mystartElement,             /* startElement */
    myendElement,               /* endElement */
    NULL,                       /* reference */
    mycharacters,               /* characters */
    NULL,                       /* ignorableWhitespace */
    NULL,                       /* processingInstruction */
    mycomments,                 /* comment */
    (warningSAXFunc) & mywarn,  /* xmlParserWarning */
    (errorSAXFunc) & myerr,     /* xmlParserError */
    (fatalErrorSAXFunc) & myerr,        /* xmlfatalParserError */
    NULL,                       /* getParameterEntity */
    NULL,                       /* cdataBlock -- should we handle this too *
                                 * ?? */
    NULL,                       /* externalSubset; */
    XML_SAX2_MAGIC,
    NULL,
    mystartElementNs,           /* startElementNs */
    myendElementNs,             /* endElementNs */
    NULL                        /* xmlStructuredErrorFunc */
};

xmlSAXHandlerPtr my_parser_ptr = &my_parser;

static int
docparser(
    swish_ParserData *parser_data,
    xmlChar *filename,
    xmlChar *buffer,
    int size
)
{

    int ret;
    xmlChar *mime = (xmlChar *)parser_data->docinfo->mime;
    xmlChar *parser = (xmlChar *)parser_data->docinfo->parser;

    if (!size && !xmlStrlen(buffer) && !parser_data->docinfo->size) {
        SWISH_WARN("%s appears to be empty -- can't parse it", parser_data->docinfo->uri);

        return 1;
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("%s -- using %s parser", parser_data->docinfo->uri, parser);

/*
* slurp file if not already in memory 
*/
    if (filename && !buffer) {
        buffer = swish_slurp_file_len(filename, (long)parser_data->docinfo->size);
        size = parser_data->docinfo->size;
    }

    if (parser[0] == 'H') {
        parser_data->is_html = 1;
        ret = html_parser(my_parser_ptr, parser_data, buffer, size);
    }

    else if (parser[0] == 'X')
        ret = xml_parser(my_parser_ptr, parser_data, buffer, size);

    else if (parser[0] == 'T')
        ret = txt_parser(parser_data, (xmlChar *)buffer, size);

    else
        SWISH_CROAK("no parser known for MIME '%s'", mime);

    if (filename) {

/*
* SWISH_DEBUG_MSG( "freeing buffer"); 
*/
        swish_xfree(buffer);
    }

    return ret;

}

static swish_ParserData *
init_parser_data(
    swish_3 *s3
)
{

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("init parser_data");

    swish_ParserData *ptr = (swish_ParserData *)swish_xmalloc(sizeof(swish_ParserData));

    ptr->s3 = s3;
    ptr->s3->ref_cnt++;

    ptr->meta_buf = xmlBufferCreateSize(SWISH_BUFFER_CHUNK_SIZE);
    ptr->prop_buf = xmlBufferCreateSize(SWISH_BUFFER_CHUNK_SIZE);

    ptr->tag = NULL;
    ptr->wordlist = swish_init_wordlist();
    ptr->wordlist->ref_cnt++;
    ptr->token_iterator = swish_init_token_iterator(s3->config, swish_init_token_list());
    ptr->token_iterator->ref_cnt++;
    ptr->properties = swish_init_nb(s3->config->properties);
    ptr->properties->ref_cnt++;
    ptr->metanames = swish_init_nb(s3->config->metanames);
    ptr->metanames->ref_cnt++;

/*
*   pick a tokenizer if one has not been explicitly set
*/
    if (s3->analyzer->tokenizer == NULL) {
        if (s3->analyzer->tokenlist) {
            s3->analyzer->tokenizer = (&swish_tokenize3);
        }
        else {
            s3->analyzer->tokenizer = (&swish_tokenize);
        }
    }

/*
* prime the stacks 
*/
    ptr->metastack = (swish_TagStack *)swish_xmalloc(sizeof(swish_TagStack));
    ptr->metastack->name = "MetaStack";
    ptr->metastack->head = NULL;
    ptr->metastack->temp = NULL;
    ptr->metastack->count = 0;
    push_tag_stack(ptr->metastack, (xmlChar *)SWISH_DEFAULT_METANAME,
                   (xmlChar *)SWISH_DEFAULT_METANAME);

    ptr->propstack = (swish_TagStack *)swish_xmalloc(sizeof(swish_TagStack));
    ptr->propstack->name = "PropStack";
    ptr->propstack->head = NULL;
    ptr->propstack->temp = NULL;
    ptr->propstack->count = 0;
    push_tag_stack(ptr->propstack, (xmlChar *)"_", (xmlChar *)"_");

/*
* no such property just to seed stack 
*/

/*
* gets toggled per-tag 
*/
    ptr->bump_word = 1;

/*
* toggle 
*/
    ptr->no_index = 0;

/*
* shortcut rather than looking parser up in hash for each tag event 
*/
    ptr->is_html = 0;

/*
* must be zero so that ++ works ok on first word 
*/
    ptr->word_pos = 0;

/*
* always start at first byte 
*/
    ptr->offset = 0;

/*
* pointer to the xmlParserCtxt since we want to free it only after
* we're completely done with it. NOTE this is a change per libxml2
* vers > 2.6.16 
*/
    ptr->ctxt = NULL;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("init done for parser_data");

    return ptr;

}

static void
free_parser_data(
    swish_ParserData *ptr
)
{
    swish_Tag *st;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData");

/*
* dec ref count for shared ptr 
*/
    ptr->s3->ref_cnt--;

/*
* Pop the stacks 
*/
    while ((st = pop_tag_stack(ptr->metastack)) != NULL) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("%s %d POP %s [%s] [%s]", ptr->metastack->name,
                            ptr->metastack->count, st->raw, st->baked, st->context);

        free_swishTag(st);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData metastack");

    swish_xfree(ptr->metastack);

    while ((st = pop_tag_stack(ptr->propstack)) != NULL) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("%s %d POP %s [%s] [%s]", ptr->propstack->name,
                            ptr->propstack->count, st->raw, st->baked, st->context);

        free_swishTag(st);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData propstack");

    swish_xfree(ptr->propstack);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData properties");

    ptr->properties->ref_cnt--;
    swish_free_nb(ptr->properties);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData metanames");

    ptr->metanames->ref_cnt--;
    swish_free_nb(ptr->metanames);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData xmlBuffer");

    xmlBufferFree(ptr->meta_buf);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData prop xmlBuffer");

    xmlBufferFree(ptr->prop_buf);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData tag");

    if (ptr->tag != NULL)
        swish_xfree(ptr->tag);

    if (ptr->ctxt != NULL) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("freeing swish_ParserData libxml2 parser ctxt");

        if (xmlStrEqual(ptr->docinfo->parser, (xmlChar *)SWISH_PARSER_XML))
            xmlFreeParserCtxt(ptr->ctxt);

        if (xmlStrEqual(ptr->docinfo->parser, (xmlChar *)SWISH_PARSER_HTML))
            htmlFreeParserCtxt(ptr->ctxt);
    }
    else {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("swish_ParserData libxml2 parser ctxt already freed");

    }

    if (ptr->wordlist != NULL) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("free swish_ParserData wordList");

        ptr->wordlist->ref_cnt--;
        swish_free_wordlist(ptr->wordlist);
    }

    if (ptr->token_iterator != NULL) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("free swish_ParserData TokenIterator");

        ptr->token_iterator->tl->ref_cnt--;
        swish_free_token_list(ptr->token_iterator->tl);
        ptr->token_iterator->ref_cnt--;
        swish_free_token_iterator(ptr->token_iterator);
    }

    if (ptr->docinfo != NULL) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("free swish_ParserData docinfo");

        ptr->docinfo->ref_cnt--;
        swish_free_docinfo(ptr->docinfo);

    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData ptr");

    swish_xfree(ptr);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("swish_ParserData all freed");
}

static HEAD *
buf_to_head(
    xmlChar *buf
)
{
    int i, j, k;
    xmlChar *line;
    HEAD *h;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("parsing buffer into head: %s", buf);

    h = swish_xmalloc(sizeof(HEAD));
    h->lines = swish_xmalloc(SWISH_MAX_HEADERS * sizeof(line));
    h->nlines = 0;
    h->body_start = 0;
    line = swish_xmalloc(SWISH_MAXSTRLEN + 1);
    i = 0;
    j = 0;
    k = 0;

    while (j < SWISH_MAX_HEADERS && i <= SWISH_MAXSTRLEN) {

/*
* SWISH_DEBUG_MSG( "i = %d j = %d k = %d", i, j, k); 
*/

        if (buf[k] == '\n') {
            SWISH_CROAK("illegal newline to start doc header");
        }
        line[i] = buf[k];

/*
* fprintf(stderr, "%c", line[i]); 
*/
        i++;
        k++;

        if (buf[k] == '\n') {

            line[i] = '\0';
            h->lines[j++] = swish_xstrdup(line);
            h->nlines++;

/*
* get to the next char no matter what, then check if == '\n' 
*/
            k++;

            if (buf[k] == '\n') {

/*
* fprintf(stderr, "found blank line at byte %d\n", k); 
*/
                h->body_start = k + 1;
                break;
            }
            i = 0;

            continue;
        }
    }

    swish_xfree(line);

    return h;
}

static swish_DocInfo *
head_to_docinfo(
    HEAD * h
)
{
    int i;
    xmlChar *val, *line;

    swish_DocInfo *info = swish_init_docinfo();

    info->ref_cnt++;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("preparing to parse %d header lines", h->nlines);

    for (i = 0; i < h->nlines; i++) {

        line = h->lines[i];
        val = (xmlChar *)xmlStrchr(line, ':');
        val = swish_str_skip_ws(++val);

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("%d parsing header line: %s", i, line);

        }

        if (!xmlStrncasecmp(line, (const xmlChar *)"Content-Length", 14)) {
            if (!val)
                SWISH_WARN("Failed to parse Content-Length header '%s'", line);

            info->size = swish_string_to_int((char *)val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Last-Modified", 13)) {

            if (!val)
                SWISH_WARN("Failed to parse Last-Modified header '%s'", line);

            info->mtime = swish_string_to_int((char *)val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Last-Mtime", 10)) {

            SWISH_WARN("Last-Mtime is deprecated in favor of Last-Modified");

            if (!val)
                SWISH_WARN("Failed to parse Last-Mtime header '%s'", line);

            info->mtime = swish_string_to_int((char *)val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Content-Location", 16)) {

            if (!val)
                SWISH_WARN("Failed to parse Content-Location header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find path name in Content-Location header '%s'",
                           line);

            if (info->uri != NULL)
                swish_xfree(info->uri);

            info->uri = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Path-Name", 9)) {

            SWISH_WARN("Path-Name is deprecated in favor of Content-Location");

            if (!val)
                SWISH_WARN("Failed to parse Path-Name header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find path name in Path-Name header '%s'", line);

            if (info->uri != NULL)
                swish_xfree(info->uri);

            info->uri = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Document-Type", 13)) {

            SWISH_WARN("Document-Type is deprecated in favor of Parser-Type");

            if (!val)
                SWISH_WARN("Failed to parse Document-Type header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find path name in Document-Type header '%s'", line);

            if (info->parser != NULL)
                swish_xfree(info->parser);

            info->parser = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Parser-Type", 11)) {

            if (!val)
                SWISH_WARN("Failed to parse Parser-Type header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find path name in Parser-Type header '%s'", line);

            if (info->parser != NULL)
                swish_xfree(info->parser);

            info->parser = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Content-Type", 12)) {

            if (!val)
                SWISH_WARN("Failed to parse Content-Type header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find path name in Content-Type header '%s'", line);

/*
* TODO: get encoding out of this line too if
* present. example:   text/xml; charset=ISO-8859-1
*/

            if (info->mime != NULL)
                swish_xfree(info->mime);

            info->mime = swish_xstrdup(val);
            continue;
        }
        if (!xmlStrncasecmp(line, (const xmlChar *)"Encoding", 8)
            || !xmlStrncasecmp(line, (const xmlChar *)"Charset", 7)) {

            if (!val)
                SWISH_WARN("Failed to parse Encoding or Charset header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find value in Encoding or Charset header '%s'",
                           line);

            if (info->encoding != NULL)
                swish_xfree(info->encoding);

            info->encoding = swish_xstrdup(val);
            continue;
        }

/*
* TODO update mode is a vers2 btree feature. still unclear if
* we'll actually support it
*/
        if (!xmlStrncasecmp(line, (const xmlChar *)"Update-Mode", 11)) {

            if (!val)
                SWISH_WARN("Failed to parse Update-Mode header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find value in Update-Mode header '%s'", line);

            if (info->update != NULL)
                swish_xfree(info->update);

            info->update = swish_xstrdup(val);
            continue;
        }

/*
* if we get here, unrecognized header line 
*/
        SWISH_WARN("Unknown header line: '%s'\n", line);

    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("returning %d header lines", h->nlines);
        swish_debug_docinfo(info);
    }

    return info;
}

static void
get_env_vars(
)
{

/*
* init the global env vars, but don't override if already set 
*/

    setenv("SWISH_PARSER_WARNINGS", "0", 0);
    SWISH_PARSER_WARNINGS = swish_string_to_int(getenv("SWISH_PARSER_WARNINGS"));

    if (SWISH_DEBUG) {
        SWISH_PARSER_WARNINGS = SWISH_DEBUG;
    }
}

int
swish_parse_fh(
    swish_3 *s3,
    FILE * fh
)
{
    xmlChar *ln;
    HEAD *head;
    int i;
    xmlChar *read_buffer;
    xmlBufferPtr head_buf;
    swish_ParserData *parser_data;
    int xmlErr;
    int min_headers, nheaders;
    double curTime;
    char *etime;
    int file_cnt;

    i = 0;
    file_cnt = 0;
    nheaders = 0;
    min_headers = 2;

    if (fh == NULL)
        fh = stdin;

    ln = swish_xmalloc(SWISH_MAXSTRLEN + 1);
    head_buf =
        xmlBufferCreateSize((SWISH_MAX_HEADERS * SWISH_MAXSTRLEN) + SWISH_MAX_HEADERS);

/*
* based on extprog.c 
*/
    while (fgets((char *)ln, SWISH_MAXSTRLEN, fh) != 0) {

/*
* we don't use fgetws() because we don't care about * indiv
* characters yet 
*/

        xmlChar *end;
        xmlChar *line;

        line = swish_str_skip_ws(ln);   /* skip leading white space */
        end = (xmlChar *)strrchr((char *)line, '\n');

/*
* trim any white space at end of doc, including \n 
*/
        if (end) {
            while (end > line && isspace((int)*(end - 1)))
                end--;

            *end = '\0';
        }

        if (nheaders >= min_headers && xmlStrlen(line) == 0) {

/*
* blank line indicates body 
*/
            curTime = swish_time_elapsed();
            parser_data = init_parser_data(s3);
            head = buf_to_head((xmlChar *)xmlBufferContent(head_buf));
            parser_data->docinfo = head_to_docinfo(head);
            swish_check_docinfo(parser_data->docinfo, s3->config);

            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("reading %ld bytes from filehandle",
                                (long int)parser_data->docinfo->size);

            read_buffer = swish_slurp_fh(fh, parser_data->docinfo->size);

/*
* parse 
*/
            xmlErr =
                docparser(parser_data, NULL, read_buffer, parser_data->docinfo->size);

            if (xmlErr)
                SWISH_WARN("parser returned error %d", xmlErr);

            if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
                SWISH_DEBUG_MSG
                    ("\n===============================================================\n");
                swish_debug_docinfo(parser_data->docinfo);
                SWISH_DEBUG_MSG("  word buffer length: %d bytes",
                                xmlBufferLength(parser_data->meta_buf));
                SWISH_DEBUG_MSG(" (%d words)", parser_data->docinfo->nwords);
            }
            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("passing to handler");

/*
* pass to callback function 
*/
            (*s3->parser->handler) (parser_data);

            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("handler done");

/*
* reset everything for next time 
*/

            swish_xfree(read_buffer);
            free_parser_data(parser_data);
            free_head(head);
            xmlBufferEmpty(head_buf);
            nheaders = 0;

/*
* count the file 
*/
            file_cnt++;

            if (SWISH_DEBUG) {
                etime = swish_print_fine_time(swish_time_elapsed() - curTime);
                SWISH_DEBUG_MSG("%s elapsed time", etime);
                swish_xfree(etime);
            }

/*
* timer 
*/
            curTime = swish_time_elapsed();

            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG
                    ("\n================ filehandle - done with file ===================\n");

        }
        else if (xmlStrlen(line) == 0) {

            SWISH_CROAK("Not enough header lines reading from filehandle");

        }
        else {

/*
* we are reading headers 
*/
            if (xmlBufferAdd(head_buf, line, -1))
                SWISH_CROAK("error adding header to buffer");

            if (xmlBufferCCat(head_buf, "\n"))
                SWISH_CROAK("can't add newline to end of header buffer");

            nheaders++;
        }

    }

    if (xmlBufferLength(head_buf)) {
        SWISH_CROAK("Some unparsed header lines remaining");
    }

    swish_xfree(ln);
    xmlBufferFree(head_buf);

    return file_cnt;
}

static void
free_head(
    HEAD * h
)
{
    int i;

    for (i = 0; i < h->nlines; i++) {
        swish_xfree(h->lines[i]);
    }
    swish_xfree(h->lines);
    swish_xfree(h);
}

/* 
* PUBLIC 
*/

/* 
* pass in a string including headers. like parsing fh, but only for one
* doc
*/
int
swish_parse_buffer(
    swish_3 *s3,
    xmlChar *buf
)
{

    int res;
    double curTime = swish_time_elapsed();
    HEAD *head;
    char *etime;

    head = buf_to_head(buf);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("number of headlines: %d", head->nlines);

    swish_ParserData *parser_data = init_parser_data(s3);

    parser_data->docinfo = head_to_docinfo(head);
    swish_check_docinfo(parser_data->docinfo, s3->config);

/*
* reposition buf pointer at start of body (just past head) 
*/

    buf += head->body_start;

    res = docparser(parser_data, 0, buf, xmlStrlen(buf));

/*
* pass to callback function 
*/
    (*s3->parser->handler) (parser_data);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        swish_debug_docinfo(parser_data->docinfo);
        SWISH_DEBUG_MSG("  word buffer length: %d bytes",
                        xmlBufferLength(parser_data->meta_buf));
        SWISH_DEBUG_MSG(" (%d words)", parser_data->docinfo->nwords);
    }

/*
* free buffers 
*/
    free_head(head);
    free_parser_data(parser_data);

    if (SWISH_DEBUG) {
        etime = swish_print_fine_time(swish_time_elapsed() - curTime);
        SWISH_DEBUG_MSG("%s elapsed time", etime);
        swish_xfree(etime);
    }
    curTime = swish_time_elapsed();

    return res;

}

/* 
* PUBLIC 
*/
int
swish_parse_file(
    swish_3 *s3,
    xmlChar *filename
)
{
    int res;
    double curTime = swish_time_elapsed();
    char *etime;

    swish_ParserData *parser_data = init_parser_data(s3);

    parser_data->docinfo = swish_init_docinfo();
    parser_data->docinfo->ref_cnt++;

    if (!swish_docinfo_from_filesystem(filename, parser_data->docinfo, parser_data)) {
        SWISH_WARN("Skipping %s", filename);
        free_parser_data(parser_data);
        return 1;
    }

    res = docparser(parser_data, filename, 0, 0);

/*
* pass to callback function 
*/
    (*s3->parser->handler) (parser_data);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        swish_debug_docinfo(parser_data->docinfo);
        SWISH_DEBUG_MSG("  word buffer length: %d bytes",
                        xmlBufferLength(parser_data->meta_buf));
        SWISH_DEBUG_MSG(" (%d words)", parser_data->docinfo->nwords);
    }

/*
* free buffers 
*/
    free_parser_data(parser_data);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        etime = swish_print_fine_time(swish_time_elapsed() - curTime);
        SWISH_DEBUG_MSG("%s elapsed time", etime);
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
    xmlChar *buffer,
    int size
)
{
    int ret = 0;
    xmlParserCtxtPtr ctxt;
    swish_ParserData *parser_data = (swish_ParserData *)user_data;
    xmlSAXHandlerPtr oldsax = NULL;

    if (sax == NULL)
        return -1;
    ctxt = xmlCreateMemoryParserCtxt((const char *)buffer, size);
    if (ctxt == NULL)
        return -1;
    oldsax = ctxt->sax;
    ctxt->sax = sax;
    ctxt->sax2 = 1;

/*
* always use sax2 -- this pulled from xmlDetextSAX2() 
*/
    ctxt->str_xml = xmlDictLookup(ctxt->dict, BAD_CAST "xml", 3);
    ctxt->str_xmlns = xmlDictLookup(ctxt->dict, BAD_CAST "xmlns", 5);
    ctxt->str_xml_ns = xmlDictLookup(ctxt->dict, XML_XML_NAMESPACE, 36);
    if ((ctxt->str_xml == NULL) || (ctxt->str_xmlns == NULL)
        || (ctxt->str_xml_ns == NULL)) {

/*
* xmlErrMemory is/was not a public func but is in
* parserInternals.h * basically, this is a bad, fatal error, so
* we'll just die 
*/

/*
* xmlErrMemory(ctxt, NULL); 
*/
        SWISH_CROAK("Fatal libxml2 memory error");
    }

    if (user_data != NULL)
        ctxt->userData = user_data;

    xmlParseDocument(ctxt);

    if (ctxt->wellFormed)
        ret = 0;
    else {
        if (ctxt->errNo != 0)
            ret = ctxt->errNo;
        else
            ret = -1;
    }
    ctxt->sax = oldsax;
    if (ctxt->myDoc != NULL) {
        xmlFreeDoc(ctxt->myDoc);
        ctxt->myDoc = NULL;
    }

    if (parser_data->docinfo->encoding != NULL)
        swish_xfree(parser_data->docinfo->encoding);

    parser_data->docinfo->encoding = document_encoding(ctxt);

    xmlFreeParserCtxt(ctxt);

    return ret;
}

static int
html_parser(
    xmlSAXHandlerPtr sax,
    void *user_data,
    xmlChar *buffer,
    int size
)
{
    htmlDocPtr ret;
    htmlParserCtxtPtr ctxt;
    htmlSAXHandlerPtr oldsax = 0;
    swish_ParserData *parser_data = (swish_ParserData *)user_data;

    xmlInitParser();

    ctxt = htmlCreateMemoryParserCtxt((const char *)buffer, xmlStrlen(buffer));

    parser_data->ctxt = ctxt;

    if (parser_data->docinfo->encoding != NULL)
        swish_xfree(parser_data->docinfo->encoding);

    parser_data->docinfo->encoding = document_encoding(ctxt);

    if (parser_data->docinfo->encoding == NULL)
        set_encoding(parser_data, (xmlChar *)buffer);

    if (ctxt == 0)
        return (0);
    if (sax != 0) {
        oldsax = ctxt->sax;
        ctxt->sax = (htmlSAXHandlerPtr) sax;
        ctxt->userData = parser_data;
    }
    htmlParseDocument(ctxt);

    if (sax != 0) {
        ctxt->sax = oldsax;
        ctxt->userData = 0;
    }

    return ret ? 0 : 1;
}

static int
txt_parser(
    swish_ParserData *parser_data,
    xmlChar *buffer,
    int size
)
{
    int err = 0;
    xmlChar *out, *enc;
    int outlen;

    out = NULL;
    enc = (xmlChar *)getenv("SWISH_ENCODING");

/*
* TODO better encoding detection. for now we assume unknown text
* files are latin1 
*/
    set_encoding(parser_data, buffer);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("txt parser encoding: %s", parser_data->docinfo->encoding);

    if (parser_data->docinfo->encoding != (xmlChar *)SWISH_DEFAULT_ENCODING) {
        if (!xmlStrncasecmp(parser_data->docinfo->encoding, (xmlChar *)"iso-8859-1", 10)) {
            out = swish_xmalloc(size * 2);

            if (!isolat1ToUTF8(out, &outlen, buffer, &size)) {
                SWISH_WARN("could not convert buf from iso-8859-1");
            }

            size = outlen;
            buffer = out;
        }

        else if (xmlStrEqual(parser_data->docinfo->encoding, (xmlChar *)"unknown")) {
            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("default env encoding -> %s", enc);

            if (xmlStrncasecmp(enc, (xmlChar *)"iso-8859-1", 10)) {
                SWISH_WARN
                    ("%s encoding is unknown (not %s) but LC_CTYPE is %s -- assuming file is %s",
                     parser_data->docinfo->uri, SWISH_DEFAULT_ENCODING, enc,
                     "iso-8859-1");

            }

            out = swish_xmalloc(size * 2);

            if (!isolat1ToUTF8(out, &outlen, buffer, &size)) {
                SWISH_WARN("could not convert buf from iso-8859-1");
            }

            size = outlen;
            buffer = out;

        }
    }

/*
* we obviously haven't any tags on which to trigger our metanames,
* so set default
* TODO get title somehow?
* TODO check config to determine if we should buffer swish_prop_description etc
*/

    push_tag_stack(parser_data->metastack, (xmlChar *)SWISH_DEFAULT_METANAME,
                   (xmlChar *)SWISH_DEFAULT_METANAME);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("%s stack PUSH %s", parser_data->metastack->head->context);

    buffer_characters(parser_data, buffer, size);
    flush_buffer(parser_data, (xmlChar *)SWISH_DEFAULT_METANAME,
                 (xmlChar *)SWISH_DEFAULT_METANAME);

    if (out != NULL) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("tmp text buffer being freed");

        swish_xfree(out);
    }

    return err;
}

static void
set_encoding(
    swish_ParserData *parser_data,
    xmlChar *buffer
)
{

/*
* this feels like it doesn't work ... would iconv() be better ? 
*/

    swish_xfree(parser_data->docinfo->encoding);

    if (xmlCheckUTF8((const xmlChar *)buffer) != 0)
        parser_data->docinfo->encoding = swish_xstrdup((xmlChar *)SWISH_DEFAULT_ENCODING);
    else
        parser_data->docinfo->encoding = swish_xstrdup((xmlChar *)"unknown");

}

static xmlChar *
document_encoding(
    xmlParserCtxtPtr ctxt
)
{
    xmlChar *enc;

    if (ctxt->encoding != NULL) {
        enc = swish_xstrdup(ctxt->encoding);

    }
    else if (ctxt->inputTab[0]->encoding != NULL) {
        enc = swish_xstrdup(ctxt->inputTab[0]->encoding);

    }
    else {

/*
* if we get here, we didn't error with bad encoding via SAX,
* so assume it's UTF-8
*/
        enc = swish_xstrdup((xmlChar *)SWISH_DEFAULT_ENCODING);
    }

    return enc;
}

static void
tokenize(
    swish_ParserData *parser_data,
    xmlChar *string,
    int len,
    xmlChar *metaname,
    xmlChar *context
)
{
    swish_MetaName *meta;

    meta = swish_hash_fetch(parser_data->s3->config->metanames, metaname);

    if (len == 0)
        return;

    if (metaname == NULL)
        metaname = parser_data->metastack->head->baked;

    if (context == NULL)
        context = parser_data->metastack->head->context;

    swish_WordList *tmplist;
    
    if (parser_data->s3->analyzer->tokenlist) {

/*
* array buffer (token_iterator) tokenizer
*/

        parser_data->docinfo->nwords +=
            (*parser_data->s3->analyzer->tokenizer) (parser_data->s3, string,
                                                     parser_data->token_iterator->tl,
                                                     meta, context);
        return;

    }
    else {

/*
* linked-list (wordlist) tokenizer
*/

        tmplist = swish_init_wordlist();
        tmplist->ref_cnt++;
        parser_data->docinfo->nwords +=
            (*parser_data->s3->analyzer->tokenizer) (parser_data->s3, string, tmplist,
                                                     parser_data->offset,
                                                     parser_data->word_pos, metaname,
                                                     context);

        if (tmplist->nwords == 0) {
            tmplist->ref_cnt--;
            swish_free_wordlist(tmplist);
            return;
        }

/*
*  append tmplist to master list 
*/
        parser_data->word_pos += tmplist->nwords;

        if (parser_data->wordlist->head == 0) {
            swish_xfree(parser_data->wordlist);
            parser_data->wordlist = tmplist;
        }
        else {

/*
* point tmp list first word's prev at current last word 
*/
            tmplist->head->prev = parser_data->wordlist->tail;

/*
* point current last word's 'next' at first word of tmp list 
*/
            parser_data->wordlist->tail->next = tmplist->head;

/*
* point current last word at last word of tmp list 
*/
            parser_data->wordlist->tail = tmplist->tail;

            parser_data->wordlist->nwords += tmplist->nwords;

            swish_xfree(tmplist);
        }

/*
* global offset is now the same as the tail end_offset 
*/
        parser_data->offset = parser_data->wordlist->tail->end_offset;

    }

}

static void
_debug_stack(
    swish_TagStack *stack
)
{
    int i = 0;

    SWISH_DEBUG_MSG("%s stack->count: %d", stack->name, stack->count);

    for (stack->temp = stack->head; stack->temp != NULL; stack->temp = stack->temp->next) {
        SWISH_DEBUG_MSG("  %d: count %d  tagstack: %s", i++, stack->temp->n,
                        stack->temp->raw);

    }

    if (i != stack->count) {
        SWISH_WARN("stack count appears wrong (%d items, but count=%d)", i, stack->count);

    }
    else {
        SWISH_DEBUG_MSG("tagstack looks ok");
    }
}

/* 
* return stack as single string of space-separated names 
*/
static xmlChar *
flatten_tag_stack(
    xmlChar *baked,
    swish_TagStack *stack
)
{
    xmlChar *tmp;
    xmlChar *flat;
    int size;
    int i;

    i = 0;
    stack->temp = stack->head;

    if (baked != NULL) {
        flat = swish_xstrdup(baked);
    }
    else {
        flat = swish_xstrdup(stack->head->baked);
        stack->temp = stack->temp->next;
    }

    for (; stack->temp != NULL; stack->temp = stack->temp->next) {
        size =
            ((xmlStrlen(flat) + (xmlStrlen(stack->temp->baked)) * sizeof(xmlChar))) + 2;
        tmp = swish_xmalloc(size);
        if (snprintf((char *)tmp, size, "%s %s", (char *)flat, (char *)stack->temp->baked)
            > 0) {
            if (flat != NULL)
                swish_xfree(flat);

            flat = tmp;
        }
        else {
            SWISH_CROAK("sprintf failed to concat %s -> %s", stack->temp->baked, flat);
        }

    }

    return flat;

}
static void
add_stack_to_prop_buf(
    xmlChar *baked,
    swish_ParserData *parser_data
)
{
    swish_TagStack *stack;
    int cleanwsp;
    swish_Property *prop;

    stack = parser_data->propstack;
    cleanwsp = 1;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("adding property %s to buffer", baked);
    }

    if (baked != NULL) {
        prop = swish_hash_fetch(parser_data->s3->config->properties, baked);
        if (prop->verbatim)
            cleanwsp = 0;

        swish_add_buf_to_nb(parser_data->properties, baked, parser_data->prop_buf,
                            (xmlChar *)SWISH_TOKENPOS_BUMPER, cleanwsp, 0);

    }

/*  add for each member in the stack */
/*  TODO configurable?? */
    for (stack->temp = stack->head; stack->temp != NULL; stack->temp = stack->temp->next) {
        if (xmlStrEqual(stack->temp->baked, (xmlChar *)"_"))
            continue;

        swish_add_buf_to_nb(parser_data->properties, stack->temp->baked,
                            parser_data->prop_buf, (xmlChar *)SWISH_TOKENPOS_BUMPER,
                            cleanwsp, 0);
    }

}

static void
free_swishTag(
    swish_Tag * st
)
{
    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG(" freeing swishTag: %s %s %s", st->raw, st->baked, st->context);
    }

    swish_xfree(st->raw);
    swish_xfree(st->baked);
    swish_xfree(st->context);
    swish_xfree(st);
}

static void
push_tag_stack(
    swish_TagStack *stack,
    xmlChar *raw,
    xmlChar *baked
)
{

    swish_Tag *thistag = swish_xmalloc(sizeof(swish_Tag));

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("%s PUSH: tag = '%s'", stack->name, raw);
        _debug_stack(stack);
    }

/* assign this tag to the struct  */
    thistag->raw = swish_xstrdup(raw);

/*  the normalized tag */
    thistag->baked = swish_xstrdup(baked);

/* increment counter  */
    thistag->n = stack->count++;

/*  push */
    thistag->next = stack->head;
    stack->head = thistag;

/*  create context */
    thistag->context = flatten_tag_stack(NULL, stack);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("%s size: %d  thistag count: %d  current head tag = '%s'",
                        stack->name, stack->count, thistag->n, stack->head->context);

        _debug_stack(stack);

    }

}

static swish_Tag *
pop_tag_stack(
    swish_TagStack *stack
)
{
/*  stack is completely empty */
    if (stack->head == NULL)
        return NULL;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("%s POP: %s", stack->name, stack->head->raw);
        _debug_stack(stack);

    }

    if (stack->count > 1) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("%s %d: popping '%s'", stack->name, stack->head->n,
                            stack->head->raw);

        }

        stack->temp = stack->head;
        stack->head = stack->head->next;
        stack->count--;

    }
    else {

/*  the stack has only one member */

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("%s %d: popping '%s' will leave stack empty [%s]",
                            stack->name, stack->head->n, stack->head->raw,
                            stack->head->context);

        }

        stack->temp = stack->head;
        stack->head = NULL;
        stack->count--;

    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("%s stack count = %d", stack->name, stack->count);
    }

    return stack->temp;

}

/* 
* returns top of the stack if the current tag matches.
*/
static swish_Tag *
pop_tag_stack_on_match(
    swish_TagStack *stack,
    xmlChar *tag
)
{

    swish_Tag *st;

    st = NULL;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("%s: POP if %s matches %s", stack->name, tag, stack->head->raw);
        _debug_stack(stack);
    }

    if (xmlStrEqual(stack->head->raw, tag)) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("%s POP '%s' == head", stack->name, tag);

        }

/*
* more than default meta 
*/
        if ((st = pop_tag_stack(stack)) != NULL) {

            if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
                SWISH_DEBUG_MSG("%s POPPED.  tag = %s  st->raw = %s", stack->name, tag,
                                st->raw);

                _debug_stack(stack);
            }

        }

/*
* only tag on stack. TODO do we ever get here? 
*/
        else if (stack->count) {
            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("%s head %s", stack->name, stack->head->raw);

        }
        else {
            SWISH_CROAK("%s stack was empty", stack->name);
        }

    }
    else {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("%s: no match for '%s'", stack->name, tag);

    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        if (st != NULL)
            SWISH_DEBUG_MSG("POP on match returning: %s", st->raw);
        else
            SWISH_DEBUG_MSG("POP on match returning null");
    }

    return st;
}
