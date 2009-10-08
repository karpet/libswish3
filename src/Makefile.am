if HAVE_XAPIAN
    SRC_XAPIAN = xapian
endif
SUBDIRS = libswish3 $(SRC_XAPIAN)

xml2_CFLAGS=`@XML2_CONFIG@ --cflags`
xml2_ld=`@XML2_CONFIG@ --libs`
myheaders=libswish3/libswish3.h
INCLUDES=-I$(top_srcdir)/src -I. -I$(top_srcdir)/src/libswish3
AM_LDFLAGS= -I$(top_srcdir)/src -I. -I./libswish3 -L$(top_srcdir)/src/libswish3 -L./libswish3 -lswish3 $(xml2_ld) 
AM_CFLAGS= -Wall $(xml2_CFLAGS) -g 
#CFLAGS= -Wall $(xml2_CFLAGS) -g -pg
# -pg is for profiling -- don't use in production
LIBSWISH3_VERSION = @LIBSWISH3_VERSION@

bin_PROGRAMS = swish_lint swish_tokenize swish_isw utf8test swish_header
check_PROGRAMS = swish_lint swish_header
swish_lint_SOURCES = swish_lint.c $(myheaders)
swish_tokenize_SOURCES = swish_tokenize.c $(myheaders)
swish_isw_SOURCES = swish_isw.c
swish_header_SOURCES = swish_header.c $(myheaders)
utf8test_SOURCES = utf8test.c $(myheaders)

tests_dir = $(top_srcdir)/t/ \
	$(top_srcdir)/t/001-wordcount.t \
	$(top_srcdir)/t/002-header.t \
	$(top_srcdir)/t/003-properties.t \
	$(top_srcdir)/t/004-metanames.t \
	$(top_srcdir)/t/005-tokenizer.t \
	$(top_srcdir)/t/SwishTestUtils.pm

test_doc_dir = $(top_srcdir)/test_docs/ \
	$(top_srcdir)/test_docs/UPPERlower.XML \
	$(top_srcdir)/test_docs/UTF-8-demo.txt \
	$(top_srcdir)/test_docs/badxml.xml \
	$(top_srcdir)/test_docs/contractions.xml \
	$(top_srcdir)/test_docs/dom.xml \
	$(top_srcdir)/test_docs/empty.pdf \
	$(top_srcdir)/test_docs/empty_doc.html \
	$(top_srcdir)/test_docs/foo.txt \
	$(top_srcdir)/test_docs/greek_and_ojibwe.txt \
	$(top_srcdir)/test_docs/has_nulls.txt \
	$(top_srcdir)/test_docs/html_broken.html \
	$(top_srcdir)/test_docs/inline.html \
	$(top_srcdir)/test_docs/inline.xml \
	$(top_srcdir)/test_docs/latin1.txt \
	$(top_srcdir)/test_docs/latin1.xml \
	$(top_srcdir)/test_docs/meta.html \
	$(top_srcdir)/test_docs/min.txt \
	$(top_srcdir)/test_docs/multi_props.xml \
	$(top_srcdir)/test_docs/nested_meta.xml \
	$(top_srcdir)/test_docs/no_words.html \
	$(top_srcdir)/test_docs/properties.html \
	$(top_srcdir)/test_docs/quickbrown.txt \
	$(top_srcdir)/test_docs/revelation.txt \
	$(top_srcdir)/test_docs/t.html \
	$(top_srcdir)/test_docs/testutf.xml \
	$(top_srcdir)/test_docs/utf.xml \
	$(top_srcdir)/test_docs/words.txt \
	$(top_srcdir)/test_docs/words.xml 

test_stdin_dir = $(top_srcdir)/test_stdin \
	$(top_srcdir)/test_stdin/doc.xml
	$(top_srcdir)/test_stdin/test.txt

test_configs_dir = $(top_srcdir)/test_configs/ \
	$(top_srcdir)/test_configs/UPPERlower.XML \
	$(top_srcdir)/test_configs/dom.conf \
	$(top_srcdir)/test_configs/meta.xml \
	$(top_srcdir)/test_configs/properties.xml \
        $(top_srcdir)/test_configs/swish.xml

EXTRA_DIST = test.sh  $(tests_dir) $(test_doc_dir) $(test_stdin_dir) $(test_configs_dir) 

TESTS = $(check_PROGRAMS) test.sh

test: check
