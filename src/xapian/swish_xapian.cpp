/*
 * This file is part of libswish3
 * Copyright (C) 2008 Peter Karman
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
    example Swish3 program using Xapian IR backend.
    many of the string conversion functions and the index_document() code
    come nearly verbatim from the xapian-omega distribution.

*/

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include <time.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>
#include <getopt.h>
#include <netinet/in.h>

#include <xapian.h>

#include <libxml/hash.h>

#include "acconfig.h"
#include "libswish3.h"

#define SWISH_MAX_OUTPUT_PROPS 64
#define SWISH_PROP_OUTPUT_PLACEHOLDER '\3'
#define SWISH_SPLIT_PROPERTIES 0
#define SWISH_FACET_FINDER_LIMIT 1000000
#define SWISH_MAX_FACETS 128

using namespace std;

/* prototypes */
int main(
    int argc,
    char **argv
);
void libxml2_version();
void swish_version();
void xapian_version();
int usage(
);
void handler(
    swish_ParserData *parser_data
);
int open_writeable_index(
    char *dbpath
);
int open_readable_index(
    char *dbpath,
    xmlChar *stemmer_lang
);
int search(
    char *query,
    xmlChar *output_format,
    xmlChar *sort_string,
    unsigned int results_offset,
    unsigned int results_limit  
);
static boolean 
delete_document(
    xmlChar *uri
);

char*
make_header_path(
    char *dbpath
);

static void
get_db_data(
    char *dbpath,
    xmlChar *db_data
);

/* global vars */
static int debug = 0;
static int verbose = 0;

static
    Xapian::WritableDatabase
    wdb;
static
    Xapian::Database::Database
    rdb;
static
    Xapian::Stem
stemmer(
    "none"
);
static
    Xapian::TermGenerator
    indexer;
static long int
    twords = 0;
static int
    skip_duplicates = 0;
static int
    overwrite = 0;
static map<string, int>
    property_name_id_map;
static map<string, unsigned long> 
    facets[SWISH_MAX_FACETS];
static unsigned int
    num_facets;
static swish_StringList 
    *facet_list;
static
    vector <
    bool >
    updated;
static swish_3 *
    s3;

extern int
    SWISH_DEBUG;

static struct option
    longopts[] = {
    {"config",      required_argument, 0,   'c'},
    {"verbose",     no_argument, 0,         'v'},
    {"debug",       required_argument, 0,   'd'},
    {"help",        no_argument, 0,         'h'},
    {"index",       required_argument, 0,   'i'},
    {"Skip-duplicates", no_argument, 0,     'S'},
    {"sort",        required_argument, 0,   's'},
    {"stemmer",     required_argument, 0,   't'},
    {"db_data",     required_argument, 0,   'T'},
    {"overwrite",   no_argument, 0,         'o'},
    {"query",       required_argument, 0,   'q'},
    {"filelist",    required_argument, 0,   'f'},
    {"Facets",      required_argument, 0,   'F'},
    {"Delete",      no_argument, 0,         'D'},
    {"output",      required_argument, 0,   'x'},
    {"begin",       required_argument, 0,   'b'},
    {"max",         required_argument, 0,   'm'},
    {"follow-symlinks", no_argument,   0,   'l'},
    {"limit",       required_argument, 0,   'L'},
    {0, 0, 0, 0}
};

int
usage(
)
{
    const char *
        descr = "swish_xapian is an example program for using libswish3 with Xapian\n";
    printf("usage: swish_xapian [opts] [- | file(s)]\n");
    printf(" opts:\n");
    printf("  -b, --begin=NUM           begin results at NUM\n");
    printf("  -c, --config=FILE         name a config file for indexing\n");
    printf("  -d, --debug[=NUM]         set debug level (see also env vars in swish_lint)\n");
    printf("  -D, --Delete              with --filelist, remove files from index\n");
    printf("  -F, --Facets=STRING       list result property counts according to STRING \"prop1 prop2\"\n");
    printf("  -f, --filelist=FILE       index filenames in FILE (one per line)\n");
    printf("  -h, --help                print this usage statement\n");
    printf("  -i, --index=PATH          name a directory for the index files\n");
    printf("  -l, --follow-symlinks     follow symbolic links when indexing\n");
    printf("  -L, --limit=STRING        limit results to a range of property values \"prop low high\"\n");
    printf("  -m, --max=NUM             maximum number of results to return (defaults to 100)\n");
    printf("  -o, --overwrite           overwrite existing index (fresh start)\n");
    printf("  -q, --query=STRING        search for STRING in index\n");
    printf("  -S, --Skip-duplicates     ignore duplicate filenames (do not update them in index)\n");
    printf("  -s, --sort=STRING         sort results according to STRING \"prop1 prop2 ...\"\n");
    printf("  -t, --stemmer=LANG        apply term stemming in language LANG (default is 'none')\n");
    printf("  -x, --output=STRING       format search results with STRING\n");
    printf("\n%s\n", descr);
    libxml2_version();
    swish_version();
    xapian_version();
    exit(0);
}


// This ought to be enough for any of the conversions below.
#define BUFSIZE 100

#ifdef SNPRINTF
#define CONVERT_TO_STRING(FMT) \
    char buf[BUFSIZE];\
    int len = SNPRINTF(buf, BUFSIZE, (FMT), val);\
    if (len == -1 || len > BUFSIZE) return string(buf, BUFSIZE);\
    return string(buf, len);
#else
#define CONVERT_TO_STRING(FMT) \
    char buf[BUFSIZE];\
    buf[BUFSIZE - 1] = '\0';\
    sprintf(buf, (FMT), val);\
    if (buf[BUFSIZE - 1]) abort(); /* Uh-oh, buffer overrun */ \
    return string(buf);
