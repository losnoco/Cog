AC_INIT([libexsid], [1.2], [], [], [http://hacks.slashdirt.org/hw/exsid/#libexsid])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([exSID.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

dnl Initialize libtool.
LT_INIT([win32-dll])

PKG_PROG_PKG_CONFIG

AC_CANONICAL_HOST

dnl The dlopen() function is in the C library for *BSD and in
dnl libdl on GLIBC-based systems. Use native functions on MinGW.
AS_CASE([$host_os],
        [mingw*], [MINGW32=yes],
	[AC_SEARCH_LIBS([dlopen], [dl dld], [], [AC_MSG_ERROR([unable to find the dlopen() function])])]
)

AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])

# debug options
AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([Build without debugging messages]); debug_flags=-DNDEBUG],
  [AC_MSG_RESULT([Build with debugging messages]); debug_flags=-DDEBUG]
)

AC_SUBST([debug_flags])


# check for libraries

AC_CHECK_HEADERS([ftd2xx.h],
	[AC_DEFINE([HAVE_FTD2XX], 1, [Define to 1 if you have ftd2xx.h])],
	[AC_MSG_WARN([ftd2xx.h not found, support will not be available])]
	)

AC_MSG_CHECKING([for libftdi1])
PKG_CHECK_EXISTS([libftdi1 >= 1.0],
                [AC_DEFINE([HAVE_FTDI], 1, [Define to 1 if you have ftdi.h])
                FTDI_CFLAGS=`$PKG_CONFIG --cflags libftdi1`
                AC_MSG_RESULT([yes])],
                [AC_MSG_WARN([libftdi1 not found, support will not be available])]
	)
AC_SUBST([FTDI_CFLAGS])

eval loadable_shrext=$shrext_cmds
AC_DEFINE_UNQUOTED([SHLIBEXT], ["${loadable_shrext}"], [Shared library extension])

AC_CACHE_CHECK([for __builtin_expect], [exsid_cv_builtin_expect],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() { __builtin_expect(0, 0); }])],
    [exsid_cv_builtin_expect=yes], [exsid_cv_builtin_expect=no])]
)

AS_IF([test "$exsid_cv_builtin_expect" = yes],
  [AC_DEFINE([HAVE_BUILTIN_EXPECT], 1, [Define to 1 if you have builtin_expect.])]
)


# doxygen
AC_CHECK_PROGS([DOXYGEN], [doxygen])
AS_IF([test -z "$DOXYGEN"],
  [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])],
  [AC_CHECK_PROG([DOT], [dot], [YES], [NO])]
)

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])


dnl libtool-style version-info number
#
# https://autotools.io/libtool/version.html
#
# Always increase the revision value.
#
# Increase the current value whenever an interface has been added, removed or changed.
#
# Increase the age value only if the changes made to the ABI are backward compatible.

LIBEXSIDCUR=0
LIBEXSIDREV=0
LIBEXSIDAGE=0
LIBEXSIDVERSION=$LIBEXSIDCUR:$LIBEXSIDREV:$LIBEXSIDAGE

AC_SUBST(LIBEXSIDVERSION)

AC_CONFIG_FILES([
Makefile
libexsid.pc
])

AC_OUTPUT
