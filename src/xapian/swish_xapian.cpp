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

#include <xapian.h>

#include <libxml/hash.h>
#include "libswish3.h"

using namespace std;

/* prototypes */
int main(
    int argc,
    char **argv
);
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
    char *query
);

/* global vars */
static int debug = 0;
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
static int
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
    {"config", required_argument, 0, 'c'},
    {"debug", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {"index", required_argument, 0, 'i'},
    {"skip-duplicates", no_argument, 0, 's'},
    {"overwrite", no_argument, 0, 'o'},
    {"query", required_argument, 0, 'q'},
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
    qp.add_prefix(string((const char *)name),
                  int_to_string(meta->id) + string((const char *)":"));
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
    prop = (swish_Property *)swish_hash_fetch(s3->config->properties, name);
    //SWISH_DEBUG_MSG("adding property %s [%d]: %s", name, prop->id,
    //                xmlBufferContent(buffer));
    doc.add_value(prop->id, (const char *)xmlBufferContent(buffer));
}

void
handler(
    swish_ParserData *parser_data
)
{
    //printf("nwords: %d\n", parser_data->docinfo->nwords);

    twords += parser_data->docinfo->nwords;

    if (SWISH_DEBUG & SWISH_DEBUG_DOCINFO) {
        swish_debug_docinfo(parser_data->docinfo);
    }
    if (SWISH_DEBUG & SWISH_DEBUG_WORDLIST) {
        swish_debug_wordlist(parser_data->wordlist);
    }
    if (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER) {
        swish_debug_nb(parser_data->properties, BAD_CAST "Property");
        swish_debug_nb(parser_data->metanames, BAD_CAST "MetaName");
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
                cout << "        .... updated." << endl;
            }
            else {
                cout << "        .... added." << endl;
            }
        }
        catch(...) {
            // FIXME: is this ever actually needed?
            wdb.add_document(newdocument);
            cout << "added (failed re-seek for duplicate)." << endl;
        }
    }
    else {
        // If this were a duplicate, we'd have skipped it above.
        wdb.add_document(newdocument);
        cout << "added." << endl;
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
        if (swish_file_exists(BAD_CAST header.c_str())) {
            swish_merge_config_with_header((char *)header.c_str(), s3->config);
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
        if (swish_file_exists(BAD_CAST header.c_str())) {
            swish_merge_config_with_header((char *)header.c_str(), s3->config);
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

int
search(
    char *qstr
)
{
    int
        total_matches;
    Xapian::Enquire * enquire;
    Xapian::Query query;
    Xapian::QueryParser qparser;
    Xapian::MSet mset;
    Xapian::MSetIterator iterator;
    Xapian::Document doc;

    total_matches = 0;
    qparser.set_stemmer(stemmer);       // TODO make this configurable
    qparser.set_database(rdb);

    // map all human metanames to internal prefix
    xmlHashScan(s3->config->metanames, (xmlHashScanner)add_prefix, &qparser);

    // TODO boolean_prefix?

    try {
        query = qparser.parse_query(string(qstr));
    }
    catch(Xapian::QueryParserError & e) {
        SWISH_CROAK("query parser error: %s", e.get_msg().c_str());
    }

    // this is very simplistic. swish-e does paging etc.
    enquire = new Xapian::Enquire(rdb);
    enquire->set_query(query);
    mset = enquire->get_mset(0, 100);
    printf("# %d estimated matches\n", mset.get_matches_estimated());
    cout << "# " + query.get_description() << endl;
    iterator = mset.begin();
    
    // output format is simple, not as flexible as swish-e. 
    // But hey. It's an example.
    for (; iterator != mset.end(); ++iterator) {
        doc = iterator.get_document();
        printf("%3d0 %s \"%s\" %s\n", iterator.get_percent(),
               doc.get_value(SWISH_PROP_DOCPATH_ID).c_str(),
               doc.get_value(SWISH_PROP_TITLE_ID).c_str(),
               doc.get_value(SWISH_PROP_SIZE_ID).c_str()
            );
        total_matches++;
    }
    
    //printf("# %d total matches\n", total_matches);
}

int
usage(
)
{

    char *
        descr = "swish_xapian is an example program for using libswish3 with Xapian\n";
    printf("swish_xapian [opts] [- | file(s)]\n");
    printf("opts:\n --config conf_file.xml\n --query <query>\n --debug [lvl]\n --help\n");
    printf(" --index path/to/index\n --skip-duplicates\n --overwrite\n");
    printf("\n%s\n", descr);
    exit(0);
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
    int
        files;
    char *
        etime;
    char *
        query;
    char *
        dbpath;
    string
        header;
    double
        start_time;
    xmlChar *
        config_file;

    config_file = NULL;
    option_index = 0;
    files = 0;
    query = NULL;
    dbpath = NULL;
    start_time = swish_time_elapsed();
    s3 = swish_init_swish3(&handler, NULL);

    while ((ch = getopt_long(argc, argv, "c:d:f:i:q:soh", longopts, &option_index)) != -1) {

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

        case 'i':
            dbpath = (char *)swish_xstrdup(BAD_CAST optarg);
            break;

        case 's':
            skip_duplicates = swish_string_to_int(optarg);
            break;

        case 'q':
            query = (char *)swish_xstrdup(BAD_CAST optarg);
            break;

        case '?':
        case 'h':
        default:
            usage();

        }

    }

    if (config_file != NULL) {
        s3->config = swish_add_config(config_file, s3->config);
    }

    i = optind;

    /*
       die with no args 
     */
    if ((!i || i >= argc) && !query) {
        swish_free_swish3(s3);
        usage();

    }

    if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
        swish_debug_config(s3->config);
    }

    if (!dbpath) {
        dbpath = (char *)swish_xstrdup(BAD_CAST SWISH_INDEX_FILENAME);
    }

    // indexing mode
    if (!query) {

        open_writeable_index(dbpath);

        for (; i < argc; i++) {
            if (argv[i][0] != '-') {
                //printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                printf("parse_file for %s", argv[i]);
                if (!swish_parse_file(s3, (unsigned char *)argv[i]))
                    files++;

            }
            else if (argv[i][0] == '-' && !argv[i][1]) {
                printf("reading from stdin\n");
                files = swish_parse_fh(s3, NULL);
            }

        }

        printf("\n\n%d files indexed\n", files);
        printf("total words: %d\n", twords);

        // how do we know when to write a header file?
        // it's legitimate to re-write if the config was defined
        // but also if it is not (defaults).
        // so we re-write every time we have a writeable db.
        header =
            dbpath + string((const char *)SWISH_PATH_SEP) +
            string((const char *)SWISH_HEADER_FILE);
        swish_write_header((char *)header.c_str(), s3->config);

    }

    // searching mode
    else {
        open_readable_index(dbpath);
        search(query);
        swish_xfree(BAD_CAST query);
    }

    etime = swish_print_time(swish_time_elapsed() - start_time);
    printf("# %s total time\n\n", etime);
    swish_xfree(etime);
    swish_xfree(dbpath);
    swish_free_swish3(s3);

    if (config_file != NULL)
        swish_xfree(config_file);

    return (0);
}
