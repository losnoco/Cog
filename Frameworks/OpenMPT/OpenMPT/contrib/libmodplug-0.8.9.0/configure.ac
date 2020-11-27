AC_INIT([libopenmpt-modplug], [0.8.9.0-openmpt1], [https://bugs.openmpt.org/], [libopenmpt-modplug], [https://lib.openmpt.org/])
AC_PREREQ([2.68])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([Makefile libopenmpt_modplug.pc libmodplug.pc])

AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects])

AM_PROG_AR

LT_INIT

AC_SYS_LARGEFILE

PKG_PROG_PKG_CONFIG([0.24])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL

LIBOPENMPT_REQUIRES_PRIVATE=
LIBOPENMPT_LIBS_PRIVATE=

# We want a modern C compiler 
AC_PROG_CC_STDC
#AC_PROG_CC_C99

# We need C++11 support
AX_CXX_COMPILE_STDCXX(17, [noext], [optional])
AS_IF([test "x$HAVE_CXX17" != "x1"],
 [
  AX_CXX_COMPILE_STDCXX(14, [noext], [optional])
  AS_IF([test "x$HAVE_CXX14" != "x1"],
   [
    AX_CXX_COMPILE_STDCXX(11, [noext], [mandatory])
   ],[]
  )
 ],[]
)

AC_LANG_PUSH([C])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CFLAGS="$CFLAGS -fvisibility=hidden -DSYM_VISIBILITY"])
AX_CFLAGS_WARN_ALL
AC_LANG_POP([C])

AC_LANG_PUSH([C++])
AX_CHECK_COMPILE_FLAG([-fvisibility=hidden], [CXXFLAGS="$CXXFLAGS -fvisibility=hidden -DSYM_VISIBILITY"])
AX_CXXFLAGS_WARN_ALL
AC_LANG_POP([C++])

PKG_CHECK_MODULES([LIBOPENMPT], [libopenmpt])

# libmodplug emulation
AC_ARG_ENABLE([libopenmpt_modplug], AS_HELP_STRING([--disable-libopenmpt_modplug], [Disable the libopenmpt_modplug emulation library of the libmodplug interface.]))
AM_CONDITIONAL([ENABLE_LIBOPENMPT_MODPLUG], [test "x$enable_libopenmpt_modplug" != "xno"])

# libmodplug replacement
AC_ARG_ENABLE([libmodplug], AS_HELP_STRING([--enable-libmodplug], [Enable libmodplug replacement library based on libopenmpt.
WARNING: This will replace your current libmodplug installation.
CAUTION: The emulation of the libmodplug interface is not complete as libmodplug exposes lots of internal implementation details. If any of those is used by an application, the emulation via libopenmpt will fail and/or crash.
]))
AM_CONDITIONAL([ENABLE_LIBMODPLUG], [test "x$enable_libmodplug" = "xyes"])

AC_OUTPUT
