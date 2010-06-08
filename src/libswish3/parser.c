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

#ifndef LIBSWISH3_SINGLE_FILE

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <dirent.h>

#include <libxml/parserInternals.h>
#include <libxml/parser.h>
#include <libxml/HTMLparser.h>
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/xinclude.h>
#include <libxml/uri.h>

#include "libswish3.h"
#endif

extern int errno;
extern int SWISH_DEBUG;

/* should we pass on libxml2 via SWISH_WARN() */
/* default is "on" per consistency with version 2.4.x */
int SWISH_PARSER_WARNINGS = 1;

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
    xmlChar **atts,
    const xmlChar *xmlns_prefix
);
static void close_tag(
    void *data,
    const xmlChar *tag,
    const xmlChar *xmlns_prefix
);
static xmlChar *bake_tag(
    swish_ParserData *parser_data,
    xmlChar *tag,
    xmlChar **atts,
    xmlChar *xmlns_prefix
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
    swish_TagStack *stack,
    char flatten_join
);
static void add_stack_to_prop_buf(
    xmlChar *baked,
    swish_ParserData *parser_data
);
static void push_tag_stack(
    swish_TagStack *stack,
    xmlChar *raw,
    xmlChar *baked,
    char flatten_join
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
static void
free_swishTagStack(
    swish_TagStack *stack
);

static void
process_xinclude(
    swish_ParserData *parser_data,
    xmlChar *uri,
    boolean is_text
);

static void
xinclude_handler(
    swish_ParserData *parser_data
);

/***********************************************************************
*                end prototypes
***********************************************************************/

swish_Parser *
swish_parser_init(
    void (*handler) (swish_ParserData *)
)
{
    swish_Parser *p = (swish_Parser *)swish_xmalloc(sizeof(swish_Parser));

    p->handler = handler;
    p->verbosity = 0;
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
        SWISH_DEBUG_MSG("parser ptr 0x%x", (long int)p);
    }

    return p;
}

void
swish_parser_free(
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
bake_tag(
    swish_ParserData *parser_data,
    xmlChar *tag,
    xmlChar **atts,
    xmlChar *xmlns_prefix
)
{
    int i, j, is_html_tag, size;
    xmlChar *swishtag,
            *tmpstr,
            *xmlns,
            *attr_lower, 
            *attr_val_lower, 
            *alias, 
            *metaname, 
            *metacontent, 
            *metaname_from_attr;
    swish_StringList *strlist;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG(" tag: %s   parser->tag: %s ", tag, parser_data->tag);
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
    
    /* XML namespace support optional */
    if (xmlns_prefix != NULL && !parser_data->s3->config->flags->ignore_xmlns) {
        xmlns = swish_str_tolower(xmlns_prefix);
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("xmlns_prefix: '%s' (%s)", xmlns, xmlns_prefix);
        }
        size = xmlStrlen(swishtag) + xmlStrlen(xmlns) + 2;     /*  : + NUL */
        tmpstr = swish_xmalloc(size + 1);
        snprintf((char *)tmpstr, size, "%s%c%s", (char *)xmlns, 
            SWISH_XMLNS_CHAR, (char *)swishtag);
        swish_xfree(swishtag);
        swish_xfree(xmlns);
        swishtag = tmpstr;
    }

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
                SWISH_DEBUG_MSG("found html tag '%s' ... bump_word = %d", swishtag, SWISH_TRUE);
            parser_data->bump_word = SWISH_TRUE;
        }
        else {
            const htmlElemDesc *element = htmlTagLookup(swishtag);

            if (!element)
                is_html_tag = 0;        /* flag that this might be a meta * name */

            else if (!element->isinline) {

/*
* need to bump token position so we don't match across block *
* elements 
*/
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("found html !inline tag '%s' ... bump_word = %d", swishtag, SWISH_TRUE);
                parser_data->bump_word = SWISH_TRUE;

            }
            else {
            
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("found html inline tag '%s' ... bump_word = %d", swishtag, SWISH_FALSE);
                parser_data->bump_word = SWISH_FALSE;
            
            }
        }

