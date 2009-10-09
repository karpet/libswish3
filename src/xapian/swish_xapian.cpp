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
    char *dbpath
);
int search(
    char *query,
    xmlChar *output_format,
    xmlChar *sort_string  
);
static boolean 
delete_document(
    xmlChar *uri
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
    "english"
);                              // TODO make this configurable
static
    Xapian::TermGenerator
    indexer;
static long int
    twords = 0;
static int
    skip_duplicates = 0;
static int
    overwrite = 0;
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
    {"overwrite",   no_argument, 0,         'o'},
    {"query",       required_argument, 0,   'q'},
    {"filelist",    required_argument, 0,   'f'},
    {"Delete",      no_argument, 0,         'D'},
    {"output",      required_argument, 0,   'x'},
    {0, 0, 0, 0}
};

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

int
string_to_int(
    const string & s
)
{
    return atoi(s.c_str());
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
    doc.add_value(prop->id, prop_buf);
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
                          
    // TODO this is usually == 0 since xapian tokenizes; get posting size from db or newdocument?
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
    string
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
        header =
            dbpath + string((const char *)SWISH_PATH_SEP) +
            string((const char *)SWISH_HEADER_FILE);
        if (swish_io_file_exists(BAD_CAST header.c_str())) {
            if (overwrite) {
                if (unlink(header.c_str()) == -1) {
                    SWISH_CROAK("Failed to unlink existing header file %s", 
                        header.c_str());
                }
            }
            else {
                swish_header_merge((char *)header.c_str(), s3->config);
            }
        }

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

int
open_readable_index(
    char *dbpath
)
{
    int
        exitcode = 1;
    string
        header;
    try {
        rdb = Xapian::Database::Database(dbpath);

        header =
            dbpath + string((const char *)SWISH_PATH_SEP) +
            string((const char *)SWISH_HEADER_FILE);
        if (swish_io_file_exists(BAD_CAST header.c_str())) {
            swish_header_merge((char *)header.c_str(), s3->config);
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

int
search(
    char *qstr,
    xmlChar *output_format,
    xmlChar *sort_string
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
    qparser.set_stemmer(stemmer);       // TODO make this configurable
    qparser.set_database(rdb);
    of = init_outputFormat();
    if (output_format) {
        build_output_format(output_format, &of);
    }

    // map all human metanames to internal prefix
    xmlHashScan(s3->config->metanames, (xmlHashScanner)add_prefix, &qparser);

    // TODO boolean_prefix?

    try {
        query = qparser.parse_query(string(qstr),
         Xapian::QueryParser::FLAG_WILDCARD | 
         Xapian::QueryParser::FLAG_BOOLEAN | 
         Xapian::QueryParser::FLAG_BOOLEAN_ANY_CASE | 
         Xapian::QueryParser::FLAG_PHRASE
        );
    }
    catch(Xapian::QueryParserError & e) {
        SWISH_CROAK("query parser error: %s", e.get_msg().c_str());
    }

    enquire = new Xapian::Enquire(rdb);
    enquire->set_query(query);
    
    if (sort_sl != NULL) {
        // swish_stringlist_debug(sort_sl);
        Xapian::MultiValueSorter sorter;
        int prop_id;
        bool dir;
        for (i=0; i<sort_sl->n; i+=2) {
            prop_id = swish_property_get_id(sort_sl->word[i], s3->config->properties);
            dir = xmlStrEqual(sort_sl->word[i+1], BAD_CAST "asc") ? true : false;
            SWISH_DEBUG_MSG("sorter.add(%d, %d)", prop_id, dir);
            sorter.add(prop_id, dir);
        }
        enquire->set_sort_by_key(&sorter, true);
    }

    // TODO this is very simplistic. swish-e does paging etc.    
    mset = enquire->get_mset(0, 100);
    printf("# %d estimated matches\n", mset.get_matches_estimated());
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

int
usage(
)
{

    const char *
        descr = "swish_xapian is an example program for using libswish3 with Xapian\n";
    printf("swish_xapian [opts] [- | file(s)]\n");
    printf("opts:\n --config conf_file.xml\n --query 'query'\n --output 'format'\n --debug [lvl]\n --help\n");
    printf(" --index path/to/index\n --Skip-duplicates\n --overwrite\n --filelist file\n --sort 'string'\n");
    printf(" --Delete\n");
    printf("\n%s\n", descr);
    libxml2_version();
    swish_version();
    xapian_version();
    exit(0);
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

int
main(
    int argc,
    char **argv
)
{
    int
        i,
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
    string
        header;
    double
        start_time, tmp_time;
    xmlChar *
        config_file;
    boolean delete_mode;

    delete_mode = SWISH_FALSE;
    config_file = NULL;
    output_format = NULL;
    filelist = NULL;
    sort_string = NULL;
    option_index = 0;
    files = 0;
    query = NULL;
    dbpath = NULL;
    start_time = swish_time_elapsed();
    swish_setup();
    s3 = swish_3_init(&handler, NULL);

    while ((ch = getopt_long(argc, argv, "c:d:f:i:q:s:SohDx:v", longopts, &option_index)) != -1) {

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

        case 'q':
            query = (char *)swish_xstrdup(BAD_CAST optarg);
            break;
        
        case 'f':
            filelist = (char *)swish_xstrdup(BAD_CAST optarg);
            break;
            
        case 'D':
            delete_mode = SWISH_TRUE;
            break;
            
        case 'x':
            output_format = swish_xstrdup(BAD_CAST optarg);
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
                    //printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                    printf("%s ... ", argv[i]);
                    if (!swish_parse_file(s3, (unsigned char *)argv[i])) {
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
            printf("# total tokenized words: %ld\n", twords);

            // how do we know when to write a header file?
            // it's legitimate to re-write if the config was defined
            // but also if it is not (defaults).
            // so we re-write every time we have a writeable db.
            swish_hash_replace(s3->config->index, (xmlChar*)"Format", swish_xstrdup((xmlChar*)SWISH_XAPIAN_FORMAT));

            header =
                dbpath + string((const char *)SWISH_PATH_SEP) +
                string((const char *)SWISH_HEADER_FILE);
            swish_header_write((char *)header.c_str(), s3->config);

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
        open_readable_index(dbpath);
        search(query, output_format, sort_string);
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

    return (0);
}
