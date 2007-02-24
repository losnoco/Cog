dnl configure.in for lamip-0.0.3 shorten input plugin

AC_INIT(libinputshorten, 0.0.3)
AC_CONFIG_SRCDIR(src/libinputshorten.c)

AM_INIT_AUTOMAKE(inputSHORTEN, 0.0.3)
AM_CONFIG_HEADER(shorten/include/config.h)

dnl AC_INIT(src/libinputshorten.c)
dnl AM_INIT_AUTOMAKE(libinputshorten, 0.0.3)

AM_DISABLE_STATIC

dnl save CFLAGS since AC_PROG_CC insert "-g -O2" if CFLAGS is empty
cflags_save="$CFLAGS"
AC_PROG_CC
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_INSTALL
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_HEADER_STDC

AC_CHECK_LIB(m, main)

AC_HEADER_STDC
AC_CHECK_HEADERS(stdarg.h inttypes.h dirent.h)

AC_CHECK_SIZEOF(unsigned long)

AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(opendir readdir closedir strerror vsnprintf)

AC_CHECK_PROGS(LAMIP, lamip,
        AC_MSG_ERROR(*** lamip not found))

CFLAGS="$cflags_save -Wall `lamip --cflags`"
CPPFLAGS="$CPPFLAGS $CFLAGS"
AC_SUBST(CFLAGS)
cflags_save="$CFLAGS"

AC_CHECK_HEADER(lamip.h,,
	AC_MSG_ERROR(*** LAMIP headers not found check your CFLAGS))

CFLAGS="$cflags_save -I. -I.. -Ishorten/include"
cflags_save="$CFLAGS"
AC_SUBST(CFLAGS)

AC_ARG_ENABLE(debug,
[  --enable-debug (for developers only... and people with probs!) ],
[enable_debug=$enableval],
[enable_debug="no"])

if test "x$enable_debug" = xyes; then
	CPPFLAGS="$CPPFLAGS -DDEBUG"
	CFLAGS="$CFLAGS -g"
	AC_MSG_RESULT([*** Debugging is enabled... ])
fi

AC_OUTPUT([Makefile shorten/Makefile shorten/util/Makefile shorten/src/Makefile src/Makefile])

echo ""
echo "*** LAMIP inputSHORTEN (Shorten Codec) plugin succefully configured ***"
echo ""
echo "This plugin allows you to play *.shn files..."
echo ""
echo "install-dir : `lamip --plugin-dir`"
echo ""
echo "CFLAGS : $CFLAGS"
echo "LDFLAGS : $LDFLAGS"
echo "CPPFLAGS : $CPPFLAGS"
echo ""
if test "x$enable_debug" = xyes; then
	echo "- debugging messages are enabled!"
	echo ""
fi