/*
* is this an HTML <meta> tag? treat 'name' attribute as a tag *
* and 'content' attribute as the tag content * we assume 'name'
* and 'content' are always in english. 
*/

        if (atts != NULL) {
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
                    SWISH_DEBUG_MSG("found HTML meta tag '%s' ... bump_word = %d", metaname, SWISH_TRUE);

                parser_data->bump_word = SWISH_TRUE;
                open_tag(parser_data, metaname, NULL, xmlns_prefix);
                buffer_characters(parser_data, metacontent, xmlStrlen(metacontent));
                close_tag(parser_data, metaname, xmlns_prefix);
                
                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG("close_tag done. swishtag = '%s', parser->tag = '%s'", 
                        swishtag, parser_data->tag);
                        
                swish_xfree(parser_data->tag);  // metaname set recursively, so must free
                swish_xfree(swishtag);          // 'meta'
                
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

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("found xml tag '%s' ... bump_word = %d", swishtag, SWISH_TRUE);
            
        parser_data->bump_word = SWISH_TRUE;

/*
    XML attributes are parsed in 2 ways:
    (1) <foo class="bar">text</foo>
        becomes
        <foo.bar>text</foo>
    
    (2) <foo class="bar">text</foo>
        becomes
        <foo.class>bar</foo.class><foo>text</foo>
        
    the (2)-style is similar to HTML parser for meta name/content

*/

        if (atts != NULL) {
            strlist = NULL;
            if (swish_hash_exists(parser_data->s3->config->stringlists, 
                    (xmlChar *)SWISH_CLASS_ATTRIBUTES)
            ) {
                strlist = swish_hash_fetch(parser_data->s3->config->stringlists,
                                 (xmlChar *)SWISH_CLASS_ATTRIBUTES);
            }

            for (i = 0; (atts[i] != NULL); i += 2) {

                if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                    SWISH_DEBUG_MSG(" %d XML attr: %s=%s [%d]", i, atts[i], atts[i + 1],
                                    xmlStrlen(atts[i + 1]));

                attr_lower = swish_str_tolower(atts[i]);
                attr_val_lower = swish_str_tolower(atts[i + 1]);

/* is this attribute a metaname? */
                if (strlist != NULL) {
                    for (j = 0; j < strlist->n; j++) {
                        if (xmlStrEqual(strlist->word[j], attr_lower)) {
                            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                                SWISH_DEBUG_MSG("found %s: %s", attr_lower, attr_val_lower);
    
/* eligible attribute name, attribute value part of baked tag */
                            size = xmlStrlen(swishtag) + xmlStrlen(attr_val_lower) + 2;     /*  dot + NUL */
                            metaname = swish_xmalloc(size + 1);
                            snprintf((char *)metaname, size, "%s%c%s", (char *)swishtag, SWISH_DOT,
                                     (char *)attr_val_lower);
    
                            swish_xfree(swishtag);
                            swishtag = metaname;
                        }
                    }
                }
                
/* 
    explicit metaname with dotted notation. 
    attribute value considered document content, similar to how HTML parser works. 
*/
                size = xmlStrlen(swishtag) + xmlStrlen(attr_lower) + 2;     /*  dot + NUL */
                metaname_from_attr = swish_xmalloc(size + 1);
                snprintf((char *)metaname_from_attr, size, "%s%c%s", (char *)swishtag, SWISH_DOT, 
                    (char *)attr_lower);
                if (swish_hash_exists(parser_data->s3->config->metanames, metaname_from_attr)) {
                    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                        SWISH_DEBUG_MSG("found XML meta tag '%s' with content '%s'", 
                            metaname_from_attr, attr_val_lower);

                    parser_data->bump_word = SWISH_TRUE;
                    open_tag(parser_data, metaname_from_attr, NULL, xmlns_prefix);
                    buffer_characters(parser_data, attr_val_lower, xmlStrlen(attr_val_lower));
                    close_tag(parser_data, metaname_from_attr, xmlns_prefix);
                    swish_xfree(parser_data->tag);  // metaname set recursively, so must free
                
                    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                        SWISH_DEBUG_MSG("close_tag done. swishtag = '%s', parser->tag = '%s'", 
                            metaname_from_attr, parser_data->tag);
                
                }

                swish_xfree(metaname_from_attr);
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
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("%s alias -> %s", swishtag, alias); 
        }
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
    swish_MetaName *meta;
    xmlChar *metaname_stored_as;
    swish_TagStack *s = parser_data->metastack;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("buffer is >>%s<< before flush",
                        xmlBufferContent(parser_data->meta_buf));

/*
* add meta_buf as-is to metanames buffer under current tag. this
* gives us both tokens and raw text de-tagged but organized by
* metaname. If the metaname is an alias_for, use the target of the alias.
*/
    meta = swish_hash_fetch(parser_data->s3->config->metanames, metaname);
    if (meta->alias_for != NULL) {
        metaname_stored_as = meta->alias_for;
    }
    else {
        metaname_stored_as = metaname;
    }
    swish_nb_add_buf(parser_data->metanames, metaname_stored_as, parser_data->meta_buf,
                        (xmlChar *)SWISH_TOKENPOS_BUMPER, 0, 1);

/*
*  if cascade_meta_context is true, add tokens (buffer) to every metaname on the stack.
*/

    if (parser_data->s3->config->flags->cascade_meta_context) {
        for (s->temp = s->head; s->temp != NULL; s->temp = s->temp->next) {
            if (xmlStrEqual(s->temp->baked, metaname_stored_as))  /*  already added */
                continue;

            swish_nb_add_buf(parser_data->metanames, s->temp->baked,
                                parser_data->meta_buf, (xmlChar *)SWISH_TOKENPOS_BUMPER,
                                0, 1);
        }
    }

    if (parser_data->s3->analyzer->tokenize) {
        tokenize(parser_data, (xmlChar *)xmlBufferContent(parser_data->meta_buf),
                 xmlBufferLength(parser_data->meta_buf), metaname_stored_as, context);
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
    open_tag(data, name, (xmlChar **)atts, NULL);
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
    close_tag(data, name, NULL);
}

/* 
* SAX2 handler 
*/
static void
mystartElementNs(
    void *data,
    const xmlChar *localname,
    const xmlChar *xmlns_prefix,
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
    xmlChar *xinclude_uri;
    boolean xinclude_is_text;
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
        atts[j] = '\0';
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        //SWISH_DEBUG_MSG(" tag: %s nb_attributes %d", localname, nb_attributes);
        if (atts != NULL) {
            for (i = 0; (atts[i] != NULL); i += 2) {
                //SWISH_DEBUG_MSG(" att: %s=%s", atts[i], atts[i + 1]);
/* SWISH_DEBUG_MSG(" att: %s=", atts[i++], atts[i] || ""); */
            }
        }
    }
            
    /* check for XInclude */
    if ((xmlStrEqual(URI, XINCLUDE_OLD_NS) || xmlStrEqual(URI, XINCLUDE_NS))
        &&
        xmlStrEqual(localname, XINCLUDE_NODE)
        &&
        atts != NULL
    ) {
    
        /*
        SWISH_DEBUG_MSG("localname=%s  xmlns_prefix=%s  URI=%s", 
            localname, xmlns_prefix, URI);
        */
        xinclude_is_text = SWISH_FALSE;
        xinclude_uri = NULL;
        for (i = 0; (atts[i] != NULL); i += 2) {
            //SWISH_DEBUG_MSG(" att: %s=%s", atts[i], atts[i + 1]);
            if (xmlStrEqual(atts[i], XINCLUDE_HREF)) {
                //SWISH_DEBUG_MSG("XInclude: %s", atts[i + 1]);
                xinclude_uri = atts[i+1];
            }
            if (xmlStrEqual(atts[i], XINCLUDE_PARSE)) {
                xinclude_is_text = (boolean)xmlStrEqual(atts[i+1], XINCLUDE_PARSE_TEXT);
            }
        }
        if (xinclude_uri != NULL) {
            process_xinclude(
                (swish_ParserData*)data, 
                xinclude_uri, 
                xinclude_is_text
            );
        }
    }

    open_tag(data, localname, atts, xmlns_prefix);

    if (atts != NULL) {
        for (i = 0; (atts[i] != NULL); i += 2) {
            xmlFree(atts[i+1]); /* do not use swish_xfree since we did not malloc it */
        }
        swish_xfree(atts);
    }
}

static void
xinclude_handler(
    swish_ParserData *parser_data
)
{   
    swish_ParserData *parent;
    swish_Token *t;
    swish_TokenIterator *it;
    
    parent = (swish_ParserData*)parser_data->s3->stash;
    it = parser_data->token_iterator;
    while ((t = swish_token_iterator_next_token(it)) != NULL) {
        //swish_token_debug(t);
        swish_token_list_add_token(
            parent->token_iterator->tl,
            t->value,
            t->len + 1, // include the NUL
            t->meta,
            t->context
        );
    }
    parent->docinfo->nwords += parser_data->docinfo->nwords;
    
    swish_buffer_concat(parent->properties, parser_data->properties);
    swish_buffer_concat(parent->metanames, parser_data->metanames);
}

static void
process_xinclude(
    swish_ParserData *parser_data,
    xmlChar *uri,
    boolean is_text
)
{
    xmlChar *xuri;
    xmlChar *path;
    void *cur_stash;
    int res;
    swish_ParserData *child_data;
    boolean path_is_absolute;
    
    /* test if absolute path */
    if (uri[0] == SWISH_PATH_SEP) {
        xuri = uri;
        path = NULL;
        path_is_absolute = SWISH_TRUE;
    }
    else {
        path = swish_fs_get_path(parser_data->docinfo->uri);
        if (path == NULL) {
            SWISH_CROAK("Unable to resolve XInclude for %s relative to %s", 
                uri, parser_data->docinfo->uri);
        }
        xuri = xmlBuildURI(uri, path);
        if (xuri == NULL) {
            SWISH_CROAK("Unable to build XInclude URI for %s and %s", uri, path);
        }
        path_is_absolute = SWISH_FALSE;
    }
    
    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("xinclude  uri=%s  path=%s  xuri=%s", parser_data->docinfo->uri, path, xuri);
    }
        
    /*
     * set up our internal handler function,
     * which merges the 2 docs together. This isn't ideal, but since we 
     * are using SAX we can't leverage all the built-in XInclude support
     * that is part of libxml2.
     */
    cur_stash   = parser_data->s3->stash;
    parser_data->s3->stash = parser_data;
    flush_buffer(   parser_data, 
                    parser_data->metastack->head->baked,
                    parser_data->metastack->head->context
                );
    child_data = init_parser_data(parser_data->s3);
    child_data->docinfo = swish_docinfo_init();
    child_data->docinfo->ref_cnt++;

    if (!swish_docinfo_from_filesystem(xuri, child_data->docinfo, child_data)) {
        SWISH_WARN("Skipping XInclude %s", xuri);
    }
    else {
        if (is_text && !xmlStrEqual(child_data->docinfo->parser, BAD_CAST SWISH_PARSER_TXT)) {
            swish_xfree(child_data->docinfo->parser);
            child_data->docinfo->parser = swish_xstrdup( BAD_CAST SWISH_PARSER_TXT );
        }
        res = docparser(child_data, xuri, 0, 0);
        xinclude_handler(child_data);
    }
    
    /* clean up */
    free_parser_data(child_data);
    if (!path_is_absolute) {
        xmlFree(path);
        xmlFree(xuri);
    }
    
    /* restore stash */
    parser_data->s3->stash = cur_stash;
}

