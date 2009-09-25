xml2_CFLAGS=`@XML2_CONFIG@ --cflags`
xml2_ld=`@XML2_CONFIG@ --libs`
INCLUDES=-I$(top_srcdir)/src

AM_CFLAGS= -Wall $(xml2_CFLAGS) -pg

check_PROGRAMS = 
myheaders =  libswish3.h
		
lib_LTLIBRARIES = libswish3.la
libswish3_la_LDFLAGS = -no-undefined -version-info 1:0:0 $(xml2_ld) -pg
include_HEADERS = libswish3.h

EXTRA_DIST = getruntime.c getruntime.h

libswish3_la_SOURCES =  config.c \
                        docinfo.c \
                        error.c \
                        hash.c \
                        io.c \
                        mem.c \
                        mime_types.c \
                        parser.c \
                        namedbuffer.c \
                        string.c \
                        times.c \
                        swish.c \
                        analyzer.c \
                        property.c \
                        metaname.c \
                        header.c \
                        tokenizer.c \
                        $(myheaders) 

