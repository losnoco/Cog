m4_define([lib_major], [2])
m4_define([lib_minor], [0])
m4_define([lib_level], [0alpha])
m4_define([lib_version], [lib_major.lib_minor.lib_level])

AC_INIT([libsidplayfp], [lib_version], [], [], [http://sourceforge.net/projects/sidplay-residfp/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/sidplayfp/sidplayfp.cpp])
AC_CONFIG_HEADER([src/config.h])
AM_INIT_AUTOMAKE

LIB_MAJOR=lib_major
LIB_MINOR=lib_minor
LIB_LEVEL=lib_level
AC_SUBST([LIB_MAJOR])
AC_SUBST([LIB_MINOR])
AC_SUBST([LIB_LEVEL])

AC_CANONICAL_HOST

AS_CASE([$host_os],
        [mingw*], [MINGW32=yes],
        [MINGW32=no]
)
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])

dnl Initialize libtool.
LT_INIT([win32-dll])

dnl Checks for programs.
AC_PROG_CXX

dnl Use C++ for tests.
AC_LANG([C++])

SID_CXX_COMPILE_STDCXX_11

dnl check for hidden visibility support
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden -fvisibility-inlines-hidden], [has_visibility=yes], [has_visibility=no])
AM_CONDITIONAL([HAVE_VISIBILITY], [test "x$has_visibility" = xyes])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])

AS_IF([test $ac_cv_sizeof_short -ne 2],
  [AC_MSG_ERROR([size of short must be 16 bit])]
)

AS_IF([test $ac_cv_sizeof_int -lt 4],
  [AC_MSG_ERROR([only 32 bit or better CPUs are supported])]
)

AS_IF([test "x${ac_cv_header_stdint_h}" != "xyes"],
  [AC_MSG_ERROR([Required header stdint.h not found])]
)

dnl Checks for non-standard functions.

AC_CHECK_DECL(
    [strcasecmp],
    [AC_CHECK_FUNCS([strcasecmp])],
    [AC_CHECK_DECL(
        [stricmp],
        [AC_CHECK_FUNCS([stricmp])]
    )]
)

AC_CHECK_DECL(
    [strncasecmp],
    [AC_CHECK_FUNCS([strncasecmp])],
    [AC_CHECK_DECL(
        [strnicmp],
        [AC_CHECK_FUNCS([strnicmp])]
    )]
)

AX_LIB_GCRYPT([auto])
AM_CONDITIONAL([LIBGCRYPT], [test "x$have_libgcrypt" = "xyes"])


dnl libtool-style version-info number
#
# http://blog.flameeyes.eu/2009/04/22/shared-object-version
#
# if you don’t change the interface at all just increase the
#     “interface revision” value;
# if you make backward-compatible changes (like adding interfaces),
#     increase the “current interface” value and the “older interface age”
#     value, reset “interface revision” to zero;
# if you make backward-incompatible changes, breaking ABI
#     (removing interfaces for instance), increase the “current interface”
#     value and # reset both “older interface age” and ”interface revision”
#     to zero.

LIBSIDPLAYCUR=4
LIBSIDPLAYREV=0
LIBSIDPLAYAGE=1
LIBSIDPLAYVERSION=$LIBSIDPLAYCUR:$LIBSIDPLAYREV:$LIBSIDPLAYAGE

LIBSTILVIEWCUR=0
LIBSTILVIEWREV=1
LIBSTILVIEWAGE=0
LIBSTILVIEWVERSION=$LIBSTILVIEWCUR:$LIBSTILVIEWREV:$LIBSTILVIEWAGE

AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes/full, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([Build without debugging messages]); debug_flags=-DNDEBUG],
  [AS_IF([test "x$enable_debug" = "xyes"],
    [AC_MSG_RESULT([Build with debugging messages]); debug_flags=-DDEBUG=1],
    [AC_MSG_RESULT([Build with all debugging messages]); debug_flags=-DDEBUG=10]
  )]
)

AC_SUBST([debug_flags])

AC_ARG_ENABLE([hardsid],
  AS_HELP_STRING([--enable-hardsid],[enable hardsid builder [default=no]])
)

AM_CONDITIONAL([HARDSID], [test "x$enable_hardsid" = "xyes"])


AC_ARG_ENABLE([inline],
  AS_HELP_STRING([--enable-inline],[enable inlining of functions [default=yes]])
)

AS_IF([test x"$enable_inline" != "xno"],
  [RESID_INLINE=inline; RESID_INLINING=1],
  [RESID_INLINE=""; RESID_INLINING=0]
)

AC_ARG_ENABLE([mmx],
  [AS_HELP_STRING([--enable-mmx],
    [enable MMX optimization [default=no]])]
)

AS_IF([test x"$enable_mmx" = xyes],
# Testing for mmintrin.h requires compiler support
# for mmx support activated by specific flags
# these are most probably defined in CXXFLAGS
# but AC_CHECK_HEADERS uses only CPPFLAGS
# so we need a workaround
  [saveCPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $CXXFLAGS"

   AC_CHECK_HEADERS([mmintrin.h])

   CPPFLAGS=$saveCPPFLAGS]
)
 
AC_CACHE_CHECK([for working bool], ac_cv_cxx_bool,
[AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([],
    [[bool flag;]])],
  [ac_cv_cxx_bool=yes],
  [ac_cv_cxx_bool=no])]
)

AS_IF([test "x$ac_cv_cxx_bool" = "xno"],
  [RESID_HAVE_BOOL=0],
  [RESID_HAVE_BOOL=1]
)

