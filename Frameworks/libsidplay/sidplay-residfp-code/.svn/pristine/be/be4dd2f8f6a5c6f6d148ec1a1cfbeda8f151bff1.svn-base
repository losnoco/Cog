dnl Process this file with autoconf to produce a configure script.
AC_INIT([sidplayfp], [1.5.0alpha], [], [], [http://sourceforge.net/projects/sidplay-residfp/])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE



AC_CANONICAL_HOST
case "$host" in
    *mingw32* )
        MINGW32=yes
        ;;
esac
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])


AC_PROG_RANLIB

dnl Checks for programs.
AC_PROG_CXX

dnl Use C++ for tests.
AC_LANG([C++])

SID_CXX_COMPILE_STDCXX_11


dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN


dnl Audio subsystem

AUDIO_LDFLAGS=""

PKG_CHECK_MODULES(ALSA,
    [alsa >= 1.0],
    [AC_DEFINE([HAVE_ALSA], 1, [Define to 1 if you have libasound (-lasound).])],
    [AC_MSG_WARN([$ALSA_PKG_ERRORS])]
)

PKG_CHECK_MODULES(PULSE,
    [libpulse-simple >= 1.0],
    [AC_DEFINE([HAVE_PULSE], 1, [Define to 1 if you have libpulse-simple (-lpulse-simple).])],
    [AC_MSG_WARN([$PULSE_PKG_ERRORS])]
)


dnl Checks what version of Unix we have and soundcard support
AC_CHECK_HEADERS([sys/ioctl.h linux/soundcard.h machine/soundcard.h \
sys/soundcard.h soundcard.h])

AC_CHECK_HEADERS([dsound.h mmsystem.h], [], [], [#include <windows.h>])

AS_IF([test "$ac_cv_header_dsound_h" = "yes"],
    [AUDIO_LDFLAGS="$AUDIO_LDFLAGS -ldsound -ldxguid"]
)

AS_IF([test "$ac_cv_header_mmsystem_h" = "yes"],
    [AUDIO_LDFLAGS="$AUDIO_LDFLAGS -lwinmm"]
)

# NetBSD/OpenBSD OSS audio emulation
AS_IF([test "x$ac_cv_header_soundcard_h" = "xyes"],
    [AUDIO_LDFLAGS="$AUDIO_LDFLAGS -lossaudio"]
)

AC_SUBST(AUDIO_LDFLAGS)

AC_CHECK_FUNCS([strncasecmp strcasecmp])

PKG_CHECK_MODULES(SIDPLAYFP, [libsidplayfp >= 1.0])
PKG_CHECK_MODULES(STILVIEW, [libstilview >= 1.0])

# hack?
saveCPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $SIDPLAYFP_CFLAGS"

AC_CHECK_HEADERS([sidplayfp/builders/residfp.h \
sidplayfp/builders/resid.h \
sidplayfp/builders/hardsid.h \
sidplayfp/builders/exsid.h])

CPPFLAGS=$saveCPPFLAGS

AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([Build without debugging messages]); debug_flags=-DNDEBUG]
)

AC_SUBST([debug_flags])

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
