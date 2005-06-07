AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = wavpack wvunpack

lib_LTLIBRARIES = libwavpack.la

wpincludedir = $(prefix)/include/wavpack

wpinclude_HEADERS = md5.h wavpack.h wputils.h unpack3.h

libwavpack_la_SOURCES = bits.c float.c metadata.c unpack.c unpack3.c utils.c \
												wputils.c words.c md5.c extra1.c extra2.c pack.c \
												md5.h wavpack.h wputils.h unpack3.h
libwavpack_la_CFLAGS = -DPACK -DUNPACK -DUSE_FSTREAMS -DTAGS -DSEEKING -DVER3
libwavpack_la_LDFLAGS = -lm @ICONV_LIBS@

wavpack_SOURCES = wavpack.c
wavpack_CFLAGS = -DPACK
wavpack_LDFLAGS = -lm -lcurses
wavpack_LDADD = libwavpack.la

wvunpack_SOURCES = wvunpack.c
wvunpack_CFLAGS = -DUNPACK -DUSE_FSTREAMS
wvunpack_LDFLAGS = -lm -lcurses
wvunpack_LDADD = libwavpack.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wavpack.pc
