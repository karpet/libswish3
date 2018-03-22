AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src doc 

test: check

single:
	perl perl/cat-libswish3.pl

perl: single
	cp src/libswish3/libswish3.h bindings/perl/
	cp src/libswish3/libswish3.c bindings/perl/

perltest: perl
	cd bindings/perl && perl Makefile.PL && make test

testall: test perltest

bin_SCRIPTS = libswish3-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libswish3.pc
distdir = $(PACKAGE)-$(VERSION)

EXTRA_DIST = m4

dist-hook:
	rm -rf `find $(distdir) -name .svn`
