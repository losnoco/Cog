
ACLOCAL_AMFLAGS = -I m4 --install
EXTRA_DIST = 
EXTRA_DIST += LICENSE
EXTRA_DIST += libopenmpt_modplug.pc.in
EXTRA_DIST += libmodplug.pc.in
EXTRA_DIST += test.sh
MOSTLYCLEANFILES = 

dist_doc_DATA = 
dist_doc_DATA += LICENSE
nobase_dist_doc_DATA = 

bin_PROGRAMS = 
check_PROGRAMS = 
lib_LTLIBRARIES = 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = 
nobase_include_HEADERS = 

if ENABLE_LIBOPENMPT_MODPLUG
lib_LTLIBRARIES += libopenmpt_modplug.la
libopenmpt_modplug_la_LDFLAGS = -version-info 1:0:0 -no-undefined
libopenmpt_modplug_la_CPPFLAGS = -I$(srcdir)/ $(LIBOPENMPT_CPPFLAGS)
libopenmpt_modplug_la_CXXFLAGS = $(LIBOPENMPT_CFLAGS)
libopenmpt_modplug_la_CFLAGS = $(LIBOPENMPT_CFLAGS)
libopenmpt_modplug_la_LIBADD = $(LIBOPENMPT_LIBS)
libopenmpt_modplug_la_SOURCES = 
libopenmpt_modplug_la_SOURCES += libopenmpt_modplug.c
libopenmpt_modplug_la_SOURCES += libopenmpt_modplug_cpp.cpp
endif

if ENABLE_LIBMODPLUG
pkgconfig_DATA += libmodplug.pc
lib_LTLIBRARIES += libmodplug.la
libmodplug_la_LDFLAGS = -version-info 1:0:0 -no-undefined
nobase_include_HEADERS += libmodplug/modplug.h libmodplug/sndfile.h libmodplug/stdafx.h
libmodplug_la_CPPFLAGS = -I$(srcdir)/ $(LIBOPENMPT_CPPFLAGS)
libmodplug_la_CXXFLAGS = $(LIBOPENMPT_CFLAGS)
libmodplug_la_CFLAGS = $(LIBOPENMPT_CFLAGS)
libmodplug_la_LIBADD = $(LIBOPENMPT_LIBS)
libmodplug_la_SOURCES = 
libmodplug_la_SOURCES += libopenmpt_modplug.c
libmodplug_la_SOURCES += libopenmpt_modplug_cpp.cpp
endif
