
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

if MINGW32
  W32_CPPFLAGS = -DWIN32_LEAN_AND_MEAN
  W32_LDFLAGS = -no-undefined
endif

AM_CPPFLAGS = -Wall -fstrict-aliasing \
$(FTDI_CFLAGS) \
${W32_CPPFLAGS} \
$(PTHREAD_CFLAGS) \
@debug_flags@

EXTRA_DIST = \
docs/mainpage.dox

lib_LTLIBRARIES = libexsid.la

libexsid_la_SOURCES = \
exSID.c exSID_defs.h exSID_ftdiwrap.h exSID_ftdiwrap.c c11threads.h

libexsid_la_HEADERS = \
exSID.h

libexsid_ladir = $(includedir)

libexsid_la_LDFLAGS = -version-info $(LIBEXSIDVERSION) $(W32_LDFLAGS) $(PTHREAD_LIBS) $(PTHREAD_CFLAGS)

#=========================================================

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libexsid.pc

#=========================================================
# docs
if HAVE_DOXYGEN
doc:
	$(DOXYGEN) Doxyfile

clean-local:
	rm -fr $(builddir)/docs/html
endif