dnl Enable branch prediction hints.
AC_ARG_ENABLE([branch-hints],
  [AS_HELP_STRING([--enable-branch-hints],
    [enable static branch prediction hints [default=yes]])],
  [],
  [enable_branch_hints=yes]
)

AS_IF([test "$enable_branch_hints" != no],
  [AC_SUBST([RESID_BRANCH_HINTS], [1])],
  [AC_SUBST([RESID_BRANCH_HINTS], [0])]
)

AC_CACHE_CHECK([for log1p], [resid_cv_log1p],
  [AC_TRY_COMPILE([#include <math.h>], [ log1p(1); ],
    resid_cv_log1p=yes, resid_cv_log1p=no)])

AS_IF([test "$resid_cv_log1p" = no],
  [AC_SUBST([RESID_HAVE_LOG1P], [0])],
  [AC_SUBST([RESID_HAVE_LOG1P], [1])])


AC_CACHE_CHECK([for __builtin_expect], [resid_cv_builtin_expect],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() { __builtin_expect(0, 0); }])],
    [resid_cv_builtin_expect=yes], [resid_cv_builtin_expect=no])]
)

AS_IF([test "$resid_cv_builtin_expect" = no],
  [AC_SUBST([HAVE_BUILTIN_EXPECT], [0])],
  [AC_SUBST([HAVE_BUILTIN_EXPECT], [1])]
)


AC_CHECK_PROGS([DOXYGEN], [doxygen])
AS_IF([test -z "$DOXYGEN"],
  [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])],
  [AC_CHECK_PROG([DOT], [dot], [YES], [NO])]
)

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])


AC_ARG_ENABLE([testsuite],
  [AS_HELP_STRING([--enable-testsuite=PATH_TO_TESTSUITE],
    [enable Lorenz' testsuite [default=no]]
  )],
  [],
  [enable_testsuite=no]
)

AS_IF([test "x$enable_testsuite" != xno],
  AC_DEFINE_UNQUOTED([PC64_TESTSUITE],
    ["$enable_testsuite"],
    [Path to Lorenz' testsuite.]
  )
)

AM_CONDITIONAL([TESTSUITE], [test "x$enable_testsuite" != xno])

# FIXME use pkgconfig
AC_ARG_ENABLE([tests],
  [AS_HELP_STRING([--enable-tests],
    [enable unit tests [default=no]]
  )],
  [[echo Enabling unit tests]
    AC_CHECK_HEADERS([UnitTest++/UnitTest++.h], [], AC_MSG_ERROR([UnitTest++.h header not found]))
        AS_IF(
                [test "x${ac_cv_header_UnitTestpp_UnitTestpp_h}" = xyes],
                AC_CHECK_LIB([UnitTest++], [main], [unittest_libs=-lUnitTest++], AC_MSG_ERROR([UnitTest++ library not found]))
        )
    [am_unittest=true]
  ],
  [am_unittest=false]
)

AM_CONDITIONAL([ENABLE_TEST], [test x$am_unittest = xtrue])
AC_SUBST([unittest_libs])

AC_SUBST(RESID_HAVE_BOOL)
AC_SUBST(RESID_INLINING)
AC_SUBST(RESID_INLINE)
AC_SUBST(LIBSIDPLAYVERSION)
AC_SUBST(LIBSTILVIEWVERSION)

# exSID support requires either libexsid or one of libfdti1 or libftd2xx
PKG_CHECK_MODULES([EXSID],
    [libexsid >= 1.0],
    [AC_DEFINE([HAVE_EXSID], 1, [Define to 1 if you have libexsid (-lexsid).])
    build_exsid_builder=yes],
    [AC_MSG_WARN([$EXSID_PKG_ERRORS] [- using internal driver])
        AS_IF([test "x$MINGW32" != "xyes"],
            AC_SEARCH_LIBS([dlopen], [dl dld], [],
              [AC_MSG_ERROR([unable to find the dlopen() function])]
            )
        )
        AC_CHECK_HEADERS([ftd2xx.h],
            [AC_DEFINE([HAVE_FTD2XX], 1, [Define to 1 if you have ftd2xx.h])
            build_exsid_builder=yes
            build_exsid_driver=yes],
            [AC_MSG_WARN([ftd2xx.h not found, support will not be available])]
        )
        AC_MSG_CHECKING([for libftdi1])
        PKG_CHECK_EXISTS([libftdi1 >= 1.0],
            [AC_DEFINE([HAVE_FTDI], 1, [Define to 1 if you have ftdi.h])
            build_exsid_builder=yes
            build_exsid_driver=yes
            FTDI_CFLAGS=`$PKG_CONFIG --cflags libftdi1`
            AC_MSG_RESULT([yes])],
            [AC_MSG_WARN([libftdi1 not found, support will not be available])]
        )]
)
AC_SUBST([FTDI_CFLAGS])

AM_CONDITIONAL([EXSID_SUPPORT], [ test "x${build_exsid_builder}" = xyes])
AM_CONDITIONAL([EXSID_DRIVER], [ test "x${build_exsid_driver}" = xyes])

eval loadable_shrext=$shrext_cmds
AC_DEFINE_UNQUOTED([SHLIBEXT], ["${loadable_shrext}"], [Shared library extension])



AC_CONFIG_FILES([
Makefile
libsidplayfp.pc
libstilview.pc
src/sidplayfp/sidversion.h
src/builders/residfp-builder/residfp/siddefs-fp.h
src/builders/resid-builder/resid/siddefs.h
tests/Makefile
])

AC_OUTPUT