#endif


class FacetFinder : public Xapian::MatchDecider {
  public:
    bool operator()(const Xapian::Document &doc) const {
        //SWISH_DEBUG_MSG("examining doc %s", doc.get_value(1).c_str());
	    int i;
        
        for (i=0; i < num_facets; i++) {
            int prop_id = property_name_id_map[string((const char*)facet_list->word[i])];
            //SWISH_DEBUG_MSG("prop_id [%d]", prop_id);
            string value(doc.get_value(prop_id));
            const xmlChar *buf = (const xmlChar *)value.c_str();
            const xmlChar *substr;
            while ((substr = xmlStrstr(buf, (const xmlChar *)SWISH_TOKENPOS_BUMPER)) != NULL) {
                int sub_len = substr - buf;
                ++facets[i][string((const char *)xmlStrsub(buf, 0, sub_len))];
                buf = substr + 1; /* move ahead */
            }
            // last one
            if (buf != NULL) {
                ++facets[i][string((const char *)buf)];
            }
            //SWISH_DEBUG_MSG("facet '%s' => %ld (0x%lx)", 
                  //value.c_str(), facets[i][value], &facets[i]);
        }
	    return true;
    };
    
};

static void
facet_finder_init()
{
    int i;
    for (i=0; i<num_facets; i++) {
        map<string,unsigned long> facet_counter;
        facets[i] = facet_counter;
        property_name_id_map[string((const char*)facet_list->word[i])] = 
            swish_property_get_id(facet_list->word[i], s3->config->properties);
    }
}

static void
facet_finder_show()
{
    int i;
    for (i=0; i<num_facets; i++) {
        printf("Facet %s (%d)\n", facet_list->word[i], i);
        map<string,unsigned long>::const_iterator it;
        for (it=facets[i].begin(); it != facets[i].end(); ++it) {
            printf(" :%s: %ld\n", it->first.c_str(), it->second);
        }
    }
}

int
string_to_int(
    const string & s
)
{
    return swish_string_to_int((char*)s.c_str());
}

string
int_to_string(
    int val
)
{
    CONVERT_TO_STRING("%d")
}

string
long_to_string(
    long val
)
{
    CONVERT_TO_STRING("%ld")
}

string
double_to_string(
    double val
)
{
    CONVERT_TO_STRING("%f")
}

string
date_to_string(
    int y,
    int m,
    int d
)
{
    char
        buf[11];
    if (y < 0)
        y = 0;
    else if (y > 9999)
        y = 9999;
    if (m < 1)
        m = 1;
    else if (m > 12)
        m = 12;
    if (d < 1)
        d = 1;
    else if (d > 31)
        d = 31;
#ifdef SNPRINTF
    int
        len = SNPRINTF(buf, sizeof(buf), "%04d%02d%02d", y, m, d);
    if (len == -1 || len > BUFSIZE)
        return string(buf, BUFSIZE);
    return string(buf, len);
#else
    buf[sizeof(buf) - 1] = '\0';
    sprintf(buf, "%04d%02d%02d", y, m, d);
    if (buf[sizeof(buf) - 1])
        abort();                /* Uh-oh, buffer overrun */
    return string(buf);
#endif
}

inline
    uint32_t
binary_string_to_int(
    const std::string & s
)
{
    if (s.size() != 4)
        return (uint32_t) - 1;
    uint32_t v;
    memcpy(&v, s.data(), 4);
    return ntohl(v);
}

inline std::string
int_to_binary_string(
    uint32_t v
)
{
    v = htonl(v);
    return std::string(reinterpret_cast < const char *>(&v), 4);
}

static
    string
get_prefix(
    xmlChar *metaname,
    swish_Config *config
)
{
    string
        prefix;
    swish_MetaName *
        meta = (swish_MetaName *)swish_hash_fetch(config->metanames, metaname);
    prefix = int_to_string(meta->id);
    return prefix + string((const char *)":");
}

static void
add_prefix(
    swish_MetaName *meta,
    Xapian::QueryParser qp,
    xmlChar *name
)
{
    swish_MetaName *meta_to_use;
    // the meta id is either on *meta or, if an alias_for, the target *meta
    if (meta->alias_for) {
        meta_to_use = (swish_MetaName *)swish_hash_fetch(s3->config->metanames, meta->alias_for);
    }
    else {
        meta_to_use = meta;
    }
    qp.add_prefix(string((const char *)name),
                  int_to_string(meta_to_use->id) + string((const char *)":"));
}

static unsigned int
get_weight(
    xmlChar *metaname,
    swish_Config *config
)
{
    unsigned int
        w;
    swish_MetaName *
        meta = (swish_MetaName *)swish_hash_fetch(config->metanames, metaname);
    return meta->bias > 0 ? meta->bias : 1;     // TODO need to account for negative values.
}

static void
add_metanames(
    xmlBufferPtr buffer,
    void *config,
    xmlChar *metaname
)
{
    // lookup weight and prefix
    string
        prefix = get_prefix(metaname, (swish_Config *)config);
    unsigned int
        weight = get_weight(metaname, (swish_Config *)config);
    indexer.index_text((const char *)xmlBufferContent(buffer), weight, prefix);
    
    // index swishdefault and swishtitle without any prefix too
    if (xmlStrEqual(metaname, BAD_CAST SWISH_DEFAULT_METANAME)
        || xmlStrEqual(metaname, BAD_CAST SWISH_TITLE_METANAME)
        ) {
        indexer.index_text((const char *)xmlBufferContent(buffer), weight);
    }
}