/* 
* SAX2 handler 
*/
static void
myendElementNs(
    void *data,
    const xmlChar *localname,
    const xmlChar *xmlns_prefix,
    const xmlChar *URI
)
{
    close_tag(data, localname, xmlns_prefix);
}

static void
open_tag(
    void *data,
    const xmlChar *tag,
    xmlChar **atts,
    const xmlChar *xmlns_prefix
)
{
    swish_ParserData *parser_data;
    xmlChar *baked;
    
    parser_data = (swish_ParserData *)data;
    
    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("<%s>", tag);

    if (parser_data->tag != NULL) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("Freeing swishtag (parser_data->tag): '%s'", parser_data->tag);
        }
        swish_xfree(parser_data->tag);
        parser_data->tag = NULL;
    }

    parser_data->tag = bake_tag(
                parser_data, 
                (xmlChar *)tag, 
                (xmlChar **)atts, 
                (xmlChar *)xmlns_prefix);
        
    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("checking config for '%s' in watched tags", parser_data->tag);

/* all tags on domstack */

    if (parser_data->tag == NULL) {
        push_tag_stack(parser_data->domstack, (xmlChar *)tag, (xmlChar *)tag, SWISH_DOT);
    }
    else {
        push_tag_stack(parser_data->domstack, (xmlChar *)tag, parser_data->tag, SWISH_DOT);
    }
    
