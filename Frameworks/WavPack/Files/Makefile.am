AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = wavpack wvunpack wvgain

lib_LTLIBRARIES = libwavpack.la

wpincludedir = $(prefix)/include/wavpack

wpinclude_HEADERS = md5.h wavpack.h wputils.h unpack3.h

libwavpack_la_SOURCES = bits.c float.c metadata.c unpack.c unpack3.c \
												wputils.c words.c md5.c extra1.c extra2.c pack.c \
												md5.h wavpack.h wputils.h unpack3.h
libwavpack_la_CFLAGS = -DPACK -DUNPACK -DUSE_FSTREAMS -DTAGS -DSEEKING -DVER3
libwavpack_la_LDFLAGS = -lm @ICONV_LIBS@

wavpack_SOURCES = wavpack.c utils.c
wavpack_CFLAGS = -DPACK
wavpack_LDFLAGS = -lm
wavpack_LDADD = libwavpack.la

wvunpack_SOURCES = wvunpack.c utils.c
wvunpack_CFLAGS = -DUNPACK -DUSE_FSTREAMS
wvunpack_LDFLAGS = -lm
wvunpack_LDADD = libwavpack.la

wvgain_SOURCES = wvgain.c utils.c
wvgain_CFLAGS = -DUNPACK -DUSE_FSTREAMS
wvgain_LDFLAGS = -lm
wvgain_LDADD = libwavpack.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wavpack.pc
