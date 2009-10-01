AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
if HAVE_XAPIAN
    SRC_XAPIAN = src/xapian
endif
SUBDIRS = src doc $(SRC_XAPIAN)

test: check

bin_SCRIPTS = libswish3-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libswish3.pc
