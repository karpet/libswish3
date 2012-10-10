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

/* swish_lint.c -- test libswish3 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <err.h>
#include <string.h>
#include <wctype.h>
#include <ctype.h>
#include <getopt.h>
#include "acconfig.h"
#include "libswish3.h"

int debug = 0;
int verbose = 0;

int main(
    int argc,
    char **argv
);
void usage(
);
void handler(
    swish_ParserData *parser_data
);
void libxml2_version(
);
void swish_version(
);

int twords = 0;

extern int SWISH_DEBUG;

static struct option longopts[] = {
    {"config", required_argument, 0, 'c'},
    {"CascadeMetaContext", required_argument, 0, 'C'},
    {"debug", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, 0, 'v'},
    {"filelist", required_argument, 0, 'f'},
    {"tokenize", required_argument, 0, 't'},
    {"xinclude", required_argument, 0, 'X'},
    {"xmlns", required_argument, 0, 'x'},
    {0, 0, 0, 0}
};

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
usage(
)
{

    char *descr = "swish_lint is an example program for using libswish3\n";
    printf("swish_lint [opts] [- | file(s)]\n");
    printf("opts:\n --config conf_file.xml\n --debug [lvl]\n --help\n --verbose\n");
    printf(" --filelist filename\n");
    printf(" --tokenize 0|1\n");
    printf(" --xinclude 0|1\n");
    printf(" --xmlns 0|1\n");
    printf(" --CascadeMetaContext 0|1\n");
    printf("\n%s\n", descr);
    printf("Debugging env vars:\n");
    printf("\tSWISH_DEBUG <-- takes sum of ints below\n");
    printf("\tSWISH_DEBUG_DOCINFO      1\n");
    printf("\tSWISH_DEBUG_TOKENIZER    2\n");
    printf("\tSWISH_DEBUG_TOKENLIST    4\n");
    printf("\tSWISH_DEBUG_PARSER       8\n");
    printf("\tSWISH_DEBUG_CONFIG      16\n");
    printf("\tSWISH_DEBUG_MEMORY      32\n");
    printf("\tSWISH_DEBUG_NAMEDBUFFER 64\n");
    printf("\tSWISH_DEBUG_IO         128\n");
    printf("Set SWISH_PARSER_WARNINGS=0 to turn off libxml2 errors and warnings\n");
    printf("Set SWISH_WARNINGS=0 to turn off libswish3 warnings\n");
    printf("stdin headers:\n");
    printf("\tContent-Length\n");
    printf("\tLast-Modified\n");
    printf("\tContent-Location\n");
    printf("\tParser-Type\n");
    printf("\tContent-Type\n");
    printf("\tEncoding\n");
    printf("\tUpdate-Mode\n");
    printf("\n");
    printf("Default PropertyNames (with unique id):\n");
    printf("%20s %d\n", SWISH_PROP_DOCID, SWISH_PROP_DOCID_ID);
    printf("%20s %d\n", SWISH_PROP_DOCPATH, SWISH_PROP_DOCPATH_ID);
    printf("%20s %d\n", SWISH_PROP_DBFILE, SWISH_PROP_DBFILE_ID);
    printf("%20s %d\n", SWISH_PROP_TITLE, SWISH_PROP_TITLE_ID);
    printf("%20s %d\n", SWISH_PROP_SIZE, SWISH_PROP_SIZE_ID);
    printf("%20s %d\n", SWISH_PROP_MTIME, SWISH_PROP_MTIME_ID);
    printf("%20s %d\n", SWISH_PROP_DESCRIPTION, SWISH_PROP_DESCRIPTION_ID);
    printf("%20s %d\n", SWISH_PROP_MIME, SWISH_PROP_MIME_ID);
    printf("%20s %d\n", SWISH_PROP_NWORDS, SWISH_PROP_NWORDS_ID);
    printf("%20s %d\n", SWISH_PROP_PARSER, SWISH_PROP_PARSER_ID);
    printf("%20s %d\n", SWISH_PROP_ENCODING, SWISH_PROP_ENCODING_ID);
    printf("\n");
    libxml2_version();
    swish_version();

}

void
handler(
    swish_ParserData *parser_data
)
{

/*
       return;
*/

    if (verbose) {
        printf("nwords: %d\n", parser_data->docinfo->nwords);
    }

    if (SWISH_DEBUG & SWISH_DEBUG_MEMORY)
        swish_mem_debug();

    twords += parser_data->docinfo->nwords;

    if (debug || (SWISH_DEBUG & SWISH_DEBUG_DOCINFO))
        swish_docinfo_debug(parser_data->docinfo);

    if (debug || (SWISH_DEBUG & SWISH_DEBUG_TOKENLIST)) {
        swish_token_list_debug(parser_data->token_iterator);
    }

    if (debug || (SWISH_DEBUG & SWISH_DEBUG_NAMEDBUFFER)) {
        swish_nb_debug(parser_data->properties, (xmlChar *)"Property");
        swish_nb_debug(parser_data->metanames, (xmlChar *)"MetaName");
    }
}