/*
* set property if this tag is configured for it 
*/
    if (swish_hash_exists(parser_data->s3->config->properties, parser_data->tag)
        ||
        swish_hash_exists(parser_data->s3->config->properties, parser_data->domstack->head->context)
    ) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG(" %s = new property", parser_data->tag);

        add_stack_to_prop_buf(NULL, parser_data);       /* NULL means all properties in the stack are added */
        xmlBufferEmpty(parser_data->prop_buf);
        
        if (swish_hash_exists(parser_data->s3->config->properties, parser_data->domstack->head->context))
            baked = parser_data->domstack->head->context;
        else
            baked = parser_data->tag;

        push_tag_stack(parser_data->propstack, (xmlChar *)tag, baked, SWISH_DOM_CHAR);

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("%s pushed ok unto propstack", baked);
    }

/*
* likewise for metastack 
*/
    if (swish_hash_exists(parser_data->s3->config->metanames, parser_data->tag)
        ||
        swish_hash_exists(parser_data->s3->config->metanames, parser_data->domstack->head->context)
    ) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG(" %s = new metaname", parser_data->tag);

        flush_buffer(parser_data, parser_data->metastack->head->baked,
                     parser_data->metastack->head->context);
                     
        if (swish_hash_exists(parser_data->s3->config->properties, parser_data->domstack->head->context))
            baked = parser_data->domstack->head->context;
        else
            baked = parser_data->tag;

        push_tag_stack(parser_data->metastack, (xmlChar *)tag, baked, SWISH_DOM_CHAR);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("config check for '%s' done", parser_data->tag);

}

static void
close_tag(
    void *data,
    const xmlChar *tag,
    const xmlChar *xmlns_prefix
)
{
    swish_ParserData *parser_data;
    swish_Tag *st;

    parser_data = (swish_ParserData *)data;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("</%s>", tag);
        
/*
* lowercase all names for comparison against metanames (which are
* also * lowercased) 
*/
    if (parser_data->tag != NULL) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("freeing parser_data->tag '%s'", parser_data->tag);
        }
        swish_xfree(parser_data->tag);
    }
    
    parser_data->tag = bake_tag(parser_data, (xmlChar *)tag, NULL, (xmlChar *)xmlns_prefix);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG(" endElement(%s) (%s)", (xmlChar *)tag, parser_data->tag);
        
    if (parser_data->tag == NULL)
        return;

    if ((st = pop_tag_stack_on_match(parser_data->propstack, (xmlChar *)tag)) != NULL) {

        add_stack_to_prop_buf(st->baked, parser_data);
        xmlBufferEmpty(parser_data->prop_buf);
        free_swishTag(st);
    }

    if ((st = pop_tag_stack_on_match(parser_data->metastack, (xmlChar *)tag)) != NULL) {

        //SWISH_DEBUG_MSG("flush_buffer before free_swishTag");
        flush_buffer(parser_data, st->baked, st->context);
        //SWISH_DEBUG_MSG("metastack pop_tag_stack_on_match free_swishTag");
        free_swishTag(st);
    }
    
    // always pop the raw domstack
    st = pop_tag_stack(parser_data->domstack);
    free_swishTag(st);
    //SWISH_DEBUG_MSG("free_swishTag raw DOMstack done");

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

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("appending %d bytes to buffer (bump_word=%d)", 
            len, parser_data->bump_word);
    }

    for (i = 0; i < len; i++) {
        output[i] = ch[i];
    }
    output[i] = '\0';

    if (parser_data->bump_word && xmlBufferLength(buf)) {
        //SWISH_DEBUG_MSG("bump_word is true; appending TOKENPOS_BUMPER to buffer");
        swish_buffer_append(buf, (xmlChar *)SWISH_TOKENPOS_BUMPER, 1);
    }
    
    swish_buffer_append(buf, output, len);

    if (parser_data->bump_word && xmlBufferLength(parser_data->prop_buf)) {
        //SWISH_DEBUG_MSG("bump_word is true; appending TOKENPOS_BUMPER to buffer");
        swish_buffer_append(parser_data->prop_buf, (xmlChar *)SWISH_TOKENPOS_BUMPER, 1);
    }
    else if (xmlBufferLength(parser_data->prop_buf)) {
        swish_buffer_append(parser_data->prop_buf, (xmlChar*)" ", 1);
    }

    swish_buffer_append(parser_data->prop_buf, output, len);
    
    // reset
    parser_data->bump_word = SWISH_FALSE;
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
    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        int i;
        for (i=0; i<len; i++) {
            SWISH_DEBUG_MSG("%c [%d]", ch[i], i);
        }
    }
    
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
    /* passing args as last param is ignored but quiets a gcc warning */
    xmlParserError(parser_data->ctxt, (char *)str, args);
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
    if (parser_data->ctxt == NULL) {
        SWISH_WARN("ctxt is null");
    }

    va_start(args, msg);
    vsnprintf((char *)str, 1000, (char *)msg, args);
    /* passing args as last param is ignored but quiets a gcc warning */
    xmlParserWarning(parser_data->ctxt, (char *)str, args);
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
    (errorSAXFunc) & mywarn,     /* xmlParserError */
    (fatalErrorSAXFunc) & myerr, /* xmlfatalParserError */
    NULL,                       /* getParameterEntity */
    NULL,                       /* cdataBlock */
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
    ret = 0;
    xmlChar *mime = (xmlChar *)parser_data->docinfo->mime;
    xmlChar *parser = (xmlChar *)parser_data->docinfo->parser;

    if (!size && !xmlStrlen(buffer) && !parser_data->docinfo->size) {
        SWISH_WARN("%s appears to be empty -- can't parse it", parser_data->docinfo->uri);

        return 1;
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("%s -- using %s parser [%c]", parser_data->docinfo->uri, parser, parser[0]);

/*
* slurp file if not already in memory 
*/
    if (filename && !buffer) {
        if (parser_data->docinfo->is_gzipped) {
            buffer = swish_io_slurp_gzfile_len(
                filename, 
                &(parser_data->docinfo->size), 
                SWISH_FALSE
            );
            parser_data->docinfo->size = xmlStrlen(buffer);
        }
        else {
            buffer = swish_io_slurp_file_len(
                filename, 
                (off_t)parser_data->docinfo->size,
                SWISH_FALSE
            );
        }
        size = parser_data->docinfo->size;
    }

    if (parser[0] == 'H' || parser[0] == 'h') {
        parser_data->is_html = SWISH_TRUE;
        ret = html_parser(my_parser_ptr, parser_data, buffer, size);
    }
    else if (parser[0] == 'X' || parser[0] == 'x') {
        ret = xml_parser(my_parser_ptr, parser_data, buffer, size);
    }
    else if (parser[0] == 'T' || parser[0] == 't') {
        ret = txt_parser(parser_data, (xmlChar *)buffer, size);
    }
    else {
        SWISH_CROAK("no parser known for MIME '%s'", mime);
    }
    
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
    ptr->token_iterator = swish_token_iterator_init(s3->analyzer);
    ptr->token_iterator->ref_cnt++;
    ptr->properties = swish_nb_init(s3->config->properties);
    ptr->properties->ref_cnt++;
    ptr->metanames = swish_nb_init(s3->config->metanames);
    ptr->metanames->ref_cnt++;

/*
*   set tokenizer if one has not been explicitly set
*/
    if (s3->analyzer->tokenizer == NULL) {
        s3->analyzer->tokenizer = (&swish_tokenize);
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
                   (xmlChar *)SWISH_DEFAULT_METANAME, SWISH_DOM_CHAR);

    ptr->propstack = (swish_TagStack *)swish_xmalloc(sizeof(swish_TagStack));
    ptr->propstack->name = "PropStack";
    ptr->propstack->head = NULL;
    ptr->propstack->temp = NULL;
    ptr->propstack->count = 0;
    push_tag_stack(ptr->propstack, (xmlChar *)SWISH_DOM_STR, (xmlChar *)SWISH_DOM_STR, SWISH_DOM_CHAR);
    
    ptr->domstack  = (swish_TagStack *)swish_xmalloc(sizeof(swish_TagStack));
    ptr->domstack->name  = "DOMStack";
    ptr->domstack->head  = NULL;
    ptr->domstack->temp  = NULL;
    ptr->domstack->count = 0;

/*
* gets toggled per-tag 
*/
    ptr->bump_word = SWISH_TRUE;

/*
* toggle 
*/
    ptr->no_index = SWISH_FALSE;

/*
* shortcut rather than looking parser up in hash for each tag event 
*/
    ptr->is_html = SWISH_FALSE;
        
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

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        SWISH_DEBUG_MSG("init done for parser_data");
    }
    
    return ptr;

}

