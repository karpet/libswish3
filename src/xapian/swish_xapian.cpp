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

/* example Swish3 program using Xapian IR backend */

//#include <config.h>

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
int             main(int argc, char **argv);
int             usage();
void            handler(swish_ParserData * parser_data);
int             open_writeable_index(char* dbpath);
int             open_readable_index(char* dbpath);
int             do_search(char* query);

/* global vars */
static int      debug = 0;
static Xapian::WritableDatabase wdb;
static Xapian::Database::Database rdb;
static Xapian::Stem stemmer("english");
static Xapian::TermGenerator indexer;
static int      twords = 0;
static int      skip_duplicates = 0;
static int      overwrite = 0;
static vector<bool> updated;
static swish_3* s3;


extern int SWISH_DEBUG;

static struct option longopts[] =
{
    {"config",          required_argument,  0, 'c'},
    {"debug",           required_argument,  0, 'd'},
    {"help",            no_argument,        0, 'h'},
    {"index",           required_argument,  0, 'i'},
    {"skip-duplicates", no_argument,        0, 's'},
    {"overwrite",       no_argument,        0, 'o'},
    {"query",           required_argument,  0, 'q'},
    {0, 0, 0, 0}
};


#define SWISH_PREFIX_URL    "U"
#define SWISH_PREFIX_MTIME  "T"
#define SWISH_PROP_LAST_MOD 0

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
string_to_int(const string &s)
{
    return atoi(s.c_str());
}

string
int_to_string(int val)
{
    CONVERT_TO_STRING("%d")
}

string
long_to_string(long val)
{
    CONVERT_TO_STRING("%ld")
}

string
double_to_string(double val)
{
    CONVERT_TO_STRING("%f")
}

string
date_to_string(int y, int m, int d)
{
    char buf[11];
    if (y < 0) y = 0; else if (y > 9999) y = 9999;
    if (m < 1) m = 1; else if (m > 12) m = 12;
    if (d < 1) d = 1; else if (d > 31) d = 31;
#ifdef SNPRINTF
    int len = SNPRINTF(buf, sizeof(buf), "%04d%02d%02d", y, m, d);
    if (len == -1 || len > BUFSIZE) return string(buf, BUFSIZE);
    return string(buf, len);
#else
    buf[sizeof(buf) - 1] = '\0';
    sprintf(buf, "%04d%02d%02d", y, m, d);
    if (buf[sizeof(buf) - 1]) abort(); /* Uh-oh, buffer overrun */
    return string(buf);
#endif
}

inline uint32_t binary_string_to_int(const std::string &s)
{
    if (s.size() != 4) return (uint32_t)-1;
    uint32_t v;
    memcpy(&v, s.data(), 4);
    return ntohl(v);
}

inline std::string int_to_binary_string(uint32_t v)
{
    v = htonl(v);
    return std::string(reinterpret_cast<const char*>(&v), 4);
}

static string
get_prefix(xmlChar* metaname, swish_Config* config)
{
    string prefix;
    swish_MetaName *meta = (swish_MetaName*)swish_hash_fetch(config->metanames, metaname);
    prefix = int_to_string(meta->id);
    return prefix;
}

static unsigned int
get_weight(xmlChar* metaname, swish_Config* config)
{
    unsigned int w;
    swish_MetaName *meta = (swish_MetaName*)swish_hash_fetch(config->metanames, metaname);
    return meta->bias > 0 ? meta->bias : 1; // TODO need to account for negative values.
}

static void
add_metanames(xmlBufferPtr buffer, void* config, xmlChar* metaname)
{
    // lookup weight and prefix
    string prefix       = get_prefix(metaname, (swish_Config*)config);
    unsigned int weight = get_weight(metaname, (swish_Config*)config);
    indexer.index_text((const char*)xmlBufferContent(buffer), weight, prefix);
}

static void
add_properties(xmlBufferPtr buffer, Xapian::Document doc, xmlChar* name)
{
    swish_Property* prop;
    prop = (swish_Property*)swish_hash_fetch(s3->config->properties, name);
    doc.add_value(prop->id, (const char*)xmlBufferContent(buffer));
}

