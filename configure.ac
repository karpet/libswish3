AC_INIT([libswish3], [0.1.0],
        [Peter Karman <peter@peknet.com>],
        [libswish3])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(src/acconfig.h)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([
                Makefile
                src/Makefile
                src/libswish3/Makefile
                src/xapian/Makefile
                doc/Makefile
                libswish3.pc
                libswish3-config
                ])
                
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
    AC_SUBST([XAPIAN_CONFIG])
fi
dnl this conditional lets us include the xapian stuff in dist without requiring it to be built.
AM_CONDITIONAL([HAVE_XAPIAN], [HAVE_XAPIAN])

dnl Check pod2man for creating man pages
AC_CHECK_PROG([POD2MAN], [pod2man], [pod2man], [false])
if test "$POD2MAN" = "false"; then
    dnl disable building of man pages?
    AC_MSG_WARN([pod2man was not found - needed for building man pages])
fi

                
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_OUTPUT
