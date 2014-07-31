#!/usr/bin/env perl
use strict;
use warnings;
use File::Slurp;

my $src_dir   = 'src/libswish3';
my $dest_file = 'src/libswish3/libswish3.c';
chomp( my $svn_rev = `git log --oneline -n 1 .` );
#print "svn_rev == $svn_rev";
$svn_rev =~ s/^(\w+).*/$1/;
my $version = '1.0.' . $svn_rev;

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
/*
 * This file is part of libswish3
 * Copyright (C) 2010 Peter Karman
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
 /**
  * This file is automatically generated from the individual src .c and .h files
  * that are part of the libswish3 distribution. The guiding purpose of this file
  * is to allow for easier distribution in language bindings. See the
  * bindings/perl/3.xs file in the libswish3 distribution for one example.
  */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# ifndef HAVE_ALLOCA
#  ifdef  __cplusplus
extern "C"
#  endif
void *alloca (size_t);
# endif
#endif

#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include <assert.h>
#include <wchar.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>
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
#include <libxml/xinclude.h>
#include <libxml/uri.h>


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
print "File written to $dest_file\n";