static void
free_swishTagStack(
    swish_TagStack *stack
)
{
    swish_Tag *st;
    
    while ((st = pop_tag_stack(stack)) != NULL) {
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("%s %d POP %s [%s] [%s]", stack->name,
                            stack->count, st->raw, st->baked, st->context);

        free_swishTag(st);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing stack %s", stack->name);

    swish_xfree(stack);
}

static void
free_parser_data(
    swish_ParserData *ptr
)
{

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData");

/*
* dec ref count for shared ptr 
*/
    ptr->s3->ref_cnt--;

/*
* Pop the stacks 
*/
    free_swishTagStack(ptr->metastack);
    free_swishTagStack(ptr->propstack);
    free_swishTagStack(ptr->domstack);
    
/* free named buffers */

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData properties");

    ptr->properties->ref_cnt--;
    swish_nb_free(ptr->properties);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("freeing swish_ParserData metanames");

    ptr->metanames->ref_cnt--;
    swish_nb_free(ptr->metanames);

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

    if (ptr->token_iterator != NULL) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("free swish_ParserData TokenIterator");

        ptr->token_iterator->ref_cnt--;
        swish_token_iterator_free(ptr->token_iterator);
    }

    if (ptr->docinfo != NULL) {

        if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
            SWISH_DEBUG_MSG("free swish_ParserData docinfo");

        ptr->docinfo->ref_cnt--;
        swish_docinfo_free(ptr->docinfo);

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
    const xmlChar *newlines;
    HEAD *h;

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("parsing head from buffer: %s", buf);

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

            if (buf[k] == '\n' || buf[k] == '\0') {

                if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
                    SWISH_DEBUG_MSG("found blank header line at byte %d\n", k);
                }
                
                h->body_start = k + 1;
                break;
            }
            i = 0;

            continue;
        }
    }
    
    swish_xfree(line);

    /* sanity check */
    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        SWISH_DEBUG_MSG("finished parsing head from buffer");
        newlines = xmlStrstr((const xmlChar*)buf, (const xmlChar*)"\n\n");
        if (newlines != NULL) {
            SWISH_DEBUG_MSG("strstr found body start at %d; loop at %d",
            (int)(buf - newlines), h->body_start);
        }
    }


    return h;
}