static void
add_properties(
    xmlBufferPtr buffer,
    Xapian::Document doc,
    xmlChar *name
)
{
    swish_Property *
        prop;
    const xmlChar *substr;
    const xmlChar *buf;
    int sub_len;
    string prop_buf;

    prop = (swish_Property *)swish_hash_fetch(s3->config->properties, name);

    // break the buffer up into substrings and re-join.
    // i.e. replace SWISH_TOKENPOS_BUMPER with a space.
    // this is a compile-time option. 
    // TODO make it run-time option.
    
    if (SWISH_SPLIT_PROPERTIES) {
        buf = xmlBufferContent(buffer);
        while ((substr = xmlStrstr(buf, (const xmlChar *)SWISH_TOKENPOS_BUMPER)) != NULL) {
            sub_len = substr - buf;
            //SWISH_DEBUG_MSG("%d <%s> substr: %s", sub_len, name, xmlStrsub(buf, 0, sub_len) );
            prop_buf += string((const char *)xmlStrsub(buf, 0, sub_len));
            prop_buf += " ";
            buf = substr + 1;
        }
        // last one
        if (buf != NULL) {
            //SWISH_DEBUG_MSG("%d <%s> substr: %s", xmlStrlen(buf), name, buf );
            prop_buf += string((const char *)buf);
        }
    }
    else {
        prop_buf = string((const char *)xmlBufferContent(buffer));
    }
    if (prop->type == SWISH_PROP_INT && prop_buf.length()) {
        //SWISH_DEBUG_MSG("add prop %s: %s", name, prop_buf.c_str());
        doc.add_value(prop->id, Xapian::sortable_serialise(string_to_int(prop_buf)));
    }
    else {
        doc.add_value(prop->id, prop_buf);
    }
}

void
handler(
    swish_ParserData *parser_data
)
{
    //printf("nwords: %d\n", parser_data->docinfo->nwords);

    twords += parser_data->docinfo->nwords;

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        swish_docinfo_debug(parser_data->docinfo);
    }
    if (SWISH_DEBUG & SWISH_DEBUG_TOKENLIST) {
        swish_token_list_debug(parser_data->token_iterator);
    }
    if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
        swish_nb_debug(parser_data->properties, BAD_CAST "Property");
        swish_nb_debug(parser_data->metanames, BAD_CAST "MetaName");
    }

    // Put the data in the document
    Xapian::Document newdocument;
    xmlChar *
        title = BAD_CAST swish_nb_get_value(parser_data->properties,
                                              BAD_CAST SWISH_PROP_TITLE);
    //printf("title = %s", (char *)title);
    string
        unique_id = SWISH_PREFIX_URL + string((const char *)parser_data->docinfo->uri);
    string
        record = "url=" + string((const char *)parser_data->docinfo->uri);
    record += "\ntitle=" + string((const char *)title);
    record += "\ntype=" + string((const char *)parser_data->docinfo->mime);
    record += "\nmodtime=" + long_to_string(parser_data->docinfo->mtime);
    record += "\nsize=" + long_to_string(parser_data->docinfo->size);
    newdocument.set_data(record);

    // Index the title, document text, and keywords.
    indexer.set_document(newdocument);
    indexer.increase_termpos(100);
    newdocument.add_term(SWISH_PREFIX_MTIME +
                         long_to_string(parser_data->docinfo->mtime));
    newdocument.add_term(unique_id);

    struct tm *
        tm = localtime(&(parser_data->docinfo->mtime));
    string
        date_term = "D" + date_to_string(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    newdocument.add_term(date_term);    // Date (YYYYMMDD)
    date_term.resize(7);
    date_term[0] = 'M';
    newdocument.add_term(date_term);    // Month (YYYYMM)
    date_term.resize(5);
    date_term[0] = 'Y';
    newdocument.add_term(date_term);    // Year (YYYY)

    // add all docinfo values as properties
    newdocument.add_value(SWISH_PROP_MTIME_ID,
                          long_to_string(parser_data->docinfo->mtime));
    newdocument.add_value(SWISH_PROP_DOCPATH_ID,
                          string((const char *)parser_data->docinfo->uri));
    newdocument.add_value(SWISH_PROP_SIZE_ID, long_to_string(parser_data->docinfo->size));
    newdocument.add_value(SWISH_PROP_MIME_ID,
                          string((const char *)parser_data->docinfo->mime));
    newdocument.add_value(SWISH_PROP_PARSER_ID,
                          string((const char *)parser_data->docinfo->parser));
                          
    // TODO this is usually == 0 since xapian tokenizes; 
    // get posting size from db or newdocument?
    newdocument.add_value(SWISH_PROP_NWORDS_ID,
                          long_to_string(parser_data->docinfo->nwords));

    // title is special value
    newdocument.add_value(SWISH_PROP_TITLE_ID, string((const char *)title));

    // add all metanames and properties
    xmlHashScan(parser_data->metanames->hash, (xmlHashScanner)add_metanames, s3->config);
    xmlHashScan(parser_data->properties->hash, (xmlHashScanner)add_properties, &newdocument);

    if (!skip_duplicates) {
        // If this document has already been indexed, update the existing
        // entry.
        try {
            Xapian::docid did = wdb.replace_document(unique_id, newdocument);
            if (did < updated.size()) {
                updated[did] = true;
                if (verbose) {
                    cout << "        .... updated." << endl;
                }
            }
            else {
                if (verbose) {
                    cout << "        .... added." << endl;
                }
            }
        }
        catch(...) {
            // FIXME: is this ever actually needed?
            wdb.add_document(newdocument);
            if (verbose) {
                cout << "         .... added (failed re-seek for duplicate)." << endl;
            }
        }
    }
    else {
        // If this doc is already in index, skip it.
        if (wdb.term_exists(unique_id)) {
            if (verbose) {
                cout << "         .... skipped (already in db)." <<endl;
            }
        }
        else {  
            wdb.add_document(newdocument);
            if (verbose) {
                cout << "         .... added." << endl;
            }
        }
    }
}