int
main(
    int argc,
    char **argv
)
{
    int i, ch;
    extern char *optarg;
    extern int optind;
    int option_index;
    long int files;
    char *etime;
    double start_time;
    long int num_lines;
    xmlChar *filelist = NULL;
    xmlChar *config_file = NULL;
    xmlChar line_in_file[SWISH_MAXSTRLEN];
    FILE *filehandle = NULL;
    swish_3 *s3;

    swish_setup();  /*  always call first */
    option_index = 0;
    files = 0;
    start_time = swish_time_elapsed();
    s3 = swish_3_init(&handler, NULL);

    while ((ch = getopt_long(argc, argv, "c:d:f:ht:vx:X:C:", longopts, &option_index)) != -1) {

        switch (ch) {
            case 0:                /* If this option set a flag, do nothing else now. */
                if (longopts[option_index].flag != 0)
                    break;
                printf("option %s", longopts[option_index].name);
                if (optarg)
                    printf(" with arg %s", optarg);
                printf("\n");
                break;

            case 'c':              /* should we set up default config first ? then override
* here ? */

/* printf("optarg = %s\n", optarg); */
                config_file = swish_xstrdup((xmlChar *)optarg);
                s3->config = swish_config_add(s3->config, config_file);
                break;

            case 'd':
                printf("turning on debug mode: %s\n", optarg);

                if (!isdigit(optarg[0]))
                    err(1, "-d option requires a positive integer as argument\n");

                SWISH_DEBUG = swish_string_to_int(optarg);
                debug = 1;
                break;

            case 'v':
                verbose = 1;
                s3->parser->verbosity = 1;
                break;

            case 'f':
                filelist = swish_xstrdup((xmlChar *)optarg);
                break;

            case 't':
                s3->analyzer->tokenize = swish_string_to_boolean(optarg);
                break;

            case 'x':
                s3->config->flags->ignore_xmlns = swish_string_to_boolean(optarg);
                break;

            case 'X':
                s3->config->flags->follow_xinclude = swish_string_to_boolean(optarg);
                break;

            case 'C':
                s3->config->flags->cascade_meta_context = swish_string_to_boolean(optarg);
                break;

            case '?':
            case 'h':
            default:
                usage();
                exit(0);

        }

    }

    i = optind;

    if ((!i || i >= argc) && !filelist) {
        usage();
    }
    else {
        if (SWISH_DEBUG & SWISH_DEBUG_CONFIG) {
            swish_config_debug(s3->config);
        }

        for (; i < argc; i++) {

            if (argv[i][0] != '-') {

/* printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"); */
                if (swish_fs_is_file(BAD_CAST argv[i])) {
                    printf("parse_file for %s\n", argv[i]);
                    if (swish_parse_file(s3, BAD_CAST argv[i]))
                        files++;
                }
                else if (swish_fs_is_dir(BAD_CAST argv[i])) {
                    printf("parse_directory for %s\n", argv[i]);
                    files += swish_parse_directory(s3, BAD_CAST argv[i], 1);
                }
                else {
                    SWISH_CROAK("'%s' is neither a file nor a directory", argv[i]);
                }
            }
            else if (argv[i][0] == '-' && !argv[i][1]) {

                printf("reading from stdin\n");
                files = swish_parse_fh(s3, NULL);

            }

        }

        if (filelist) {

            num_lines = swish_io_count_operable_file_lines(filelist);
            printf("%ld valid file names in filelist %s\n", num_lines, filelist);

/* open file and treat each line as a file name */
            filehandle = fopen((const char*)filelist, "r");
            if (filehandle == NULL) {
                SWISH_CROAK("failed to open filelist %s", filelist);
            }
            while (fgets((char*)line_in_file, SWISH_MAXSTRLEN, filehandle) != NULL) {

                xmlChar *end;
                xmlChar *line;

                line = swish_str_skip_ws(line_in_file);   /* skip leading white space */
                if (swish_io_is_skippable_line(line))
                    continue;

                end = (xmlChar *)strrchr((char *)line, '\n');

/* trim any white space at end of doc, including \n */
                if (end) {
                    while (end > line && isspace((int) * (end - 1)))
                        end--;

*end = '\0';
                }

                if (verbose) {
                    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
                    printf("parse_file for %s\n", line);
                }
                if (!swish_parse_file(s3, (unsigned char *)line)) {
                    files++;
                    if (!verbose) {
                        double per = ((double)files / (double)num_lines);
                        printf("\r%10ld of %10ld files (%02.1f%%)",
                               files, num_lines, per * 100);
                    }
                }

            }

            if (fclose(filehandle)) {
                SWISH_CROAK("error closing filelist");
            }

        }

        printf("\n\n%ld files parsed\n", files);
        printf("total words: %d\n", twords);

        etime = swish_time_print(swish_time_elapsed() - start_time);
        printf("%s total time\n\n", etime);
        swish_xfree(etime);

    }

    if (config_file != NULL)
        swish_xfree(config_file);

    if (filelist != NULL)
        swish_xfree(filelist);

    swish_3_free(s3);
    swish_mem_debug();

    return (0);
}