static swish_DocInfo *
head_to_docinfo(
    HEAD * h
)
{
    int i;
    xmlChar *val, *line;

    swish_DocInfo *info = swish_docinfo_init();

    info->ref_cnt++;

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO)
        SWISH_DEBUG_MSG("preparing to parse %d header lines", h->nlines);

    for (i = 0; i < h->nlines; i++) {

        line = h->lines[i];

        if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO)
            SWISH_DEBUG_MSG("parsing header line: >%s<", line);

        val = (xmlChar *)xmlStrchr(line, ':');
        if(!val) {
            SWISH_CROAK("bad header line: %s", line);
        }
        val = swish_str_skip_ws(++val);

        if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
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

            SWISH_WARN("%s: Last-Mtime is deprecated in favor of Last-Modified", val);

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

            SWISH_WARN("%s: Path-Name is deprecated in favor of Content-Location", val);

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

            SWISH_WARN("%s: Document-Type is deprecated in favor of Parser-Type", val);

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
        if (!xmlStrncasecmp(line, (const xmlChar *)"Action", 11)) {

            if (!val)
                SWISH_WARN("Failed to parse Action header '%s'", line);

            if (!*val)
                SWISH_WARN("Failed to find value in Action header '%s'", line);

            if (info->action != NULL)
                swish_xfree(info->action);

            info->action = swish_xstrdup(val);
            continue;
        }

/*
* if we get here, unrecognized header line 
*/
        SWISH_WARN("Unknown header line: '%s'\n", line);

    }

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        SWISH_DEBUG_MSG("returning %d header lines", h->nlines);
        swish_docinfo_debug(info);
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

    setenv("SWISH_PARSER_WARNINGS", "1", 0);
    SWISH_PARSER_WARNINGS = swish_string_to_int(getenv("SWISH_PARSER_WARNINGS"));

    if (SWISH_DEBUG) {
        SWISH_PARSER_WARNINGS = SWISH_DEBUG;
    }
    
}

unsigned int
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
    unsigned int file_cnt;

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
    while (fgets((char *)ln, SWISH_MAXSTRLEN, fh) != NULL) {

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
            swish_docinfo_check(parser_data->docinfo, s3->config);

            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("reading %ld bytes from filehandle",
                                (long int)parser_data->docinfo->size);

            read_buffer = swish_io_slurp_fh(fh, parser_data->docinfo->size, SWISH_FALSE);

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
                swish_docinfo_debug(parser_data->docinfo);
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
                etime = swish_time_print_fine(swish_time_elapsed() - curTime);
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

            if ((xmlBufferCCat(head_buf, "\n")) != 0)
                SWISH_CROAK("can't add newline to end of header buffer");

            nheaders++;
            
            if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
                SWISH_DEBUG_MSG("nheaders = %d for buffer >%s<", 
                    nheaders, xmlBufferContent(head_buf));
            }
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
    swish_docinfo_check(parser_data->docinfo, s3->config);

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
        swish_docinfo_debug(parser_data->docinfo);
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
        etime = swish_time_print_fine(swish_time_elapsed() - curTime);
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

    parser_data->docinfo = swish_docinfo_init();
    parser_data->docinfo->ref_cnt++;

    if (!swish_docinfo_from_filesystem(filename, parser_data->docinfo, parser_data)) {
        SWISH_WARN("Skipping %s", filename);
        free_parser_data(parser_data);
        return SWISH_ERR_NO_SUCH_FILE;
    }

    res = docparser(parser_data, filename, 0, 0);

/*
* pass to callback function 
*/
    (*s3->parser->handler) (parser_data);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        swish_docinfo_debug(parser_data->docinfo);
        SWISH_DEBUG_MSG("  word buffer length: %d bytes",
                        xmlBufferLength(parser_data->meta_buf));
        SWISH_DEBUG_MSG(" (%d words)", parser_data->docinfo->nwords);
    }

/*
* free buffers 
*/
    free_parser_data(parser_data);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
        etime = swish_time_print_fine(swish_time_elapsed() - curTime);
        SWISH_DEBUG_MSG("%s elapsed time", etime);
        swish_xfree(etime);
    }

    return res;

}

/*
 * based on swish-e 2.4 indexadir() in fs.c
 */