int
open_writeable_index(
    char *dbpath
)
{
    int
        exitcode = 1;
    char *
        header;
    try {
        if (!overwrite) {
            wdb = Xapian::WritableDatabase(dbpath, Xapian::DB_CREATE_OR_OPEN);
            if (!skip_duplicates) {
                // + 1 so that wdb.get_lastdocid() is a valid subscript.
                updated.resize(wdb.get_lastdocid() + 1);
            }
        }
        else {
            wdb = Xapian::WritableDatabase(dbpath, Xapian::DB_CREATE_OR_OVERWRITE);
        }

        indexer.set_stemmer(stemmer);

        // read header if it exists
        header = make_header_path(dbpath);
        if (swish_fs_file_exists(BAD_CAST header)) {
            if (overwrite) {
                if (unlink(header) == -1) {
                    SWISH_CROAK("Failed to unlink existing header file %s", header);
                }
            }
            else {
                swish_header_merge(header, s3->config);
            }
        }
        swish_xfree(header);

        wdb.flush();

        // cout << "\n\nNow we have " << wdb.get_doccount() << " documents.\n";
        exitcode = 0;
    }
    catch(const Xapian::Error & e
    )
    {
        cout << "Exception: " << e.get_msg() << endl;
    } catch(const string & s
    )
    {
        cout << "Exception: " << s << endl;
    } catch(const char *s
    )
    {
        cout << "Exception: " << s << endl;
    } catch(...) {
        cout << "Caught unknown exception" << endl;
    }

    return exitcode;
}

char*
make_header_path(
    char *dbpath
)
{
    char *header;
    int len;
    len = strlen(dbpath) + 1 + strlen(SWISH_HEADER_FILE) + 1;
    header = (char*)swish_xmalloc(len);
    if (!snprintf(header, len, "%s%c%s", dbpath, SWISH_PATH_SEP, SWISH_HEADER_FILE)) {
        SWISH_CROAK("Failed to make header path from %s%c%s", dbpath, SWISH_PATH_SEP, SWISH_HEADER_FILE);
    }
    return header;
}

int
open_readable_index(
    char *dbpath,
    xmlChar *stemmer_lang
)
{
    int
        exitcode = 1;
    char *
        header;
                
    if (!swish_fs_is_dir((xmlChar*)dbpath)) {
        SWISH_CROAK("No such index at %s", dbpath);
    }

    try {
        rdb = Xapian::Database::Database(dbpath);

        header = make_header_path(dbpath);
        if (swish_fs_file_exists(BAD_CAST header)) {
            swish_header_merge(header, s3->config);
        }
        
        // check for stemmer lang
        if (stemmer_lang != NULL
            &&
            swish_hash_exists(s3->config->index, BAD_CAST "Stemmer")
        ) {
            if ((xmlStrEqual(stemmer_lang, BAD_CAST swish_hash_fetch(s3->config->index, BAD_CAST "Stemmer"))) == 0) {
                SWISH_CROAK("You asked for --stemmer=%s but config has %s",
                    stemmer_lang, BAD_CAST swish_hash_fetch(s3->config->index, BAD_CAST "Stemmer"));
                
            }
        
        }

        exitcode = 0;
    } catch(const Xapian::Error & e
    )
    {
        cout << "Exception: " << e.get_msg() << endl;
    } catch(const string & s
    )
    {
        cout << "Exception: " << s << endl;
    } catch(const char *s
    )
    {
        cout << "Exception: " << s << endl;
    } catch(...) {
        cout << "Caught unknown exception" << endl;
    }

    return exitcode;

}

/*  -- parse print control and print it
    --  output on file <f>
    --  *s = "\....."
    -- return: string ptr to char after control sequence.
*/

static xmlChar *
get_control_char(
    xmlChar *s,
    xmlChar *c
)
{
    xmlChar *se;

    if (*s != '\\')
        return s;

    *c = swish_get_C_escaped_char(s, &se);
    return se;
}

struct outputFormat 
{
    string  tmpl;
    int     props[SWISH_MAX_OUTPUT_PROPS];
    int     num_props;
};

static outputFormat
init_outputFormat(
)
{
    outputFormat of;
    char tmpl[] = "%c %c \"%c\" \"%c\"\n";
    char buf[strlen(tmpl)+1];
    sprintf(buf, tmpl, 
        SWISH_PROP_OUTPUT_PLACEHOLDER, 
        SWISH_PROP_OUTPUT_PLACEHOLDER, 
        SWISH_PROP_OUTPUT_PLACEHOLDER, 
        SWISH_PROP_OUTPUT_PLACEHOLDER);
    of.tmpl = buf;
    of.props[0] = SWISH_PROP_RANK_ID;
    of.props[1] = SWISH_PROP_DOCPATH_ID;
    of.props[2] = SWISH_PROP_TITLE_ID;
    of.props[3] = SWISH_PROP_SIZE_ID;
    of.num_props = 4;
    return of;
}

