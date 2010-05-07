#!/usr/bin/env perl
use strict;
use warnings;
use File::Slurp;

my $src_dir   = 'src/libswish3';
my $dest_file = 'src/libswish3/libswish3.c';
chomp( my $svn_rev = `svnversion .` );
$svn_rev =~ s/:\d+$//;
my $version = '0.1.' . $svn_rev;

my @files = qw(
    libswish3.h
    getruntime.h
    getruntime.c
    utf8.c
    config.c
    docinfo.c
    error.c
    hash.c
    fs.c
    io.c
    mem.c
    mime_types.c
    parser.c
    namedbuffer.c
    string.c
    times.c
    swish.c
    analyzer.c
    property.c
    metaname.c
    header.c
    tokenizer.c

);

my $includes = <<EOF;

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
/* #include <err.h> *//* conflicts with Perl err.h ? */
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <dirent.h>
#include <time.h>

#if defined (HAVE_GETRUSAGE) && defined (HAVE_SYS_RESOURCE_H)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef HAVE_TIMES
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/times.h>
#endif

#include <zlib.h>

#include <libxml/parserInternals.h>
#include <libxml/parser.h>
#include <libxml/hash.h>
#include <libxml/xmlstring.h>
#include <libxml/HTMLparser.h>
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/encoding.h>


#define LIBSWISH3_SINGLE_FILE 1

EOF

my @buf = ($includes);

for my $file (@files) {
    my $c = read_file("$src_dir/$file");
    if ( $file eq 'libswish3.h' ) {
        $c =~ s/ VERSION\n/ "$version"\n/s;
    }

    push @buf, "\n\n/*************** start $file ************/\n";
    push @buf, $c;
    push @buf, "\n\n/*************** end $file ************/\n";
}

write_file( $dest_file, @buf );

