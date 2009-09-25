CC=$(CXX)
AM_CFLAGS= -Wall `@XML2_CONFIG@ --cflags`
INCLUDES=`xapian-config --cxxflags` `@XML2_CONFIG@ --cflags` -I$(top_srcdir)/src/libswish3
AM_LDFLAGS=`xapian-config --libs` `@XML2_CONFIG@ --libs` -L$(top_srcdir)/src/libswish3 -lswish3

bin_PROGRAMS = swish_xapian
check_PROGRAMS = swish_xapian
swish_xapian_SOURCES = swish_xapian.cpp

TESTS = $(check_PROGRAMS) test.pl
CLEAN = index.swish

test: check
