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

tests_dir = t \
	t/001-wordcount.t \
	t/002-header.t \
	t/003-properties.t \
	t/004-metanames.t \
	t/005-tokenizer.t \
	t/SwishTestUtils.pm

test_doc_dir = test_docs \
	test_docs/UPPERlower.XML \
	test_docs/UTF-8-demo.txt \
	test_docs/badxml.xml \
	test_docs/contractions.xml \
	test_docs/dom.xml \
	test_docs/empty.pdf \
	test_docs/empty_doc.html \
	test_docs/foo.txt \
	test_docs/greek_and_ojibwe.txt \
	test_docs/has_nulls.txt \
	test_docs/html_broken.html \
	test_docs/inline.html \
	test_docs/inline.xml \
	test_docs/latin1.txt \
	test_docs/latin1.xml \
	test_docs/meta.html \
	test_docs/min.txt \
	test_docs/multi_props.xml \
	test_docs/nested_meta.xml \
	test_docs/no_words.html \
	test_docs/properties.html \
	test_docs/quickbrown.txt \
	test_docs/revelation.txt \
	test_docs/t.html \
	test_docs/testutf.xml \
	test_docs/utf.xml \
	test_docs/words.txt \
	test_docs/words.xml 

test_stdin_dir = test_stdin \
	test_stdin/doc.xml \
	test_stdin/test.txt

test_configs_dir = test_configs \
	test_configs/UPPERlower.XML \
	test_configs/dom.conf \
	test_configs/meta.xml \
	test_configs/properties.xml \
        test_configs/swish.xml

EXTRA_DIST = test.sh  $(tests_dir) $(test_doc_dir) $(test_stdin_dir) $(test_configs_dir) 

TESTS = $(check_PROGRAMS) test.sh

.PHONEY: test

test: check