unsigned int
swish_parse_directory(
    swish_3 *s3,
    xmlChar *dir,
    boolean follow_symlinks
)
{
    DIR *dir_handle;
#ifdef NEXTSTEP
    struct direct   *dir_ptr;
#else
    struct dirent   *dir_ptr;
#endif
    xmlChar *pathbuf;
    unsigned int pathbuflen;
    unsigned int dir_len;
    unsigned int files_parsed;
    
    files_parsed = 0;
    
    if ((dir_handle = opendir((char*)dir)) == NULL) {
        SWISH_WARN("Failed to open directory '%s' : %s", dir, strerror(errno));
        return files_parsed;
    }

    pathbuflen = SWISH_MAXSTRLEN;
    pathbuf = (xmlChar*)swish_xmalloc(pathbuflen + 1);
    dir_len = xmlStrlen(dir);
    
    /* case of root dir */
    if ( dir_len == 1 && dir[0] == SWISH_PATH_SEP ) 
        dir_len = 0;
        
    while ((dir_ptr = readdir(dir_handle)) != NULL) {
        int file_len = strlen( dir_ptr->d_name );

        /* For security reasons, don't index dot files */
        /* TODO Check for hidden under Windows? */
        if ((dir_ptr->d_name)[0] == '.')
            continue;


        /* Build full path to file */

        /* reallocate filename buffer, if needed (dir + path + SWISH_PATH_SEP ) */
        if ( (dir_len + file_len + 1) > pathbuflen ) {
            pathbuflen = dir_len + file_len + 256;
            pathbuf = (xmlChar *)swish_xrealloc(pathbuf, pathbuflen + 1);
        }

        if ( dir_len )
            memcpy(pathbuf, dir, dir_len);

        pathbuf[dir_len] = SWISH_PATH_SEP;  // Add path separator
        memcpy(pathbuf + dir_len + 1, dir_ptr->d_name, file_len);
        pathbuf[dir_len + file_len + 1] = '\0';

        /* Check if the path is a symlink */
        if ( !follow_symlinks && swish_fs_is_link( pathbuf ) )
            continue;


        if ( swish_fs_is_dir(pathbuf) ) {
            /* recurse immediately. this is a depth-first algorithm */
            if (s3->parser->verbosity) {
                printf("Found directory: %s\n", pathbuf);
            }
            files_parsed += swish_parse_directory(s3, pathbuf, follow_symlinks);
        }
        else if (swish_fs_is_link(pathbuf) && follow_symlinks) {
            if (s3->parser->verbosity) {
                printf("Found symlink: %s\n", pathbuf);
            }
            // TODO?
        
        }
        else if (swish_fs_is_file(pathbuf)) {
            if (s3->parser->verbosity) {
                printf("Found file: %s\n", pathbuf);
            }
            if (!swish_parse_file(s3, pathbuf)) {
                files_parsed++;
            }
        }
        else {
            SWISH_CROAK("Unknown file in directory: %s", pathbuf);
        }
    }
    closedir(dir_handle);
    swish_xfree(pathbuf);

    return files_parsed;
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
* xmlErrMemory is/was not a public func but is in parserInternals.h.
* basically, this is a bad, fatal error, so we'll just die.
*/

/*
* xmlErrMemory(ctxt, NULL); 
*/
        SWISH_CROAK("Fatal libxml2 memory error");
    }

    if (user_data != NULL)
        ctxt->userData = user_data;

