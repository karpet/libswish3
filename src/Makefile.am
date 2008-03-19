SUBDIRS = libswish3

# do not list xapian as a SUBDIR because it is optional to build xapian/*

xml2_CFLAGS=`xml2-config --cflags`
xml2_ld=`xml2-config --libs`
myheaders=libswish3/libswish3.h
INCLUDES=-I$(top_srcdir)/src -I. -I$(top_srcdir)/src/libswish3
LDFLAGS=$(xml2_ld) -I$(top_srcdir)/src -L$(top_srcdir)/src/libswish3 -lswish3
CFLAGS= -Wall $(xml2_CFLAGS) -g -pg
# -pg is for profiling -- don't use in production

bin_PROGRAMS = swish_lint swish_words swish_isw utf8test swish_header
check_PROGRAMS = swish_lint
swish_lint_SOURCES = swish_lint.c $(myheaders)
swish_words_SOURCES = swish_words.c $(myheaders)
swish_isw_SOURCES = swish_isw.c
swish_header_SOURCES = swish_header.c $(myheaders)
utf8test_SOURCES = utf8test.c $(myheaders)

TESTS = $(check_PROGRAMS) test.pl

test: check