static void
build_output_format(
    xmlChar *tmpl,
    outputFormat *of
)
{
    xmlChar *tmp, *tmp2;
    xmlChar ctrl;   // TODO could be multibyte utf-8 seq
    int len;
    xmlChar *propname;
    swish_Property *prop;
    int prop_id;

    // reset in case it's been set before
    of->num_props = 0;
    of->tmpl.clear();

    while (*tmpl) {
        switch (*tmpl) {
        
            case '<':
                        
                // find the delimiter
                tmp = tmpl;
                tmp = swish_str_skip_ws(tmp);
                if (*tmp != '<')
                    SWISH_CROAK("error parsing output_format string");
                        
                // bump to next non-whitespace char
                tmp = swish_str_skip_ws(++tmp);
                
                // get just the propname
                tmp2 = tmp;
                while (*tmp) {
                    if ((*tmp == '>') || isspace(*tmp)) {
                    /* delim > or whitespace ? */
                        break;              /* break on delim */
                    }
                    tmp++;
                }
                len = tmp - tmp2;
                propname = swish_xstrndup(tmp2, len);
                tmpl = ++tmp;
                
                //SWISH_DEBUG_MSG("propname = '%s'", propname);
                prop_id = swish_property_get_id(propname, s3->config->properties);
                
                if (of->num_props == SWISH_MAX_OUTPUT_PROPS) {
                    SWISH_CROAK("Max number of PropertyNames reached in output template: %d",
                        SWISH_MAX_OUTPUT_PROPS);
                }
                
                // update the outputFormat
                of->tmpl += SWISH_PROP_OUTPUT_PLACEHOLDER;
                of->props[of->num_props++] = prop_id;
                swish_xfree(propname);
                
                break;

            case '\\':             /* print format controls */
                tmpl = get_control_char(tmpl, &ctrl);
                of->tmpl += ctrl;
                break;


            default:               /* verbatim */
                of->tmpl += *tmpl;
                tmpl++;
                break;
                
        }            
    }    
}

struct PropertyValueRangeProcessor : public Xapian::ValueRangeProcessor {
    swish_Property *prop;
    PropertyValueRangeProcessor(swish_Property *prop_) : prop(prop_) {}

    Xapian::valueno operator()(std::string &begin, std::string &end) {
        string::size_type colon = begin.find(":", 0);
        if (colon == string::npos) {
            return Xapian::BAD_VALUENO;
        }
        string propname = begin.substr(0, colon);

        if (propname == string((const char*)prop->name)) {
            //SWISH_DEBUG_MSG("match propname: %s", propname.c_str());
        }
        else {
            return Xapian::BAD_VALUENO;
        }
        
        // ignore the prefix
        begin.erase(0, colon+1);
        
        switch (prop->type) {
            case SWISH_PROP_STRING: {
                begin = Xapian::Unicode::tolower(begin);
                end = Xapian::Unicode::tolower(end);
            }
            break;
            
            case SWISH_PROP_INT: {
            
                // make ints sortable as strings
                size_t b_b = 0, e_b = 0;
                size_t b_e = string::npos, e_e = string::npos;

                // Adjust begin string if necessary.
                if (b_e != string::npos)
            	    begin.resize(b_e);
            
                // Adjust end string if necessary.
                if (e_e != string::npos)
            	    end.resize(e_e);
                    
                //SWISH_DEBUG_MSG("begin='%s' end='%s'", begin.c_str(), end.c_str());

                double beginnum, endnum;
                const char * startptr;
                char * endptr;
                int errno;
            
                errno = 0;
                startptr = begin.c_str() + b_b;
                beginnum = strtod(startptr, &endptr);
                if (endptr != startptr - b_b + begin.size())
            	    // Invalid characters in string
            	    return Xapian::BAD_VALUENO;
                if (errno)
            	    // Overflow or underflow
            	    return Xapian::BAD_VALUENO;
            
                errno = 0;
                startptr = end.c_str() + e_b;
                endnum = strtod(startptr, &endptr);
                if (endptr != startptr - e_b + end.size())
            	    // Invalid characters in string
            	    return Xapian::BAD_VALUENO;
                if (errno)
            	    // Overflow or underflow
            	    return Xapian::BAD_VALUENO;
            
                //SWISH_DEBUG_MSG("beginnum=%d endnum=%d", beginnum, endnum);
                begin.assign(Xapian::sortable_serialise(beginnum));
                end.assign(Xapian::sortable_serialise(endnum));
                //SWISH_DEBUG_MSG("begin='%s' end='%s'", begin.c_str(), end.c_str());

            }
            break;
            
            case SWISH_PROP_DATE: {
                // treat like a string, since format allows
                // i.e. nothing to do.
            
            }
            break;
            
            default: 
                SWISH_CROAK("No PropertyValueRangeProcessor for %s prop_type %d", 
                    prop->name, prop->type);
        }
            
        
        return prop->id;
    }
};

static void
add_property_range_processor(
    swish_Property *prop,
    Xapian::QueryParser qp,
    xmlChar *name
)
{    
    PropertyValueRangeProcessor * proc;
    proc = new PropertyValueRangeProcessor(prop);
    qp.add_valuerangeprocessor(proc);
    //SWISH_DEBUG_MSG("added PropertyValue for %s %d", name, prop->id);
}


