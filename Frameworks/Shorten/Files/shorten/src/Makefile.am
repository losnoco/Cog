noinst_LTLIBRARIES = libshorten.la
BUILT_SOURCES = bitshift.h

libshorten_la_LDFLAGS = -module -export-dynamic -avoid-version
libshorten_la_SOURCES = array.c convert.c fixio.c id3v2.c misc.c output.c shorten.c seek.c sulawalaw.c vario.c wave.c decode.c

INCLUDES = -I$(top_srcdir)/include

CLEANFILES = $(BUILT_SOURCES)

array.c: $(BUILT_SOURCES)

bitshift.h:
	-@if [ -x "../util/mkbshift" ] ; then \
	  ../util/mkbshift ; \
	else \
	  echo "" ; \
	  echo "***  missing ../util/mkbshift - run '$(MAKE)' in the top-level directory to build it  ***" ; \
	  echo "" ; \
	  exit 1 ; \
	fi