void 
handler(swish_ParserData * parser_data)
{
    printf("nwords: %d\n", parser_data->docinfo->nwords);
    
    twords += parser_data->docinfo->nwords;

    if (SWISH_DEBUG)
    {
        swish_debug_docinfo(parser_data->docinfo);
        swish_debug_wordlist(parser_data->wordlist);
        swish_debug_nb(parser_data->properties, (xmlChar*)"Property");
        swish_debug_nb(parser_data->metanames, (xmlChar*)"MetaName");
    }
    
    
    // Put the data in the document
    Xapian::Document newdocument;
    string unique_id = SWISH_PREFIX_URL + string((const char*)parser_data->docinfo->uri);
    string record = "url=" + string( (const char*)parser_data->docinfo->uri );
	record += "\ntitle=" + string((const char*)
        swish_hash_fetch(parser_data->properties->hash, (xmlChar*)SWISH_PROP_TITLE));
    record += "\ntype=" + string( (const char*)parser_data->docinfo->mime );
	record += "\nmodtime=" + long_to_string(parser_data->docinfo->mtime);
	record += "\nsize=" + long_to_string(parser_data->docinfo->size);
    newdocument.set_data(record);

    // Index the title, document text, and keywords.
    indexer.set_document(newdocument);
	indexer.increase_termpos(100);
    newdocument.add_term(SWISH_PREFIX_MTIME + string((const char*)parser_data->docinfo->mime));
    newdocument.add_term(unique_id);

    struct tm *tm = localtime(&(parser_data->docinfo->mtime));
    string date_term = "D" + date_to_string(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    newdocument.add_term(date_term); // Date (YYYYMMDD)
    date_term.resize(7);
    date_term[0] = 'M';
    newdocument.add_term(date_term); // Month (YYYYMM)
    date_term.resize(5);
    date_term[0] = 'Y';
    newdocument.add_term(date_term); // Year (YYYY)

    // Add last_mod as a value to allow "sort by date".
    newdocument.add_value(SWISH_PROP_LAST_MOD, 
        int_to_binary_string((uint32_t)parser_data->docinfo->mtime));
    
    // add all metanames and properties
    xmlHashScan(parser_data->metanames->hash,  (xmlHashScanner)add_metanames,  s3->config);
    xmlHashScan(parser_data->properties->hash, (xmlHashScanner)add_properties, &newdocument);

    if (!skip_duplicates) {
	    // If this document has already been indexed, update the existing
	    // entry.
	    try {      
	        Xapian::docid did = wdb.replace_document(unique_id, newdocument);
	        if (did < updated.size()) {
		        updated[did] = true;
		        cout << "updated." << endl;
	        } else {
		        cout << "added." << endl;
	        }
	    } catch (...) {
	        // FIXME: is this ever actually needed?
	        wdb.add_document(newdocument);
	        cout << "added (failed re-seek for duplicate)." << endl;
	    }
    } else {
	    // If this were a duplicate, we'd have skipped it above.
	    wdb.add_document(newdocument);
	    cout << "added." << endl;
    }
}

int open_writeable_index( char* dbpath )
{
    int exitcode = 1;
    try {
	if (!overwrite) {
	    wdb = Xapian::WritableDatabase(dbpath, Xapian::DB_CREATE_OR_OPEN);
	    if (!skip_duplicates) {
		// + 1 so that wdb.get_lastdocid() is a valid subscript.
		updated.resize(wdb.get_lastdocid() + 1);
	    }
	} else {
	    wdb = Xapian::WritableDatabase(dbpath, Xapian::DB_CREATE_OR_OVERWRITE);
	}

	indexer.set_stemmer(stemmer);

	wdb.flush();
    
	// cout << "\n\nNow we have " << wdb.get_doccount() << " documents.\n";
	exitcode = 0;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }
    
    return exitcode;
}

int open_readable_index(char* dbpath)
{
    int exitcode = 1;
    try {
        rdb = Xapian::Database::Database(dbpath);
    
        exitcode = 0;
    } catch (const Xapian::Error &e) {
	cout << "Exception: " << e.get_msg() << endl;
    } catch (const string &s) {
	cout << "Exception: " << s << endl;
    } catch (const char *s) {
	cout << "Exception: " << s << endl;
    } catch (...) {
	cout << "Caught unknown exception" << endl;
    }
    
    return exitcode;

}

int do_search(char* query)
{

}

int 
usage()
{

    char * descr = "swish_xapian is an example program for using libswish3 with Xapian\n";
    printf("swish_xapian [opts] [- | file(s)]\n");
    printf("opts:\n --config conf_file.xml\n --debug [lvl]\n --help\n");
    printf(" --index path/to/index\n --skip-duplicates\n --overwrite\n");
    printf("\n%s\n", descr);
    exit(0);
}

int 
main(int argc, char **argv)
{
    int             i, ch;
    extern char    *optarg;
    extern int      optind;
    int             option_index;
    int             files;
    char           *etime;
    char           *query;
    char           *dbpath;
    double          start_time;
    xmlChar        *config_file = NULL;
    
    option_index    = 0;
    files           = 0;
    query           = NULL;
    dbpath          = NULL;
    start_time      = swish_time_elapsed();
    s3              = swish_init_swish3( &handler, NULL );

    while ((ch = getopt_long(argc, argv, "c:d:f:i:q:soh", longopts, &option_index)) != -1)
    {
        /* printf("switch is %c\n",   ch); */
        /* printf("optarg is %s\n", optarg); */
        /* printf("optind = %d\n",  optind); */

        switch (ch)
        {
        case 0:    /* If this option set a flag, do nothing else now. */
            if (longopts[option_index].flag != 0)
                break;
            printf("option %s", longopts[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'c':    /* should we set up default config first ? then override
                 * here ? */

            //printf("optarg = %s\n", optarg);
            config_file = swish_xstrdup( (xmlChar*)optarg );
            break;
            

        case 'd':
            printf("turning on debug mode: %s\n", optarg);

            if (!isdigit(optarg[0]))
                err(1, "-d option requires a positive integer as argument\n");

            SWISH_DEBUG = (int) strtol(optarg, (char **) NULL, 10);
            break;


        case 'o':
            overwrite = 1;
            break;
            
        case 'i':
            dbpath = (char*)swish_xstrdup( (xmlChar*)optarg );
            break;
            
        case 's':
            skip_duplicates = (int) strtol(optarg, (char **) NULL, 10);
            break;
            
        case 'q':
            query = (char*)swish_xstrdup( (xmlChar*)optarg );

        case '?':
        case 'h':
        default:
            usage();

        }

    }
    
    if (config_file != NULL)
    {
        s3->config = swish_add_config(config_file, s3->config);
    }

    i = optind;
    
    /* die with no args */
    if (!i || i >= argc)
    {
        swish_free_swish3( s3 );
        usage();

    }
    
    if (SWISH_DEBUG == 20)
    {
        swish_debug_config(s3->config);   
    }
    
    if (!dbpath) {
        dbpath = (char*)swish_xstrdup((xmlChar*)SWISH_INDEX_FILENAME);
    }
    
    if (!query) {
        open_writeable_index(dbpath);
    }
    else {
        open_readable_index(dbpath);
    }
    
    // indexing mode
    if (!query) {   
        for (; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                printf("parse_file for %s\n", argv[i]);
                if (! swish_parse_file(s3, (unsigned char *) argv[i]))
                    files++;

            }
            else if (argv[i][0] == '-' && !argv[i][1])
            {
                printf("reading from stdin\n");
                files = swish_parse_fh(s3, NULL);
            }

        }
        
        printf("\n\n%d files indexed\n", files);
        printf("total words: %d\n", twords);
        
        // TODO write header
        
    }
    
    // searching mode
    else {
        do_search(query);
    }


    etime = swish_print_time(swish_time_elapsed() - start_time);
    printf("%s total time\n\n", etime);
    swish_xfree(etime);
    swish_xfree(dbpath);
    swish_free_swish3( s3 );

    if (config_file != NULL)
        swish_xfree(config_file);
    
    return (0);
}