int
search(
    char *qstr,
    xmlChar *output_format,
    xmlChar *sort_string,
    unsigned int results_offset,
    unsigned int results_limit
)
{
    int total_matches;
    Xapian::Enquire * enquire;
    Xapian::Query query;
    Xapian::QueryParser qparser;
    Xapian::MSet mset;
    Xapian::MSetIterator iterator;
    Xapian::Document doc;
    outputFormat of;
    int i, j;
    const char *ofbuf;
    swish_StringList *sort_sl;

    if (sort_string != NULL) {
        sort_sl = swish_stringlist_parse_sort_string(sort_string, s3->config);
    }
    else {
        sort_sl = NULL;
    }
            
    total_matches = 0;
    qparser.set_stemmer(stemmer);
    qparser.set_database(rdb);
    of = init_outputFormat();
    if (output_format) {
        build_output_format(output_format, &of);
    }

    // map all human metanames to internal prefix
    xmlHashScan(s3->config->metanames, (xmlHashScanner)add_prefix, &qparser);

    /*
     * for each property, add the ValueRangeProcessor
     * NOTE that we intentionally leak the ValueRangeProcessor objects created
     * in each hash scan, because otherwise they are destroyed before the qparser
     * can use them, causing a segfault.
     * See http://lists.xapian.org/pipermail/xapian-devel/2009-November/001193.html
     */
    xmlHashScan(s3->config->properties, (xmlHashScanner)add_property_range_processor, &qparser);
    
    // TODO boolean_prefix?

    try {
        query = qparser.parse_query(string(qstr),
            Xapian::QueryParser::FLAG_WILDCARD | 
            Xapian::QueryParser::FLAG_BOOLEAN | 
            Xapian::QueryParser::FLAG_BOOLEAN_ANY_CASE | 
            Xapian::QueryParser::FLAG_PHRASE
        );
        //cout << "# " + query.get_description() << endl;
    }
    catch(Xapian::QueryParserError &e) {
        SWISH_CROAK("query parser error: %s", e.get_msg().c_str());
    }
    
    enquire = new Xapian::Enquire(rdb);
    enquire->set_query(query);
    
    if (sort_sl != NULL) {
        swish_stringlist_debug(sort_sl);
        Xapian::MultiValueSorter sorter;
        int prop_id;
        bool dir;
        for (i=sort_sl->n-1; i>0; i-=2) {
            prop_id = swish_property_get_id(sort_sl->word[i-1], s3->config->properties);
            dir = xmlStrEqual(sort_sl->word[i], BAD_CAST "asc") ? false : true;
            SWISH_DEBUG_MSG("sorter.add(%d, %d)", prop_id, dir);
            sorter.add(prop_id, dir);
        }
        enquire->set_sort_by_key(&sorter, true);
    }

    if (num_facets) {
        FacetFinder facet_finder;
        facet_finder_init();
        mset = enquire->get_mset(results_offset, results_limit, SWISH_FACET_FINDER_LIMIT, NULL, &facet_finder);
    }
    else {
        mset = enquire->get_mset(results_offset, results_limit);
    }
    printf("# Results include %d of %d estimated total matches\n", mset.size(), mset.get_matches_estimated());
    cout << "# " + query.get_description() << endl;
    iterator = mset.begin();
        
    for (; iterator != mset.end(); ++iterator) {
        doc = iterator.get_document();
        
        // this looks a little convoluted but basically
        // just iterating over the outputFormat template
        // and fetching values from the doc as needed.
        i = 0;
        ofbuf = of.tmpl.c_str();
        for ( j=0; ofbuf[j] != '\0'; j++ ) {
            if (ofbuf[j] == SWISH_PROP_OUTPUT_PLACEHOLDER) {
                // a property placeholder
                if (of.props[i] == SWISH_PROP_RANK_ID) {
                    printf("%3d0", iterator.get_percent());
                }
                else if (of.props[i] == SWISH_PROP_MTIME_ID) {
                    printf("%s", swish_time_format(
                            (time_t)swish_string_to_int((char*)doc.get_value(of.props[i]).c_str())
                    ));
                }
                else {
                    cout << doc.get_value(of.props[i]);
                }
                i++;
            }
            else {
                // literal char
                printf("%c", ofbuf[j]);
            }
        }
        
        total_matches++;
    }
    
    if (num_facets) {
        facet_finder_show();
    }
    
    //printf("# %d total matches\n", total_matches);
    
}

void
libxml2_version(
)
{
    printf("  libxml2 version:\t%s\n", swish_libxml2_version());
}

void
swish_version(
)
{
    printf("libswish3 version:\t%s\n", swish_lib_version());
    printf("    swish version:\t%s\n", SWISH_VERSION);
}

void
xapian_version(
)
{
    printf("libxapian version:\t%s\n", XAPIAN_VERSION);
}

static boolean 
delete_document(
    xmlChar *uri
)
{
    string unique_id;
    unique_id = SWISH_PREFIX_URL + string((const char *)uri);
    if (wdb.term_exists(unique_id)) {
        printf("%s   ... deleted\n", uri);
        wdb.delete_document(unique_id);
        return SWISH_TRUE;
    }
    else {
        printf("%s   ... skipped (not in database)\n", uri);
        return SWISH_FALSE;
    }
}

static void
get_db_data(
    char *dbpath,
    xmlChar *db_data
)
{
    // TODO dump data from index, similar to delve


}

