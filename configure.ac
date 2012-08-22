if [ ! $SVNDIR ]; then
    SVNDIR="."
fi
define([svnversion], esyscmd([sh -c "svnversion $SVNDIR | tr -d '\n'"]))
AC_INIT([libswish3], [1.0.svnversion], [Swish-e Users List <users@lists.swish-e.org>], [http://swish-e.org/swish3/])
AM_CONFIG_HEADER(src/acconfig.h)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(config)
AC_SUBST(ac_aux_dir)
AC_CONFIG_FILES([
                Makefile
                src/Makefile
                src/libswish3/Makefile
                src/xapian/Makefile
                doc/Makefile
                libswish3.pc
                libswish3-config
                ])
PACKAGE=libswish3
dnl version number
MAJOR_VERSION=1
MINOR_VERSION=0
MICRO_VERSION=svnversion
INTERFACE_AGE=0
BINARY_AGE=0
VERSION=$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION
AC_DEFINE(SVN_REVISION, "svnversion", [SVN Revision])

dnl ########################################################################
dnl Checks for zlib library. -- from swish-e configure.in
_cppflags="${CPPFLAGS}"
_ldflags="${LDFLAGS}"

dnl AC_ARG_WITH(zlib,AC_HELP_STRING([--with-zlib=DIR], [use zlib in DIR (YES if found)]),,withval=maybe)

AC_ARG_WITH(zlib,
[  --with-zlib[[=DIR]]       use libz in DIR],[
  if test "$withval" != "no" -a "$withval" != "yes"; then
    Z_DIR=$withval
    CPPFLAGS="${CPPFLAGS} -I$withval/include"
    LDFLAGS="${LDFLAGS} -L$withval/lib"
  fi
])
if test "$with_zlib" = "no"; then
    AC_MSG_ERROR([libz required])
else
    AC_CHECK_HEADERS(zlib.h,
        AC_CHECK_LIB(z, gzread,[
            AC_DEFINE(HAVE_ZLIB,[],[Do we have zlib])
            if test "x${Z_DIR}" != "x"; then
                Z_CFLAGS="-I${Z_DIR}/include"
                Z_LIBS="-L${Z_DIR}/lib -lz"
                [case ${host} in
                    *-*-solaris*)
                        Z_LIBS="-L${Z_DIR}/lib -R${Z_DIR}/lib -lz"
                        ;;
                esac]
            else
                Z_LIBS="-lz"
            fi]))
fi

dnl mingw build requires this for win32
if test "x${target}" == "xi586-mingw32msvc"; then
     Z_LIBS="-L${Z_DIR}/lib -R${Z_DIR}/lib -lzdll"
fi

dnl echo "Z_LIBS = $Z_LIBS"


AC_SUBST(Z_CFLAGS)
AC_SUBST(Z_LIBS)


CPPFLAGS=${_cppflags}
LDFLAGS=${_ldflags}

dnl #######################################################################
dnl libxml2 required to build

LIBXML_REQUIRED_VERSION=2.6.0
AC_ARG_WITH(libxml2,AC_HELP_STRING([--with-libxml2=DIR],[DIR where xml2-config resides]))
if test "$withval" != ""; then
  XML2_CONFIG_PATH=$withval;
else
  XML2_CONFIG_PATH="$PATH"
fi

dnl AC_MSG_CHECKING(xml2-config in $XML2_CONFIG_PATH)

AC_PATH_PROG(XML2_CONFIG, xml2-config,"no", $XML2_CONFIG_PATH)                
if test "$XML2_CONFIG" = "no"; then
    withval="no"
else
    withval=`$XML2_CONFIG --prefix`
fi

if test "$withval" = "no"; then
    AC_MSG_ERROR([libxml2 required])
else

    AC_SUBST(LIBXML_REQUIRED_VERSION)
    AC_MSG_CHECKING(for libxml2 libraries >= $LIBXML_REQUIRED_VERSION)

    AC_DEFUN([VERSION_TO_NUMBER],
    [`$1 | sed -e 's/libxml //' | awk 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`])

    vers=VERSION_TO_NUMBER($XML2_CONFIG --version)
    XML2_VERSION=`$XML2_CONFIG --version`

    if test "$vers" -ge VERSION_TO_NUMBER(echo $LIBXML_REQUIRED_VERSION);then
        AC_MSG_RESULT(found version $XML2_VERSION)
    else
        AC_MSG_ERROR(You need at least libxml2 $LIBXML_REQUIRED_VERSION for this version of libswish3)
    fi


    AC_DEFINE(HAVE_LIBXML2,[],[Libxml2 support included])
    AC_SUBST([XML2_CONFIG])
fi

dnl ############################################################################
dnl should we build the xapian app
withval=""
XAPIAN_REQUIRED_VERSION=1.0.0
AC_ARG_WITH(xapian,AC_HELP_STRING([--with-xapian=DIR],[DIR where xapian-config resides]))
if test "$withval" != ""; then
dnl  AC_MSG_RESULT("using path at $withval");
  XAPIAN_CONFIG_PATH=$withval;
else
  XAPIAN_CONFIG_PATH="$PATH"
fi

dnl AC_MSG_CHECKING(xapian-config in $XAPIAN_CONFIG_PATH)

AC_PATH_PROG(XAPIAN_CONFIG, xapian-config, "no", $XAPIAN_CONFIG_PATH)                
if test "$XAPIAN_CONFIG" = "no"; then
    withval="no"
fi

we_have_xapian=no
if test "$withval" = "no"; then
    AC_MSG_RESULT(Skipping swish_xapian build.)
else

    AC_SUBST(XAPIAN_REQUIRED_VERSION)
    AC_MSG_CHECKING(for xapian libraries >= $XAPIAN_REQUIRED_VERSION)

    AC_DEFUN([XAP_VERSION_TO_NUMBER], 
    [`$1 | awk 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`])
    
    XAPIAN_VERSION=`$XAPIAN_CONFIG --version | sed -e 's/.*xapian-core //'`
    vers=XAP_VERSION_TO_NUMBER(echo $XAPIAN_VERSION)
    
    XAPIAN_REQ_VERSION_INT=XAP_VERSION_TO_NUMBER(echo $XAPIAN_REQUIRED_VERSION);
dnl    AC_MSG_RESULT(xapian version looks like $XAPIAN_VERSION ($vers) need $XAPIAN_REQ_VERSION_INT);

    if test "$vers" -ge "$XAPIAN_REQ_VERSION_INT";then
        AC_MSG_RESULT(found version $XAPIAN_VERSION)
    else
        AC_MSG_ERROR(You need at least xapian $XAPIAN_REQUIRED_VERSION for this version of libswish3 and you have $XAPIAN_VERSION)
    fi

    AC_DEFINE(HAVE_XAPIAN,[],[xapian support included])
    we_have_xapian=yes
    AC_SUBST([XAPIAN_CONFIG])
    AC_SUBST([XAPIAN_VERSION])
fi
dnl this conditional lets us include the xapian stuff in dist without requiring it to be built.
AM_CONDITIONAL([HAVE_XAPIAN], [test "$we_have_xapian" = yes])

dnl ###############################################################################################
dnl Check pod2man for creating man pages
AC_CHECK_PROG([POD2MAN], [pod2man], [pod2man], [false])
if test "$POD2MAN" = "false"; then
    dnl disable building of man pages?
    AC_MSG_WARN([pod2man was not found - needed for building man pages])
fi

dnl ##############################################################################################
dnl 
AC_ARG_ENABLE(daystamp,
             AC_HELP_STRING([--enable-daystamp], [Adds today's date to version]),
             daystamp=yes,)

if test x$daystamp = xyes; then
        TODAY=`/bin/date +%Y%m%d`
        VERSION="$VERSION-$TODAY"
fi
LIBSWISH3_VERSION=$VERSION
AC_SUBST(LIBSWISH3_VERSION)
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
                
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_OUTPUT
