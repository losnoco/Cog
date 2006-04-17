# wavpack 4.2 configure.ac

AC_INIT(wavpack, 4.2, bryant@wavpack.com)
AM_INIT_AUTOMAKE(wavpack, 4.2, bryant@wavpack.com)
AC_CONFIG_SRCDIR([pack.c])

# Check for os version
VERSION_OS=$(uname -s)
AC_DEFINE_UNQUOTED(VERSION_OS, "$VERSION_OS", [os version])

# Check for processor characteristics
AC_C_BIGENDIAN(AC_DEFINE([HIGHFIRST], [1], [big-endian machine]))

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.
AC_CHECK_LIB(m, log10, [], AC_MSG_ERROR(math library not found))

# Check for iconv
AC_ARG_WITH(iconv, [  --with-iconv[=DIR]      Add ICONV support (on)])
if test "$with_iconv" = "no" ; then
    AC_MSG_ERROR([[Sorry, you can't deactivate iconv.]])
else
    if test "$with_iconv" != "yes" -a "$with_iconv" != "" ; then
       CPPFLAGS="${CPPFLAGS} -I$with_iconv/include"
       ICONV_LIBS="-L$with_iconv/lib"
    fi

    AC_CHECK_HEADER(iconv.h,
        AC_MSG_CHECKING(for iconv)
        AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],[
iconv_t cd = iconv_open ("","");
iconv (cd, NULL, NULL, NULL, NULL);],[
            AC_MSG_RESULT(yes)
            WITH_ICONV=1
            ICONV=""],[
            AC_MSG_RESULT(no)
            AC_MSG_CHECKING(for iconv in -liconv)

            _ldflags="${LDFLAGS}"
            _libs="${LIBS}"
            LDFLAGS="${LDFLAGS} ${ICONV_LIBS}"
            LIBS="${LIBS} -liconv"

            AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],[
iconv_t cd = iconv_open ("","");
iconv (cd, NULL, NULL, NULL, NULL);],[
                AC_MSG_RESULT(yes)
                WITH_ICONV=1
                ICONV_LIBS="${ICONV_LIBS} -liconv"
                ICONV="${ICONV_LIBS}"],[
                AC_MSG_ERROR([[Can't find iconv libraries.]])])]),
                AC_MSG_ERROR([[Can't find iconv headers.]]))
fi
AC_SUBST(ICONV)
AC_SUBST(ICONV_LIBS)

AC_OUTPUT(Makefile wavpack.pc)