int
main(
    int argc,
    char **argv
)
{
    int
        i,
        j,
        ch;
    extern char *
        optarg;
    extern int
        optind;
    int
        option_index;
    long int
        files;
    long int 
        num_lines;
    char *
        etime;
    char *
        query;
    char *
        query_extra;
    char *
        dbpath;
    xmlChar *
        output_format;
    char *
        filelist;
    string
        line_in_file;
    xmlChar *
        buf;
    xmlChar *
        sort_string;
    xmlChar *
        stemmer_lang;
    char *
        header;
    double
        start_time, tmp_time;
    xmlChar *
        config_file;
    xmlChar *
        db_data;
    boolean delete_mode;
    unsigned int results_offset, results_limit;
    boolean follow_symlinks;
    swish_StringList *
        limit_string;
    unsigned int stringlengths;

    delete_mode = SWISH_FALSE;
    config_file = NULL;
    output_format = NULL;
    filelist = NULL;
    sort_string = NULL;
    option_index = 0;
    files = 0;
    query = NULL;
    dbpath = NULL;
    
    swish_setup();
    start_time = swish_time_elapsed();
    s3 = swish_3_init(&handler, NULL);
    results_offset = 0;
    results_limit  = 100;
    facet_list = NULL;
    follow_symlinks = SWISH_FALSE;
    stemmer_lang = NULL;
    db_data = NULL;
    query_extra = NULL;

    while ((ch = getopt_long(argc, argv, "c:d:f:i:q:s:SohDlL:x:vF:b:m:t:T:", longopts, &option_index)) != -1) {

        switch (ch) {
        case 0:                /* If this option set a flag, do nothing else now. */
            if (longopts[option_index].flag != 0)
                break;
            printf("option %s", longopts[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'c':
            //printf("optarg = %s\n", optarg);
            config_file = swish_xstrdup(BAD_CAST optarg);
            break;

        case 'd':
            printf("turning on debug mode: %s\n", optarg);

            if (!isdigit(optarg[0]))
                err(1, "-d option requires a positive integer as argument\n");

            SWISH_DEBUG = swish_string_to_int(optarg);
            break;

        case 'o':
            overwrite = 1;
            break;
            
        case 'v':
            verbose = 1;
            s3->parser->verbosity = 1;
            break;

        case 'i':
            dbpath = (char *)swish_xstrdup(BAD_CAST optarg);
            break;

        case 'S':
            skip_duplicates = 1;
            break;
            
        case 's':
            sort_string = swish_xstrdup(BAD_CAST optarg);
            break;
            
        case 't':
            try {
                stemmer = Xapian::Stem(optarg);
                stemmer_lang = swish_xstrdup(BAD_CAST optarg);
            } catch (const Xapian::Error &) {
                cerr << "Unknown stemming language '" << optarg << "'.\n";
                cerr << "Available language names are: "
                     << Xapian::Stem::get_available_languages() << endl;
                return 1;
            }
            break;

        case 'T':
            db_data = swish_xstrdup(BAD_CAST optarg);
            break;
            
        case 'q':
            query = (char *)swish_xstrdup(BAD_CAST optarg);
            break;
        
        case 'f':
            filelist = (char *)swish_xstrdup(BAD_CAST optarg);
            break;
            
        case 'F':
            facet_list = swish_stringlist_build(BAD_CAST optarg);
            num_facets = facet_list->n;
            break;
            
        case 'D':
            delete_mode = SWISH_TRUE;
            break;
            
        case 'x':
            output_format = swish_xstrdup(BAD_CAST optarg);
            break;
        
        case 'b':
            results_offset = swish_string_to_int(optarg);
            break;
            
        case 'm':
            results_limit = swish_string_to_int(optarg);
            break;
            
        case 'l':
            follow_symlinks = SWISH_TRUE;
            break;
            
        case 'L':
            limit_string = swish_stringlist_build(BAD_CAST optarg);
            if (limit_string->n != 3) {
                SWISH_CROAK("Bad format in --limit string: %s", optarg);
            }
            stringlengths = 0;
            // get the lengths so we know how much to malloc
            for (j=0; j<limit_string->n; j++) {
                stringlengths += xmlStrlen(limit_string->word[j]);
            }
            if (query_extra == NULL) {
                query_extra = (char*)swish_xmalloc((stringlengths+4)*sizeof(char));
                snprintf(query_extra, (stringlengths+4)*sizeof(char), "%s:%s..%s",
                    limit_string->word[0], limit_string->word[1], limit_string->word[2]);
            }
            else {
                query_extra = (char*)swish_xrealloc(query_extra, 
                    (strlen(query_extra)+stringlengths+6)*(sizeof(char)));
                snprintf(query_extra, (strlen(query_extra)+stringlengths+6)*(sizeof(char)),
                    "%s AND %s:%s..%s", query_extra, limit_string->word[0], 
                    limit_string->word[1], limit_string->word[2]);
            }
            swish_stringlist_free(limit_string);
            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }

    i = optind;

    /*
       die with no args or filelist
     */
    if ((!i || i >= argc) && !query && !filelist) {
        swish_3_free(s3);
        usage();

    }

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        swish_config_debug(s3->config);
    }

    if (!dbpath) {
        dbpath = (char *)swish_xstrdup(BAD_CAST SWISH_INDEX_FILENAME);
    }
    
    if (db_data) {
        get_db_data(dbpath, db_data);
    
    
        return 0;
    }
    
    // indexing mode
    if (!query) {

        if (open_writeable_index(dbpath)) {
            SWISH_CROAK("Failed to open writeable index '%s'", dbpath);
        }
        
        /* merge config file with header AFTER we've loaded 
         * the header above in open_writeable_index().
         * This way the header is updated with the config file contents
         * when we write it below, and we can effectively update a header
         * file after the initial indexing.
         */
        if (config_file != NULL) {
            s3->config = swish_config_add(s3->config, config_file);
        }
        
        
        // look for stemmer mismatch
        if (swish_hash_exists(s3->config->index, BAD_CAST "Stemmer")) {
            if (stemmer_lang != NULL
                &&
                (xmlStrEqual(stemmer_lang, BAD_CAST swish_hash_fetch(s3->config->index, BAD_CAST "Stemmer"))) == 0
            ) {
                SWISH_CROAK("You asked for --stemmer=%s but index already configured for %s.",
                    stemmer_lang, BAD_CAST swish_hash_fetch(s3->config->index, BAD_CAST "Stemmer"));
            
            }
            else {
                try {
                    stemmer = Xapian::Stem((char*)swish_hash_fetch(s3->config->index, BAD_CAST "Stemmer"));
                } catch (const Xapian::Error &) {
                    cerr << "Unknown stemming language in header file: '" << optarg << "'.\n";
                    cerr << "Available language names are: "
                     << Xapian::Stem::get_available_languages() << endl;
                    return 1;
                }
            }
        }
        else if (stemmer_lang != NULL) {
            swish_hash_add(s3->config->index, BAD_CAST "Stemmer", swish_xstrdup(stemmer_lang));
        }
        else {
            swish_hash_add(s3->config->index, BAD_CAST "Stemmer", swish_xstrdup(BAD_CAST "none"));
        }

        // always turn tokenizing off since we use the Xapian term tokenizer.
        s3->config->flags->tokenize = SWISH_FALSE;
        s3->analyzer->tokenize = SWISH_FALSE;

        for (; i < argc; i++) {
            if (argv[i][0] != '-') {
                if (delete_mode) {
                    if (delete_document(BAD_CAST argv[i]))
                        files++;
                }
                else {
                    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                    printf("%s ... ", argv[i]);
                    if (swish_fs_is_dir(BAD_CAST argv[i])) {
                        files += swish_parse_directory(s3, BAD_CAST argv[i], follow_symlinks);
                        printf(" ok\n");
                    }
                    else if (!swish_parse_file(s3, BAD_CAST argv[i])) {
                        files++;
                        printf(" ok\n");
                    }
                }

            }
            
            // TODO delete docs from stdin via header api
            else if (argv[i][0] == '-' && !argv[i][1]) {
                files = swish_parse_fh(s3, NULL);
            }

        }
        
        if (filelist != NULL) {
        /* open the file, iterating over each line and indexing each. */
        
            num_lines = swish_io_count_operable_file_lines((xmlChar*)filelist);
            printf("%ld valid file names found in %s\n", num_lines, filelist);
        
            ifstream input_file(filelist,ifstream::in);
            if (input_file.good()) {
            
                while(! input_file.eof()) {
                    getline(input_file, line_in_file);
                    if (line_in_file.empty() || (line_in_file.length() == 0))
                        continue;
                        
                    buf = BAD_CAST line_in_file.c_str();
                    if (swish_io_is_skippable_line(buf))
                        continue;
                                                
                    if (delete_mode) {
                        if (delete_document(buf)) {
                            files++;
                            if (!verbose) {
                                double per = ((double)files / (double)num_lines);
                                printf("\r%10ld of %10ld files (%02.1f%%)", 
                                    files, num_lines, per*100);
                            }
                        }
                    }
                    else {
                        if (verbose) {
                            cout << line_in_file;
                        }
                        if (!swish_parse_file(s3, buf)) {
                            files++;
                            if (!verbose) {
                                double per = ((double)files / (double)num_lines);
                                printf("\r%10ld of %10ld files (%02.1f%%)", 
                                    files, num_lines, per*100);
                            }
                        }
                    }
                }
                
                input_file.close();
            
            }
            else {
                SWISH_CROAK("Failed to open filelist %s", filelist);
            }
        
        }

        if (delete_mode) {
            wdb.flush();
            printf("%ld documents deleted from database\n", files);
        }
        else {
            printf("\n\n%ld files indexed\n", files);
            //printf("# total tokenized words: %ld\n", twords);

            // how do we know when to write a header file?
            // it's legitimate to re-write if the config was defined
            // but also if it is not (defaults).
            // so we re-write every time we have a writeable db.
            swish_hash_replace(s3->config->index, BAD_CAST "Format", swish_xstrdup(BAD_CAST SWISH_XAPIAN_FORMAT));
            swish_hash_replace(s3->config->index, BAD_CAST "Name", swish_xstrdup(BAD_CAST dbpath));

            header = make_header_path(dbpath);
            printf("writing header to %s ... ", header);
            swish_header_write(header, s3->config);
            printf("\n");
            swish_xfree(header);
            
            /* index overhead time measured separately */
            etime = swish_time_print(swish_time_elapsed() - start_time);
            printf("# indexing time: %s\n", etime);
            swish_xfree(etime);

            /* flush explicitly so that total time does not print before object destroy */
            wdb.flush();
        }

    }

    // searching mode
    else {
        // append any query parts (e.g. limits)
        if (query_extra != NULL) {
            //SWISH_DEBUG_MSG("query_extra: '%s'", query_extra);
            query = (char*)swish_xrealloc(query, (strlen(query)+strlen(query_extra)+6)*sizeof(char));
            snprintf(query, (strlen(query)+strlen(query_extra)+6)*sizeof(char),
                "%s AND %s", query, query_extra);
            swish_xfree(query_extra);
        }
        open_readable_index(dbpath, stemmer_lang);
        search(query, output_format, sort_string, results_offset, results_limit);
        swish_xfree(BAD_CAST query);
    }

    etime = swish_time_print(swish_time_elapsed() - start_time);
    printf("# total time: %s\n", etime);
    swish_xfree(etime);
    swish_xfree(dbpath);
    swish_3_free(s3);

    if (config_file != NULL)
        swish_xfree(config_file);
        
    if (filelist != NULL)
        swish_xfree(filelist);
        
    if (output_format != NULL)
        swish_xfree(output_format);
        
    if (sort_string != NULL)
        swish_xfree(sort_string);
        
    if (facet_list != NULL)
        swish_stringlist_free(facet_list);
        
    if (stemmer_lang != NULL)
        swish_xfree(stemmer_lang);

    return (0);
}
