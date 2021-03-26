/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define for threaded driver */
#define EXSID_THREADED 1

/* Algorithm AES in gcrypt library */
/* #undef GCRYPT_WITH_AES */

/* Algorithm ARCFOUR in gcrypt library */
/* #undef GCRYPT_WITH_ARCFOUR */

/* Algorithm BLOWFISH in gcrypt library */
/* #undef GCRYPT_WITH_BLOWFISH */

/* Algorithm CAST5 in gcrypt library */
/* #undef GCRYPT_WITH_CAST5 */

/* Algorithm CRC in gcrypt library */
/* #undef GCRYPT_WITH_CRC */

/* Algorithm DES in gcrypt library */
/* #undef GCRYPT_WITH_DES */

/* Algorithm DSA in gcrypt library */
/* #undef GCRYPT_WITH_DSA */

/* Algorithm ELGAMAL in gcrypt library */
/* #undef GCRYPT_WITH_ELGAMAL */

/* Algorithm HAVAL in gcrypt library */
/* #undef GCRYPT_WITH_HAVAL */

/* Algorithm IDEA in gcrypt library */
/* #undef GCRYPT_WITH_IDEA */

/* Algorithm MD2 in gcrypt library */
/* #undef GCRYPT_WITH_MD2 */

/* Algorithm MD4 in gcrypt library */
/* #undef GCRYPT_WITH_MD4 */

/* Algorithm MD5 in gcrypt library */
/* #undef GCRYPT_WITH_MD5 */

/* Algorithm RFC2268 in gcrypt library */
/* #undef GCRYPT_WITH_RFC2268 */

/* Algorithm RMD160 in gcrypt library */
/* #undef GCRYPT_WITH_RMD160 */

/* Algorithm RSA in gcrypt library */
/* #undef GCRYPT_WITH_RSA */

/* Algorithm SERPENT in gcrypt library */
/* #undef GCRYPT_WITH_SERPENT */

/* Algorithm SHA0 in gcrypt library */
/* #undef GCRYPT_WITH_SHA0 */

/* Algorithm SHA1 in gcrypt library */
/* #undef GCRYPT_WITH_SHA1 */

/* Algorithm SHA224 in gcrypt library */
/* #undef GCRYPT_WITH_SHA224 */

/* Algorithm SHA256 in gcrypt library */
/* #undef GCRYPT_WITH_SHA256 */

/* Algorithm SHA384 in gcrypt library */
/* #undef GCRYPT_WITH_SHA384 */

/* Algorithm SHA512 in gcrypt library */
/* #undef GCRYPT_WITH_SHA512 */

/* Algorithm TIGER in gcrypt library */
/* #undef GCRYPT_WITH_TIGER */

/* Algorithm TWOFISH in gcrypt library */
/* #undef GCRYPT_WITH_TWOFISH */

/* Algorithm WHIRLPOOL in gcrypt library */
/* #undef GCRYPT_WITH_WHIRLPOOL */

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have libexsid (-lexsid). */
/* #undef HAVE_EXSID */

/* Define to 1 if you have ftd2xx.h */
/* #undef HAVE_FTD2XX */

/* Define to 1 if you have the <ftd2xx.h> header file. */
/* #undef HAVE_FTD2XX_H */

/* Define to 1 if you have ftdi.h */
#define HAVE_FTDI 1

/* Gcrypt library is available */
/* #undef HAVE_GCRYPT */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <mmintrin.h> header file. */
/* #undef HAVE_MMINTRIN_H */

/* Define to 1 if you have pthread.h */
#define HAVE_PTHREAD_H 1

/* Have PTHREAD_PRIO_INHERIT. */
#define HAVE_PTHREAD_PRIO_INHERIT 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `stricmp' function. */
/* #undef HAVE_STRICMP */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strnicmp' function. */
/* #undef HAVE_STRNICMP */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <UnitTest++/UnitTest++.h> header file. */
/* #undef HAVE_UNITTEST___UNITTEST___H */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "libsidplayfp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libsidplayfp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsidplayfp 2.1.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libsidplayfp"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://sourceforge.net/projects/sidplay-residfp/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.1.0"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Shared library extension */
#define SHLIBEXT ".dylib"

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.1.0"

/* Path to VICE testsuite. */
/* #undef VICE_TESTSUITE */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

#if defined(__x86_64__) \
    || defined(_M_X64)
#define HAVE_MMINTRIN_H 1
#elif (defined(__arm64__) && defined(__APPLE__)) || defined(__aarch64__)
#define HAVE_ARM_NEON_H 1
#endif
