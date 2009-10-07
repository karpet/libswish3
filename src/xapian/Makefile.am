CC=$(CXX)
AM_CFLAGS= -Wall `@XML2_CONFIG@ --cflags`
myheaders=libswish3/libswish3.h
INCLUDES=-I$(top_srcdir)/src -I. -I$(top_srcdir)/src/libswish3 `@XAPIAN_CONFIG@ --cxxflags` `@XML2_CONFIG@ --cflags` 
AM_LDFLAGS=-L$(top_srcdir)/src/libswish3 -lswish3 `@XAPIAN_CONFIG@ --libs` `@XML2_CONFIG@ --libs` 

XAPIAN_VERSION = @XAPIAN_VERSION@

bin_PROGRAMS = swish_xapian
check_PROGRAMS = swish_xapian
swish_xapian_SOURCES = swish_xapian.cpp $(myheaders)

TESTS = $(check_PROGRAMS) test.pl
CLEAN = index.swish

test: check