/* track ctxt in parser_data during the actual parsing
   so that warnings/errors show context. But set to NULL
   afterwards so we don't try and free it when parser_data
   gets freed.
*/
    parser_data->ctxt = ctxt;
    if (xmlParseDocument(ctxt) < 0) {
        SWISH_WARN("recovering from libxml2 error for %s", parser_data->docinfo->uri);
    }
    parser_data->ctxt = NULL;

    if (ctxt->wellFormed) {
        ret = 0;
    }
    else {
        if (ctxt->errNo != 0) {
            ret = ctxt->errNo;
        }
        else {
            ret = -1;
        }
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
    int ret;
    htmlParserCtxtPtr ctxt;
    xmlChar *default_encoding;
    htmlSAXHandlerPtr oldsax = 0;
    swish_ParserData *parser_data = (swish_ParserData *)user_data;
    default_encoding = (xmlChar *)getenv("SWISH_ENCODING");

    xmlInitParser();

    ctxt = htmlCreateMemoryParserCtxt((const char *)buffer, xmlStrlen(buffer));
    
    parser_data->ctxt = ctxt;

    if (parser_data->docinfo->encoding != NULL) {
        swish_xfree(parser_data->docinfo->encoding);
    }
        
    parser_data->docinfo->encoding = document_encoding(ctxt);
        
    if (parser_data->docinfo->encoding == NULL) {
        set_encoding(parser_data, (xmlChar *)buffer);
    }
    
    /*
     * HTML parser defaults to ISO-8859-1 and that's what Swish-e 2.x does.
     * Leave that default alone, since we assume any HTML docs that actually
     * care about encoding will explicitly specify it in the <meta> header,
     * which libxml2 should respect.
    if (ctxt->encoding == NULL) {
        xmlCharEncoding enc = xmlParseCharEncoding((char*)parser_data->docinfo->encoding);
        xmlSwitchEncoding(ctxt, enc);
    }
    */

    if (ctxt == 0) {
        return (0);
    }
    
    if (sax != 0) {
        oldsax = ctxt->sax;
        ctxt->sax = (htmlSAXHandlerPtr) sax;
        ctxt->userData = parser_data;
    }
    
    ret = htmlParseDocument(ctxt);

    if (sax != 0) {
        ctxt->sax = oldsax;
        ctxt->userData = 0;
    }

    return ret;
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
    outlen = 0;

/*
* TODO better encoding detection. for now we assume unknown text
* files are latin1 
*/
    set_encoding(parser_data, buffer);

    if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
        SWISH_DEBUG_MSG("txt parser encoding: %s", parser_data->docinfo->encoding);

    if (!xmlStrEqual(parser_data->docinfo->encoding, (xmlChar *)SWISH_DEFAULT_ENCODING)) {
        SWISH_WARN("%s docinfo->encoding %s != %s", 
            parser_data->docinfo->uri, parser_data->docinfo->encoding, SWISH_DEFAULT_ENCODING);

        if (!xmlStrncasecmp(parser_data->docinfo->encoding, (xmlChar *)SWISH_LATIN1_ENCODING, 9)) {
            outlen = size * 2;
            out = swish_xmalloc(outlen);

            if (!isolat1ToUTF8(out, &outlen, buffer, &size)) {
                SWISH_WARN("could not convert buf from %s (outlen: %d)", SWISH_LATIN1_ENCODING, outlen);
            }
            else {
                SWISH_WARN("converted %s from %s to %s", 
                    parser_data->docinfo->uri, SWISH_LATIN1_ENCODING, SWISH_DEFAULT_ENCODING);
            }
            
            size = outlen;
            buffer = out;
        }

        else if (xmlStrEqual(parser_data->docinfo->encoding, enc)) {
            if (SWISH_DEBUG & SWISH_DEBUG_PARSER)
                SWISH_DEBUG_MSG("default env encoding -> %s", enc);

            if (xmlStrncasecmp(enc, (xmlChar *)SWISH_LATIN1_ENCODING, 9)) {
                SWISH_WARN
                    ("%s encoding is unknown (not %s) but LC_CTYPE is %s -- assuming file is %s",
                     parser_data->docinfo->uri, SWISH_DEFAULT_ENCODING, enc, SWISH_LATIN1_ENCODING);

            }

            outlen = size * 2;
            out = swish_xmalloc(outlen);

            if (!isolat1ToUTF8(out, &outlen, buffer, &size)) {
                SWISH_WARN("could not convert buf from %s (outlen: %d): %s", 
                    SWISH_LATIN1_ENCODING, outlen, buffer);
                swish_xfree(out);
                return SWISH_ENCODING_ERROR;
            }
            else {
                SWISH_WARN("converted %s from %s to %s", 
                    parser_data->docinfo->uri, SWISH_LATIN1_ENCODING, SWISH_DEFAULT_ENCODING);
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
                   (xmlChar *)SWISH_DEFAULT_METANAME, SWISH_DOM_CHAR);

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

    if (xmlCheckUTF8((const xmlChar *)buffer) != 0) {
        parser_data->docinfo->encoding = swish_xstrdup((xmlChar *)SWISH_DEFAULT_ENCODING);
    }
    else {
        parser_data->docinfo->encoding = swish_xstrdup((xmlChar *)getenv("SWISH_ENCODING"));
    }
}

static xmlChar *
document_encoding(
    xmlParserCtxtPtr ctxt
)
{
    xmlChar *enc;

    if (ctxt->encoding != NULL) {
        enc = swish_xstrdup(ctxt->encoding);
        //SWISH_DEBUG_MSG("ctxt->encoding == %s", enc);
    }
    else if (ctxt->inputTab[0]->encoding != NULL) {
        enc = swish_xstrdup(ctxt->inputTab[0]->encoding);
        //SWISH_DEBUG_MSG("ctxt->inputTab->encoding == %s", enc);
    }
    else {

/*
* if we get here, we didn't error with bad encoding via SAX,
* so assume the current locale encoding.
*/
        enc = swish_xstrdup((xmlChar *)getenv("SWISH_ENCODING"));
        //SWISH_DEBUG_MSG("using default SWISH_ENCODING == %s", enc);
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

    parser_data->docinfo->nwords +=
            (*parser_data->s3->analyzer->tokenizer) (parser_data->token_iterator, 
                                                    string, meta, context);
    return;

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
    swish_TagStack *stack,
    char flatten_join
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
            (
            (xmlStrlen(flat)*sizeof(xmlChar)) + 
            (xmlStrlen(stack->temp->baked)*sizeof(xmlChar)) +
            3   // flatten_join + nulls
            );
            
        tmp = swish_xmalloc(size);
                
        if (snprintf((char *)tmp, size, "%s%c%s",
             (char *)stack->temp->baked, flatten_join, (char *)flat)
            > 0
        ) {
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
    boolean cleanwsp;
    swish_Property *prop;
    xmlChar *prop_to_store;

    stack = parser_data->propstack;
    cleanwsp = 1;
    
    if (baked != NULL) {
        /* If the propertyname is an alias_for, use the target of the alias. */
        prop = swish_hash_fetch(parser_data->s3->config->properties, baked);
        if (prop->alias_for != NULL) {
            prop_to_store = prop->alias_for;
        }
        else {
            prop_to_store = baked;
        }
    
        /* override per-property */
        if (prop->verbatim) {
            cleanwsp = 0;
        }
    
        if (SWISH_DEBUG & SWISH_DEBUG_PARSER) {
            SWISH_DEBUG_MSG("adding property %s to buffer", prop_to_store);
        }

        swish_nb_add_buf(parser_data->properties, prop_to_store, parser_data->prop_buf,
                            (xmlChar *)SWISH_TOKENPOS_BUMPER, cleanwsp, 0);
    }

    /* Swish-e 2.x behavior is to add for each member in the stack */
    for (stack->temp = stack->head; stack->temp != NULL; stack->temp = stack->temp->next) {
        if (xmlStrEqual(stack->temp->baked, (xmlChar *)SWISH_DOM_STR))
            continue;

        swish_nb_add_buf(parser_data->properties, stack->temp->baked,
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
        SWISH_DEBUG_MSG(" freeing swishTag: (raw)%s (baked)%s (context)%s", 
            st->raw, st->baked, st->context);
    }

    //SWISH_DEBUG_MSG("free raw: %s", st->raw);
    swish_xfree(st->raw);
    //SWISH_DEBUG_MSG("free baked: %s", st->baked);
    swish_xfree(st->baked);
    //SWISH_DEBUG_MSG("free context: %s", st->context);
    swish_xfree(st->context);
    //SWISH_DEBUG_MSG("free swishTag");
    swish_xfree(st);
    //SWISH_DEBUG_MSG("free swishTag done");
}

static void
push_tag_stack(
    swish_TagStack *stack,
    xmlChar *raw,
    xmlChar *baked,
    char flatten_join
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
    thistag->context = flatten_tag_stack(NULL, stack, flatten_join);

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
