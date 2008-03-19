CC=$(CXX)
CFLAGS= -Wall `xml2-config --cflags`
INCLUDES=`xapian-config --cxxflags` `xml2-config --cflags` -I$(top_srcdir)/src/libswish3
LDFLAGS=`xapian-config --libs` `xml2-config --libs` -L$(top_srcdir)/src/libswish3 -lswish3

bin_PROGRAMS = swish_xapian
check_PROGRAMS = swish_xapian
swish_xapian_SOURCES = swish_xapian.cpp

TESTS = $(check_PROGRAMS) test.pl
CLEAN = index.swish3

test: check
