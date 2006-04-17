libdir = `lamip --plugin-dir`
lib_LTLIBRARIES = libinputshorten.la

INCLUDES = -I. -I.. -I$(top_srcdir)/shorten -I$(top_srcdir)/shorten/util -I$(top_srcdir)/shorten/src
libinputshorten_la_LDFLAGS = -module -avoid-version
libinputshorten_la_LIBADD = $(top_srcdir)/shorten/src/libshorten.la
libinputshorten_la_SOURCES = libinputshorten.c
