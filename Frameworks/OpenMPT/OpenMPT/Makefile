#
# libopenmpt and openmpt123 GNU Makefile
#
# Authors: Joern Heusipp
#          OpenMPT Devs
# 
# The OpenMPT source code is released under the BSD license.
# Read LICENSE for more details.
#

#
# Supported parameters:
#
#
# Build configuration (provide on each `make` invocation):
#
#  CONFIG=[gcc|clang|mingw-w64|emscripten|djgpp] (default: CONFIG=)
#
#  Build configurations can override or change defaults of other build options.
#  See below and in `build/make/` for details.
#
#
# Compiler options (environment variables):
#
#  CC
#  CXX
#  LD
#  AR
#  CPPFLAGS
#  CXXFLAGS
#  CFLAGS
#  LDFLAGS
#  LDLIBS
#  ARFLAGS
#  PKG_CONFIG
#
#  CXXSTDLIB_PCLIBSPRIVATE   C++ standard library (or libraries) required for
#                   static linking. This will be put in the pkg-config file
#                   libopenmpt.pc Libs.private field and used for nothing else.
#
#
#
# Build flags (provide on each `make` invocation) (defaults are shown):
#
#  DYNLINK=1           Dynamically link examples and openmpt123 against libopenmpt
#  SHARED_LIB=1        Build shared library
#  STATIC_LIB=1        Build static library
#  EXAMPLES=1          Build examples
#  OPENMPT123=1        Build openmpt123
#  IN_OPENMPT=0        Build in_openmpt (WinAMP 2.x plugin)
#  XMP_OPENMPT=0       Build xmp-openmpt (XMPlay plugin)
#  SHARED_SONAME=1     Set SONAME of shared library
#  DEBUG=0             Build debug binaries without optimization and with symbols
#  OPTIMIZE=vectorize  -O3
#           speed      -O2
#           size       -Os/-Oz
#           test       -Og
#           debug      -O0
#           none
#  OPTIMIZE_LTO=0      Build with link-time-optimizations
#  OPTIMIZE_FASTMATH=0 Use no non-standard-compliant fastmath optimizations
#                   =1  + use mild non-standard-compliant fastmath optimizations
#                   =2  + assume no inf/nan
#                   =3  + set ftz/daz
#  TEST=1              Include test suite in default target.
#  ONLY_TEST=0         Only build the test suite.
#  STRICT=0            Treat warnings as errors.
#  MODERN=0            Pass more modern compiler options.
#  ANCIENT=0           Pass compiler options compatible with older versions.
#  NATIVE=0            Optimize for system CPU.
#  STDCXX=c++17        C++ standard version (default depends on compiler)
#  STDC=c17            C standard version (default depends on compiler)
#  ANALYZE=0           Enable static analyzer.
#  CHECKED=0           Enable run-time assertions.
#  CHECKED_ADDRESS=0   Enable address sanitizer
#  CHECKED_UNDEFINED=0 Enable undefined behaviour sanitizer
#
#
# Build flags for libopenmpt (provide on each `make` invocation)
#  (defaults are 0):
#
#  NO_ZLIB=1        Avoid using zlib, even if found
#  NO_MPG123=1      Avoid using libmpg123, even if found
#  NO_OGG=1         Avoid using libogg, even if found
#  NO_VORBIS=1      Avoid using libvorbis, even if found
#  NO_VORBISFILE=1  Avoid using libvorbisfile, even if found
#
#  LOCAL_ZLIB=1        Build local copy of zlib, even if found
#  LOCAL_MPG123=1      Build local copy of libmpg123, even if found
#  LOCAL_OGG=1         Build local copy of libogg, even if found
#  LOCAL_VORBIS=1      Build local copy of libvorbis, even if found
#
#  NO_MINIZ=1       Do not fallback to miniz
#  NO_MINIMP3=1     Do not fallback to minimp3
#  NO_STBVORBIS=1   Do not fallback to stb_vorbis
#
#  USE_ALLEGRO42=1  Use liballegro 4.2 (DJGPP only)
#
# Build flags for libopenmpt examples and openmpt123
#  (provide on each `make` invocation)
#  (defaults are 0):
#
#  NO_PORTAUDIO=1      Avoid using PortAudio, even if found
#  NO_PORTAUDIOCPP=1   Avoid using PortAudio C++, even if found
#
# Build flags for openmpt123 (provide on each `make` invocation)
#  (defaults are 0):
#
#  NO_PULSEAUDIO=1     Avoid using PulseAudio, even if found
#  NO_SDL2=1           Avoid using SDL2, even if found
#  NO_FLAC=1           Avoid using FLAC, even if found
#  NO_SNDFILE=1        Avoid using libsndfile, even if found
#
#
# Install options (provide on each `make install` invocation)
#
#  PREFIX   (e.g.:  PREFIX=$HOME/opt, default: PREFIX=/usr/local)
#  DESTDIR  (e.g.:  DESTDIR=bin/dest, default: DESTDIR=)
#
#
# Verbosity:
#
#  QUIET=[0,1]      (default: QUIET=0)
#  VERBOSE=[0,1,2]  (default: VERBOSE=0)
#
#
# Supported targets:
#
#     make clean
#     make [all]
#     make doc
#     make check
#     make dist
#     make dist-doc
#     make install
#     make install-doc
#


.PHONY: all
all:


INFO       = @echo
SILENT     = @
VERYSILENT = @


ifeq ($(VERBOSE),2)
INFO       = @true
SILENT     = 
VERYSILENT = 
endif

ifeq ($(VERBOSE),1)
INFO       = @true
SILENT     = 
VERYSILENT = @
endif


ifeq ($(QUIET),1)
INFO       = @true
SILENT     = @
VERYSILENT = @
endif


# general settings

DYNLINK=1
SHARED_LIB=1
STATIC_LIB=1
EXAMPLES=1
FUZZ=0
SHARED_SONAME=1
DEBUG=0
OPTIMIZE=vectorize
OPTIMIZE_LTO=0
OPTIMIZE_FASTMATH=0
TEST=1
ONLY_TEST=0
SOSUFFIX=.so
SOSUFFIXWINDOWS=0
NO_SHARED_LINKER_FLAG=0
OPENMPT123=1
IN_OPENMPT=0
XMP_OPENMPT=0
MODERN=0
NATIVE=0
STRICT=0

FLAVOUR_DIR=

FORCE_UNIX_STYLE_COMMANDS=0

CHECKED=0
CHECKED_ADDRESS=0
CHECKED_UNDEFINED=0

REQUIRES_RUNPREFIX=0


# get commandline or defaults

CPPFLAGS := $(CPPFLAGS)
CXXFLAGS := $(CXXFLAGS)
CFLAGS   := $(CFLAGS)
LDFLAGS  := $(LDFLAGS)
LDLIBS   := $(LDLIBS)
ARFLAGS  := $(ARFLAGS)

PC_LIBS_PRIVATE := $(PC_LIBS_PRIVATE)

PREFIX   := $(PREFIX)
ifeq ($(PREFIX),)
PREFIX := /usr/local
endif

MANDIR ?= $(PREFIX)/share/man
#DESTDIR := $(DESTDIR)
#ifeq ($(DESTDIR),)
#DESTDIR := bin/dest
#endif


# version

include libopenmpt/libopenmpt_version.mk 

LIBOPENMPT_SO_VERSION=$(LIBOPENMPT_LTVER_CURRENT)


# host setup

ifneq ($(MSYSTEM)x,x)

HOST=unix
HOST_FLAVOUR=

TOOLCHAIN_SUFFIX=

CPPCHECK = cppcheck

MKDIR_P = mkdir -p
RM = rm -f
RMTREE = rm -rf
INSTALL = install
INSTALL_MAKE_DIR = install -d
INSTALL_DIR = cp -r -v
FIXPATH = $1

HOST_FLAVOUR=MSYS2

NUMTHREADS:=$(shell nproc)

else ifeq ($(OS),Windows_NT)

HOST=windows
HOST_FLAVOUR=

TOOLCHAIN_SUFFIX=

CPPCHECK = cppcheck

MKDIR_P = mkdir
RM = del /q /f
RMTREE = del /q /f /s
INSTALL = echo install
INSTALL_MAKE_DIR = echo install
INSTALL_DIR = echo install
FIXPATH = $(subst /,\,$1)

NUMTHREADS:=$(NUMBER_OF_PROCESSORS)

else

HOST=unix
HOST_FLAVOUR=

TOOLCHAIN_SUFFIX=

CPPCHECK = cppcheck

MKDIR_P = mkdir -p
RM = rm -f
RMTREE = rm -rf
INSTALL = install
INSTALL_MAKE_DIR = install -d
INSTALL_DIR = cp -r -v
FIXPATH = $1

UNAME_S:=$(shell uname -s)
ifeq ($(UNAME_S),Darwin)
HOST_FLAVOUR=MACOSX
endif
ifeq ($(UNAME_S),Linux)
HOST_FLAVOUR=LINUX
endif
ifeq ($(UNAME_S),NetBSD)
HOST_FLAVOUR=NETBSD
endif
ifeq ($(UNAME_S),FreeBSD)
HOST_FLAVOUR=FREEBSD
endif
ifeq ($(UNAME_S),OpenBSD)
HOST_FLAVOUR=OPENBSD
endif
ifeq ($(UNAME_S),Haiku)
HOST_FLAVOUR=HAIKU
endif

ifeq ($(HOST_FLAVOUR),LINUX)
NUMTHREADS:=$(shell nproc)
else
NUMTHREADS:=1
endif

endif


# early build setup

BINDIR_MADE:=$(shell $(MKDIR_P) bin)


# compiler setup

PKG_CONFIG ?= pkg-config

ifeq ($(CONFIG)x,x)
include build/make/config-defaults.mk

else ifeq ($(CONFIG),mingw64-win32)
$(warning warning: 'CONFIG=mingw64-win32' is deprecated. Use 'CONFIG=mingw-w64 WINDOWS_ARCH=x86' instead.)
WINDOWS_ARCH=x86
WINDOWS_FAMILY=
WINDOWS_VERSION=
include build/make/config-mingw-w64.mk

else ifeq ($(CONFIG),mingw64-win64)
$(warning warning: 'CONFIG=mingw64-win64' is deprecated. Use 'CONFIG=mingw-w64 WINDOWS_ARCH=amd64' instead.)
WINDOWS_ARCH=amd64
WINDOWS_FAMILY=
WINDOWS_VERSION=
include build/make/config-mingw-w64.mk

else ifeq ($(CONFIG),mingw64-winrt-x86)
$(warning warning: 'CONFIG=mingw64-winrt-x86' is deprecated. Use 'CONFIG=mingw-w64 WINDOWS_ARCH=x86 WINDOWS_FAMILY=pc-app WINDOWS_VERSION=win8' instead.)
WINDOWS_ARCH=x86
WINDOWS_FAMILY=pc-app
WINDOWS_VERSION=win8
include build/make/config-mingw-w64.mk

else ifeq ($(CONFIG),mingw64-winrt-amd64)
$(warning warning: 'CONFIG=mingw64-winrt-amd64' is deprecated. Use 'CONFIG=mingw-w64 WINDOWS_ARCH=amd64 WINDOWS_FAMILY=pc-app WINDOWS_VERSION=win8' instead.)
WINDOWS_ARCH=amd64
WINDOWS_FAMILY=pc-app
WINDOWS_VERSION=win8
include build/make/config-mingw-w64.mk

else
include build/make/config-$(CONFIG).mk

endif


ifeq ($(FORCE_UNIX_STYLE_COMMANDS),1)
MKDIR_P = mkdir -p
RM = rm -f
RMTREE = rm -rf
endif


# build setup

ifeq ($(SOSUFFIXWINDOWS),1)
LIBOPENMPT_SONAME=libopenmpt-$(LIBOPENMPT_SO_VERSION)$(SOSUFFIX)
else
LIBOPENMPT_SONAME=libopenmpt$(SOSUFFIX).$(LIBOPENMPT_SO_VERSION)
endif

INSTALL_PROGRAM = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 0644
INSTALL_LIB = $(INSTALL) -m 0644
INSTALL_DATA_DIR = $(INSTALL_DIR)
INSTALL_MAKE_DIR += -m 0755

CPPFLAGS += -Isrc -Icommon -I.

ifeq ($(XMP_OPENMPT),1)
CPPFLAGS += -Iinclude/pugixml/src
endif

ifeq ($(MPT_COMPILER_GENERIC),1)

CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  += 

ifeq ($(DEBUG),1)
CPPFLAGS += -DMPT_BUILD_DEBUG
CXXFLAGS += -g
CFLAGS   += -g
else
ifneq ($(OPTIMIZE),none)
CXXFLAGS += -O
CFLAGS   += -O
endif
endif

ifeq ($(CHECKED),1)
CPPFLAGS += -DMPT_BUILD_CHECKED
CXXFLAGS += -g
CFLAGS   += -g
endif

CXXFLAGS += -Wall
CFLAGS   += -Wall

else

ifeq ($(MPT_COMPILER_NOVISIBILITY),1)
CXXFLAGS +=
CFLAGS   +=
else
CXXFLAGS += -fvisibility=hidden
CFLAGS   += -fvisibility=hidden
endif
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  += 

ifeq ($(DEBUG),1)
CPPFLAGS += -DMPT_BUILD_DEBUG
CXXFLAGS += -g
CFLAGS   += -g
else ifeq ($(OPTIMIZE),debug)
CPPFLAGS += 
CXXFLAGS += -O0 -fno-omit-frame-pointer
CFLAGS   += -O0 -fno-omit-frame-pointer
else ifeq ($(OPTIMIZE),test)
CPPFLAGS += 
CXXFLAGS += -Og -fno-omit-frame-pointer
CFLAGS   += -Og -fno-omit-frame-pointer
else ifeq ($(OPTIMIZE),size)
CXXFLAGS += -Os
CFLAGS   += -Os -fno-strict-aliasing
LDFLAGS  += 
ifneq ($(MPT_COMPILER_NOSECTIONS),1)
CXXFLAGS += -ffunction-sections -fdata-sections
CFLAGS   += -ffunction-sections -fdata-sections
endif
ifneq ($(MPT_COMPILER_NOGCSECTIONS),1)
LDFLAGS  += -Wl,--gc-sections
endif
else ifeq ($(OPTIMIZE),speed)
CXXFLAGS += -O2
CFLAGS   += -O2 -fno-strict-aliasing
ifneq ($(MPT_COMPILER_NOSECTIONS),1)
CXXFLAGS += -ffunction-sections -fdata-sections
CFLAGS   += -ffunction-sections -fdata-sections
endif
ifneq ($(MPT_COMPILER_NOGCSECTIONS),1)
LDFLAGS  += -Wl,--gc-sections
endif
else ifeq ($(OPTIMIZE),vectorize)
CXXFLAGS += -O3
CFLAGS   += -O3 -fno-strict-aliasing
ifneq ($(MPT_COMPILER_NOSECTIONS),1)
CXXFLAGS += -ffunction-sections -fdata-sections
CFLAGS   += -ffunction-sections -fdata-sections
endif
ifneq ($(MPT_COMPILER_NOGCSECTIONS),1)
LDFLAGS  += -Wl,--gc-sections
endif
endif

ifeq ($(FASTMATH_STYLE),gcc)

ifeq ($(OPTIMIZE_FASTMATH),3)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FASTMATH -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -ffast-math
CFLAGS += -ffast-math
else ifeq ($(OPTIMIZE_FASTMATH),2)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -fno-math-errno
CXXFLAGS += -ffinite-math-only
CXXFLAGS += -fno-rounding-math
CXXFLAGS += -fno-signaling-nans
CXXFLAGS += -fexcess-precision=fast
CXXFLAGS += -fcx-limited-range
CXXFLAGS += -fassociative-math
CXXFLAGS += -freciprocal-math
CXXFLAGS += -fno-signed-zeros
CXXFLAGS += -fno-trapping-math
CFLAGS += -fno-math-errno
CFLAGS += -ffinite-math-only
CFLAGS += -fno-rounding-math
CFLAGS += -fno-signaling-nans
CFLAGS += -fexcess-precision=fast
CFLAGS += -fcx-limited-range
CFLAGS += -fassociative-math
CFLAGS += -freciprocal-math
CFLAGS += -fno-signed-zeros
CFLAGS += -fno-trapping-math
else ifeq ($(OPTIMIZE_FASTMATH),1)
CXXFLAGS += -fno-math-errno
CXXFLAGS += -fno-rounding-math
CXXFLAGS += -fno-signaling-nans
CXXFLAGS += -fexcess-precision=fast
CXXFLAGS += -fcx-limited-range
CXXFLAGS += -fassociative-math
CXXFLAGS += -freciprocal-math
CXXFLAGS += -fno-signed-zeros
CXXFLAGS += -fno-trapping-math
CFLAGS += -fno-math-errno
CFLAGS += -fno-rounding-math
CFLAGS += -fno-signaling-nans
CFLAGS += -fexcess-precision=fast
CFLAGS += -fcx-limited-range
CFLAGS += -fassociative-math
CFLAGS += -freciprocal-math
CFLAGS += -fno-signed-zeros
CFLAGS += -fno-trapping-math
endif

else ifeq ($(FASTMATH_STYLE),clang)

ifeq ($(OPTIMIZE_FASTMATH),3)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FASTMATH -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -ffast-math
CFLAGS += -ffast-math
else ifeq ($(OPTIMIZE_FASTMATH),2)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -fno-math-errno
CXXFLAGS += -ffinite-math-only
CXXFLAGS += -fno-honor-infinities
CXXFLAGS += -fno-honor-nans
CXXFLAGS += -ffp-contract=fast
CXXFLAGS += -fassociative-math
CXXFLAGS += -freciprocal-math
CXXFLAGS += -fno-signed-zeros
CXXFLAGS += -fno-trapping-math
CFLAGS += -fno-math-errno
CFLAGS += -ffinite-math-only
CFLAGS += -fno-honor-infinities
CFLAGS += -fno-honor-nans
CFLAGS += -ffp-contract=fast
CFLAGS += -fassociative-math
CFLAGS += -freciprocal-math
CFLAGS += -fno-signed-zeros
CFLAGS += -fno-trapping-math
else ifeq ($(OPTIMIZE_FASTMATH),1)
CXXFLAGS += -fno-math-errno
CXXFLAGS += -ffp-contract=fast
CXXFLAGS += -fassociative-math
CXXFLAGS += -freciprocal-math
CXXFLAGS += -fno-signed-zeros
CXXFLAGS += -fno-trapping-math
CFLAGS += -fno-math-errno
CFLAGS += -ffp-contract=fast
CFLAGS += -fassociative-math
CFLAGS += -freciprocal-math
CFLAGS += -fno-signed-zeros
CFLAGS += -fno-trapping-math
endif

else

ifeq ($(OPTIMIZE_FASTMATH),3)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FASTMATH -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -ffast-math
CFLAGS += -ffast-math
else ifeq ($(OPTIMIZE_FASTMATH),2)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FASTMATH -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -ffast-math
CFLAGS += -ffast-math
else ifeq ($(OPTIMIZE_FASTMATH),1)
CPPFLAGS += -DMPT_CHECK_CXX_IGNORE_WARNING_FASTMATH -DMPT_CHECK_CXX_IGNORE_WARNING_FINITEMATH
CXXFLAGS += -ffast-math
CFLAGS += -ffast-math
endif

endif

ifeq ($(MPT_COMPILER_NOIPARA),1)
# See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115049>.
CXXFLAGS += -fno-ipa-ra
CFLAGS   += -fno-ipa-ra
endif

ifeq ($(CHECKED),1)
CPPFLAGS += -DMPT_BUILD_CHECKED
CXXFLAGS += -g -fno-omit-frame-pointer
CFLAGS   += -g -fno-omit-frame-pointer
endif

ifeq ($(FUZZ),1)
CPPFLAGS +=
CXXFLAGS += -fno-omit-frame-pointer
CFLAGS   += -fno-omit-frame-pointer
endif

CXXFLAGS += -Wall -Wextra -Wpedantic $(CXXFLAGS_WARNINGS)
CFLAGS   += -Wall -Wextra -Wpedantic $(CFLAGS_WARNINGS)
LDFLAGS  += $(LDFLAGS_WARNINGS)

endif

ifeq ($(STRICT),1)
CXXFLAGS += -Werror
CFLAGS   += -Werror
endif

ifeq ($(DYNLINK),1)
LDFLAGS_RPATH += -Wl,-rpath,./bin
LDFLAGS_LIBOPENMPT += -Lbin
LDLIBS_LIBOPENMPT  += -lopenmpt
endif

ifeq ($(HOST),unix)

ifeq ($(IS_CROSS),1)
else
ifeq ($(shell help2man --version > /dev/null 2>&1 && echo yes ),yes)
MPT_WITH_HELP2MAN := 1
endif
endif

ifeq ($(shell doxygen --version > /dev/null 2>&1 && echo yes ),yes)
MPT_WITH_DOXYGEN := 1
endif

endif

PC_LIBS_PRIVATE += $(CXXSTDLIB_PCLIBSPRIVATE)

ifeq ($(HACK_ARCHIVE_SUPPORT),1)
NO_ZLIB:=1
endif

ifeq ($(LOCAL_ZLIB),1)
CPPFLAGS_ZLIB := -DMPT_WITH_ZLIB
LDFLAGS_ZLIB  :=
LDLIBS_ZLIB   :=
CPPFLAGS_ZLIB += -Iinclude/zlib/
LOCAL_ZLIB_SOURCES := 
LOCAL_ZLIB_SOURCES += include/zlib/adler32.c
LOCAL_ZLIB_SOURCES += include/zlib/compress.c
LOCAL_ZLIB_SOURCES += include/zlib/crc32.c
LOCAL_ZLIB_SOURCES += include/zlib/deflate.c
LOCAL_ZLIB_SOURCES += include/zlib/gzclose.c
LOCAL_ZLIB_SOURCES += include/zlib/gzlib.c
LOCAL_ZLIB_SOURCES += include/zlib/gzread.c
LOCAL_ZLIB_SOURCES += include/zlib/gzwrite.c
LOCAL_ZLIB_SOURCES += include/zlib/infback.c
LOCAL_ZLIB_SOURCES += include/zlib/inffast.c
LOCAL_ZLIB_SOURCES += include/zlib/inflate.c
LOCAL_ZLIB_SOURCES += include/zlib/inftrees.c
LOCAL_ZLIB_SOURCES += include/zlib/trees.c
LOCAL_ZLIB_SOURCES += include/zlib/uncompr.c
LOCAL_ZLIB_SOURCES += include/zlib/zutil.c
include/zlib/%$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT) -DSTDC -DZ_HAVE_UNISTD_H
include/zlib/%.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT) -DSTDC -DZ_HAVE_UNISTD_H
else
ifeq ($(NO_ZLIB),1)
else
#LDLIBS   += -lz
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists zlib && echo yes),yes)
CPPFLAGS_ZLIB := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I zlib ) -DMPT_WITH_ZLIB
LDFLAGS_ZLIB  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   zlib ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other zlib )
LDLIBS_ZLIB   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   zlib )
PC_REQUIRES_ZLIB := zlib
else
ifeq ($(FORCE_DEPS),1)
$(error zlib not found)
else
$(warning warning: zlib not found)
endif
NO_ZLIB:=1
endif
endif
endif

ifeq ($(LOCAL_MPG123),1)
CPPFLAGS_MPG123 := -DMPT_WITH_MPG123 -DMPG123_NO_LARGENAME
LDFLAGS_MPG123  := 
LDLIBS_MPG123   := 
CPPFLAGS_MPG123 += -Iinclude/mpg123/src/include/ -Iinclude/mpg123/ports/makefile/
LOCAL_MPG123_SOURCES := 
LOCAL_MPG123_SOURCES += include/mpg123/src/compat/compat.c
LOCAL_MPG123_SOURCES += include/mpg123/src/compat/compat_str.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/dct64.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/equalizer.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/feature.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/format.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/frame.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/icy.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/icy2utf8.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/id3.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/index.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/layer1.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/layer2.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/layer3.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/lfs_wrap.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/libmpg123.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/ntom.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/optimize.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/parse.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/readers.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/stringbuf.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/synth.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/synth_8bit.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/synth_real.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/synth_s32.c
LOCAL_MPG123_SOURCES += include/mpg123/src/libmpg123/tabinit.c
include/mpg123/src/compat/%$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT) -DOPT_GENERIC
include/mpg123/src/compat/%.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT) -DOPT_GENERIC
include/mpg123/src/libmpg123/%$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT) -DOPT_GENERIC
include/mpg123/src/libmpg123/%.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT) -DOPT_GENERIC
include/mpg123/src/compat/%$(FLAVOUR_O).o : CPPFLAGS:= -Iinclude/mpg123/src/include/ -Iinclude/mpg123/ports/makefile/ $(CPPFLAGS)
include/mpg123/src/compat/%.test$(FLAVOUR_O).o : CPPFLAGS:= -Iinclude/mpg123/src/include/ -Iinclude/mpg123/ports/makefile/ $(CPPFLAGS)
include/mpg123/src/libmpg123/%$(FLAVOUR_O).o : CPPFLAGS:= -Iinclude/mpg123/src/include/ -Iinclude/mpg123/ports/makefile/ $(CPPFLAGS)
include/mpg123/src/libmpg123/%.test$(FLAVOUR_O).o : CPPFLAGS:= -Iinclude/mpg123/src/include/ -Iinclude/mpg123/ports/makefile/ $(CPPFLAGS)
else
ifeq ($(NO_MPG123),1)
else
#LDLIBS   += -lmpg123
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists 'libmpg123 >= 1.14.0' && echo yes),yes)
CPPFLAGS_MPG123 := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I 'libmpg123 >= 1.14.0' ) -DMPT_WITH_MPG123
LDFLAGS_MPG123  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   'libmpg123 >= 1.14.0' ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other 'libmpg123 >= 1.14.0' )
LDLIBS_MPG123   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   'libmpg123 >= 1.14.0' )
PC_REQUIRES_MPG123 := libmpg123
else
ifeq ($(FORCE_DEPS),1)
$(error mpg123 not found)
else
$(warning warning: mpg123 not found)
endif
NO_MPG123:=1
endif
endif
endif

ifeq ($(LOCAL_OGG),1)
CPPFLAGS_OGG := -DMPT_WITH_OGG
LDFLAGS_OGG  := 
LDLIBS_OGG   := 
CPPFLAGS_OGG += -Iinclude/ogg/include/ -Iinclude/ogg/ports/makefile/
LOCAL_OGG_SOURCES := 
LOCAL_OGG_SOURCES += include/ogg/src/bitwise.c
LOCAL_OGG_SOURCES += include/ogg/src/framing.c
include/ogg/src/%$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
include/ogg/src/%.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
else
ifeq ($(NO_OGG),1)
else
#LDLIBS   += -logg
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists ogg && echo yes),yes)
CPPFLAGS_OGG := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I ogg ) -DMPT_WITH_OGG
LDFLAGS_OGG  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   ogg ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other ogg )
LDLIBS_OGG   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   ogg )
PC_REQUIRES_OGG := ogg
else
ifeq ($(FORCE_DEPS),1)
$(error ogg not found)
else
$(warning warning: ogg not found)
endif
NO_OGG:=1
endif
endif
endif

ifeq ($(LOCAL_VORBIS),1)
CPPFLAGS_VORBIS := -DMPT_WITH_VORBIS
LDFLAGS_VORBIS  := 
LDLIBS_VORBIS   := 
CPPFLAGS_VORBIS += -Iinclude/vorbis/include/ -Iinclude/vorbis/lib/
ifneq ($(MPT_COMPILER_NOALLOCAH),1)
CPPFLAGS_VORBIS += -DHAVE_ALLOCA_H
endif
LOCAL_VORBIS_SOURCES := 
LOCAL_VORBIS_SOURCES += include/vorbis/lib/analysis.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/bitrate.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/block.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/codebook.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/envelope.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/floor0.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/floor1.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/info.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/lookup.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/lpc.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/lsp.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/mapping0.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/mdct.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/psy.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/registry.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/res0.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/sharedbook.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/smallft.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/synthesis.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/vorbisenc.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/vorbisfile.c
LOCAL_VORBIS_SOURCES += include/vorbis/lib/window.c
include/vorbis/lib/%$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
include/vorbis/lib/%.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
else
ifeq ($(NO_VORBIS),1)
else
#LDLIBS   += -lvorbis
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists vorbis && echo yes),yes)
CPPFLAGS_VORBIS := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I vorbis ) -DMPT_WITH_VORBIS
LDFLAGS_VORBIS  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   vorbis ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other vorbis )
LDLIBS_VORBIS   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   vorbis )
PC_REQUIRES_VORBIS := vorbis
else
ifeq ($(FORCE_DEPS),1)
$(error vorbis not found)
else
$(warning warning: vorbis not found)
endif
NO_VORBIS:=1
endif
endif
endif

ifeq ($(LOCAL_VORBIS),1)
CPPFLAGS_VORBISFILE := -DMPT_WITH_VORBISFILE
LDFLAGS_VORBISFILE  := 
LDLIBS_VORBISFILE   := 
else
ifeq ($(NO_VORBISFILE),1)
else
#LDLIBS   += -lvorbisfile
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists vorbisfile && echo yes),yes)
CPPFLAGS_VORBISFILE := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I vorbisfile ) -DMPT_WITH_VORBISFILE
LDFLAGS_VORBISFILE  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   vorbisfile ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other vorbisfile )
LDLIBS_VORBISFILE   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   vorbisfile )
PC_REQUIRES_VORBISFILE := vorbisfile
else
ifeq ($(FORCE_DEPS),1)
$(error vorbisfile not found)
else
$(warning warning: vorbisfile not found)
endif
NO_VORBISFILE:=1
endif
endif
endif

ifeq ($(NO_SDL2),1)
else
#LDLIBS   += -lsdl2
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists 'sdl2 >= 2.0.4' && echo yes),yes)
CPPFLAGS_SDL2 := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I 'sdl2 >= 2.0.4' ) -DMPT_WITH_SDL2
LDFLAGS_SDL2  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   'sdl2 >= 2.0.4' ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other 'sdl2 >= 2.0.4' )
LDLIBS_SDL2   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   'sdl2 >= 2.0.4' )
else
ifeq ($(FORCE_DEPS),1)
$(error sdl2 not found)
else
$(warning warning: sdl2 not found)
endif
NO_SDL2:=1
endif
endif

ifeq ($(NO_PORTAUDIO),1)
else
#LDLIBS   += -lportaudio
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists portaudio-2.0 && echo yes),yes)
CPPFLAGS_PORTAUDIO := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I portaudio-2.0 ) -DMPT_WITH_PORTAUDIO
LDFLAGS_PORTAUDIO  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   portaudio-2.0 ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other portaudio-2.0 )
LDLIBS_PORTAUDIO   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   portaudio-2.0 )
else
ifeq ($(FORCE_DEPS),1)
$(error portaudio not found)
else
$(warning warning: portaudio not found)
endif
NO_PORTAUDIO:=1
endif
endif

ifeq ($(NO_PORTAUDIOCPP),1)
else
#LDLIBS   += -lportaudiocpp
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists portaudiocpp && echo yes),yes)
CPPFLAGS_PORTAUDIOCPP := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I portaudiocpp ) -DMPT_WITH_PORTAUDIOCPP
LDFLAGS_PORTAUDIOCPP  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   portaudiocpp ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other portaudiocpp )
LDLIBS_PORTAUDIOCPP   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   portaudiocpp )
else
ifeq ($(FORCE_DEPS),1)
$(error portaudiocpp not found)
else
$(warning warning: portaudiocpp not found)
endif
NO_PORTAUDIOCPP:=1
endif
endif

ifeq ($(NO_PULSEAUDIO),1)
else
#LDLIBS   += -lpulse-simple
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists libpulse libpulse-simple && echo yes),yes)
CPPFLAGS_PULSEAUDIO := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I libpulse libpulse-simple ) -DMPT_WITH_PULSEAUDIO
LDFLAGS_PULSEAUDIO  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   libpulse libpulse-simple ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other libpulse libpulse-simple )
LDLIBS_PULSEAUDIO   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   libpulse libpulse-simple )
else
ifeq ($(FORCE_DEPS),1)
$(error pulseaudio not found)
else
$(warning warning: pulseaudio not found)
endif
NO_PULSEAUDIO:=1
endif
endif

ifeq ($(NO_FLAC),1)
else
#LDLIBS   += -lFLAC
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists 'flac >= 1.3.0' && echo yes),yes)
CPPFLAGS_FLAC := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I 'flac >= 1.3.0' ) -DMPT_WITH_FLAC
LDFLAGS_FLAC  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L   'flac >= 1.3.0' ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other 'flac >= 1.3.0' )
LDLIBS_FLAC   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l   'flac >= 1.3.0' )
else
ifeq ($(FORCE_DEPS),1)
$(error flac not found)
else
$(warning warning: flac not found)
endif
NO_FLAC:=1
endif
endif

ifeq ($(NO_SNDFILE),1)
else
#LDLIBS   += -lsndfile
ifeq ($(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --exists sndfile && echo yes),yes)
CPPFLAGS_SNDFILE := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --cflags-only-I   sndfile ) -DMPT_WITH_SNDFILE
LDFLAGS_SNDFILE  := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-L     sndfile ) $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-other sndfile )
LDLIBS_SNDFILE   := $(shell $(PKG_CONFIG)$(TOOLCHAIN_SUFFIX) --libs-only-l     sndfile )
else
ifeq ($(FORCE_DEPS),1)
$(error sndfile not found)
else
$(warning warning: sndfile not found)
endif
NO_SNDFILE:=1
endif
endif

ifeq ($(USE_ALLEGRO42),1)

CPPFLAGS_ALLEGRO42 := -Iinclude/allegro42/include -DALLEGRO_HAVE_STDINT_H -DLONG_LONG="long long" -DMPT_WITH_ALLEGRO42
LDFLAGS_ALLEGRO42 :=
LDLIBS_ALLEGRO42 := include/allegro42/lib/djgpp/liballeg.a
DEPS_ALLEGRO42 := include/allegro42/lib/djgpp/liballeg.a

include/allegro42/lib/djgpp/liballeg.a:
	+cd include/allegro42 && ./xmake.sh clean
	+cd include/allegro42 && ./xmake.sh lib

MISC_OUTPUTS += include/allegro42/lib/djgpp/liballeg.a

endif


ifeq ($(HACK_ARCHIVE_SUPPORT),1)
CPPFLAGS += -DMPT_BUILD_HACK_ARCHIVE_SUPPORT
endif

CPPCHECK_FLAGS += -j $(NUMTHREADS)
CPPCHECK_FLAGS += --std=c11 --std=c++17
CPPCHECK_FLAGS += --library=build/cppcheck/glibc-workarounds.cfg
CPPCHECK_FLAGS += --quiet
CPPCHECK_FLAGS += --enable=warning --inline-suppr --template='{file}:{line}: warning: {severity}: {message} [{id}]'
CPPCHECK_FLAGS += --check-level=exhaustive
CPPCHECK_FLAGS += --suppress=missingIncludeSystem
CPPCHECK_FLAGS += --suppress=uninitMemberVar

CPPCHECK_FLAGS += $(CPPFLAGS)
CPPFLAGS += $(CPPFLAGS_ZLIB) $(CPPFLAGS_MPG123) $(CPPFLAGS_OGG) $(CPPFLAGS_VORBIS) $(CPPFLAGS_VORBISFILE)
LDFLAGS += $(LDFLAGS_ZLIB) $(LDFLAGS_MPG123) $(LDFLAGS_OGG) $(LDFLAGS_VORBIS) $(LDFLAGS_VORBISFILE)
LDLIBS += $(LDLIBS_ZLIB) $(LDLIBS_MPG123) $(LDLIBS_OGG) $(LDLIBS_VORBIS) $(LDLIBS_VORBISFILE)

CPPFLAGS_OPENMPT123 += $(CPPFLAGS_SDL2) $(CPPFLAGS_PORTAUDIO) $(CPPFLAGS_PULSEAUDIO) $(CPPFLAGS_FLAC) $(CPPFLAGS_SNDFILE) $(CPPFLAGS_ALLEGRO42)
LDFLAGS_OPENMPT123  += $(LDFLAGS_SDL2) $(LDFLAGS_PORTAUDIO) $(LDFLAGS_PULSEAUDIO) $(LDFLAGS_FLAC) $(LDFLAGS_SNDFILE) $(LDFLAGS_ALLEGRO42)
LDLIBS_OPENMPT123   += $(LDLIBS_SDL2) $(LDLIBS_PORTAUDIO) $(LDLIBS_PULSEAUDIO) $(LDLIBS_FLAC) $(LDLIBS_SNDFILE) $(LDLIBS_ALLEGRO42)


%: %$(FLAVOUR_O).o
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

%$(FLAVOUR_O).o: %.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.cc) $(OUTPUT_OPTION) $<

%$(FLAVOUR_O).o: %.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<

%.test$(FLAVOUR_O).o: %.cpp
	$(INFO) [CXX-TEST] $<
	$(VERYSILENT)$(CXX) -DLIBOPENMPT_BUILD_TEST $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.test$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.cc) -DLIBOPENMPT_BUILD_TEST $(OUTPUT_OPTION) $<

%.test$(FLAVOUR_O).o: %.c
	$(INFO) [CC-TEST] $<
	$(VERYSILENT)$(CC) -DLIBOPENMPT_BUILD_TEST $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*.test$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) -DLIBOPENMPT_BUILD_TEST $(OUTPUT_OPTION) $<

%.tar.gz: %.tar
	$(INFO) [GZIP] $<
	$(SILENT)gzip --rsyncable --no-name --best > $@ < $<


-include build/dist.mk
DIST_LIBOPENMPT_VERSION_PURE:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)
ifeq ($(MPT_SVNVERSION),)
SVN_INFO:=$(shell svn info . > /dev/null 2>&1 ; echo $$? )
ifeq ($(SVN_INFO),0)
# in svn checkout
MPT_SVNVERSION := $(shell svnversion -n . | tr ':' '-' )
MPT_SVNURL := $(shell svn info --xml | grep '^<url>' | sed 's/<url>//g' | sed 's/<\/url>//g' )
MPT_SVNDATE := $(shell svn info --xml | grep '^<date>' | sed 's/<date>//g' | sed 's/<\/date>//g' )
CPPFLAGS += -D MPT_SVNURL=\"$(MPT_SVNURL)\" -D MPT_SVNVERSION=\"$(MPT_SVNVERSION)\" -D MPT_SVNDATE=\"$(MPT_SVNDATE)\"
DIST_OPENMPT_VERSION:=r$(MPT_SVNVERSION)
ifeq ($(LIBOPENMPT_VERSION_PREREL),)
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+release
else
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+r$(MPT_SVNVERSION)
endif
else
GIT_STATUS:=$(shell git status > /dev/null 2>&1 ; echo $$? )
ifeq ($(GIT_STATUS),0)
# in git chechout
MPT_SVNVERSION := $(shell git log --grep=git-svn-id -n 1 | grep git-svn-id | tail -n 1 | tr ' ' '\n' | tail -n 2 | head -n 1 | sed 's/@/ /g' | awk '{print $$2;}' )$(shell if [ $$(git rev-list $$(git log --grep=git-svn-id -n 1 --format=format:'%H')  ^$$(git log -n 1 --format=format:'%H') --count ) -ne 0 ] ; then  echo M ; fi )
MPT_SVNURL := $(shell git log --grep=git-svn-id -n 1 | grep git-svn-id | tail -n 1 | tr ' ' '\n' | tail -n 2 | head -n 1 | sed 's/@/ /g' | awk '{print $$1;}' )
MPT_SVNDATE := $(shell git log -n 1 --date=iso --format=format:'%cd' | sed 's/ +0000/Z/g' | tr ' ' 'T' )
CPPFLAGS += -D MPT_SVNURL=\"$(MPT_SVNURL)\" -D MPT_SVNVERSION=\"$(MPT_SVNVERSION)\" -D MPT_SVNDATE=\"$(MPT_SVNDATE)\"
DIST_OPENMPT_VERSION:=r$(MPT_SVNVERSION)
ifeq ($(LIBOPENMPT_VERSION_PREREL),)
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+release
else
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+r$(MPT_SVNVERSION)
endif
else
# not in svn checkout
DIST_OPENMPT_VERSION:=rUNKNOWN
ifeq ($(LIBOPENMPT_VERSION_PREREL),)
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+release
else
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+rUNKNOWN
endif
endif
endif
else
# in dist package
DIST_OPENMPT_VERSION:=r$(MPT_SVNVERSION)
ifeq ($(LIBOPENMPT_VERSION_PREREL),)
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+release
else
DIST_LIBOPENMPT_VERSION:=$(LIBOPENMPT_VERSION_MAJOR).$(LIBOPENMPT_VERSION_MINOR).$(LIBOPENMPT_VERSION_PATCH)$(LIBOPENMPT_VERSION_PREREL)+r$(MPT_SVNVERSION)
endif
endif
DIST_LIBOPENMPT_TARBALL_VERSION:=$(DIST_LIBOPENMPT_VERSION_PURE)

ifeq ($(MPT_SVNVERSION),)
else
MPT_SVNREVISION := $(shell echo $(MPT_SVNVERSION) | sed 's/M//g' | sed 's/S//g' | sed 's/P//g' | sed -E 's/([0-9]+)-//g' )
MPT_SVNDIRTY := $(shell echo $(MPT_SVNVERSION) | grep -v 'M\|S\|P' >/dev/null 2>&1 ; echo $$? )
MPT_SVNMIXED := $(shell echo $(MPT_SVNVERSION) | grep -v '-' >/dev/null 2>&1; echo $$? )
endif


CPPFLAGS += -DLIBOPENMPT_BUILD


COMMON_CXX_SOURCES += \
 $(sort $(wildcard src/openmpt/all/*.cpp)) \
 $(sort $(wildcard src/openmpt/base/*.cpp)) \
 $(sort $(wildcard src/openmpt/logging/*.cpp)) \
 $(sort $(wildcard src/openmpt/random/*.cpp)) \
 $(sort $(wildcard common/*.cpp)) \
 
SOUNDLIB_CXX_SOURCES += \
 $(COMMON_CXX_SOURCES) \
 $(sort $(wildcard src/openmpt/soundbase/*.cpp)) \
 $(sort $(wildcard soundlib/*.cpp)) \
 $(sort $(wildcard soundlib/plugins/*.cpp)) \
 $(sort $(wildcard soundlib/plugins/dmo/*.cpp)) \
 $(sort $(wildcard sounddsp/*.cpp)) \
 

ifeq ($(HACK_ARCHIVE_SUPPORT),1)
SOUNDLIB_CXX_SOURCES += $(sort $(wildcard unarchiver/*.cpp))
endif

LIBOPENMPT_CXX_SOURCES += \
 $(SOUNDLIB_CXX_SOURCES) \
 $(sort $(wildcard libopenmpt/*.cpp)) \
 
include/miniz/miniz$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
include/miniz/miniz.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
ifeq ($(LOCAL_ZLIB),1)
LIBOPENMPT_C_SOURCES += $(LOCAL_ZLIB_SOURCES)
LIBOPENMPTTEST_C_SOURCES += $(LOCAL_ZLIB_SOURCES)
else
ifeq ($(NO_ZLIB),1)
ifeq ($(NO_MINIZ),1)
else
LIBOPENMPT_C_SOURCES += include/miniz/miniz.c
LIBOPENMPTTEST_C_SOURCES += include/miniz/miniz.c
CPPFLAGS += -DMPT_WITH_MINIZ
CPPFLAGS += -Iinclude
endif
endif
endif

include/minimp3/minimp3$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
include/minimp3/minimp3.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
ifeq ($(LOCAL_MPG123),1)
LIBOPENMPT_C_SOURCES += $(LOCAL_MPG123_SOURCES)
LIBOPENMPTTEST_C_SOURCES += $(LOCAL_MPG123_SOURCES)
else
ifeq ($(NO_MPG123),1)
ifeq ($(NO_MINIMP3),1)
else
LIBOPENMPT_C_SOURCES += include/minimp3/minimp3.c
LIBOPENMPTTEST_C_SOURCES += include/minimp3/minimp3.c
CPPFLAGS += -DMPT_WITH_MINIMP3
CPPFLAGS += -Iinclude
endif
endif
endif

include/stb_vorbis/stb_vorbis$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
include/stb_vorbis/stb_vorbis.test$(FLAVOUR_O).o : CFLAGS+=$(CFLAGS_SILENT)
ifeq ($(LOCAL_VORBIS),1)
ifeq ($(LOCAL_OGG),1)
LIBOPENMPT_C_SOURCES += $(LOCAL_OGG_SOURCES)
LIBOPENMPTTEST_C_SOURCES += $(LOCAL_OGG_SOURCES)
endif
LIBOPENMPT_C_SOURCES += $(LOCAL_VORBIS_SOURCES)
LIBOPENMPTTEST_C_SOURCES += $(LOCAL_VORBIS_SOURCES)
else
ifeq ($(NO_OGG),1)
ifeq ($(NO_STBVORBIS),1)
else
LIBOPENMPT_C_SOURCES += include/stb_vorbis/stb_vorbis.c
LIBOPENMPTTEST_C_SOURCES += include/stb_vorbis/stb_vorbis.c
CPPFLAGS += -DMPT_WITH_STBVORBIS -DSTB_VORBIS_NO_PULLDATA_API -DSTB_VORBIS_NO_STDIO
CPPFLAGS += -Iinclude
endif
else
ifeq ($(NO_VORBIS),1)
ifeq ($(NO_STBVORBIS),1)
else
LIBOPENMPT_C_SOURCES += include/stb_vorbis/stb_vorbis.c
LIBOPENMPTTEST_C_SOURCES += include/stb_vorbis/stb_vorbis.c
CPPFLAGS += -DMPT_WITH_STBVORBIS -DSTB_VORBIS_NO_PULLDATA_API -DSTB_VORBIS_NO_STDIO
CPPFLAGS += -Iinclude
endif
else
ifeq ($(NO_VORBISFILE),1)
ifeq ($(NO_STBVORBIS),1)
else
LIBOPENMPT_C_SOURCES += include/stb_vorbis/stb_vorbis.c
LIBOPENMPTTEST_C_SOURCES += include/stb_vorbis/stb_vorbis.c
CPPFLAGS += -DMPT_WITH_STBVORBIS -DSTB_VORBIS_NO_PULLDATA_API -DSTB_VORBIS_NO_STDIO
CPPFLAGS += -Iinclude
endif
else
endif
endif
endif
endif

LIBOPENMPT_OBJECTS += $(LIBOPENMPT_CXX_SOURCES:.cpp=$(FLAVOUR_O).o) $(LIBOPENMPT_C_SOURCES:.c=$(FLAVOUR_O).o)
LIBOPENMPT_DEPENDS = $(LIBOPENMPT_OBJECTS:$(FLAVOUR_O).o=$(FLAVOUR_O).d)
ALL_OBJECTS += $(LIBOPENMPT_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPT_DEPENDS)

ifeq ($(DYNLINK),1)
OUTPUT_LIBOPENMPT += bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX)
else
OBJECTS_LIBOPENMPT += $(LIBOPENMPT_OBJECTS)
endif


INOPENMPT_CXX_SOURCES += \
 libopenmpt/plugin-common/libopenmpt_plugin_gui.cpp \
 libopenmpt/in_openmpt/in_openmpt.cpp \
 

INOPENMPT_OBJECTS += $(INOPENMPT_CXX_SOURCES:.cpp=$(FLAVOUR_O).o) $(INOPENMPT_C_SOURCES:.c=$(FLAVOUR_O).o)
INOPENMPT_DEPENDS = $(INOPENMPT_OBJECTS:$(FLAVOUR_O).o=$(FLAVOUR_O).d)
ALL_OBJECTS += $(INOPENMPT_OBJECTS)
ALL_DEPENDS += $(INOPENMPT_DEPENDS)


XMPOPENMPT_CXX_SOURCES += \
 include/pugixml/src/pugixml.cpp \
 libopenmpt/plugin-common/libopenmpt_plugin_gui.cpp \
 libopenmpt/xmp-openmpt/xmp-openmpt.cpp \
 

XMPOPENMPT_OBJECTS += $(XMPOPENMPT_CXX_SOURCES:.cpp=$(FLAVOUR_O).o) $(XMPOPENMPT_C_SOURCES:.c=$(FLAVOUR_O).o)
XMPOPENMPT_DEPENDS = $(XMPOPENMPT_OBJECTS:$(FLAVOUR_O).o=$(FLAVOUR_O).d)
ALL_OBJECTS += $(XMPOPENMPT_OBJECTS)
ALL_DEPENDS += $(XMPOPENMPT_DEPENDS)


OPENMPT123_CXX_SOURCES += \
 $(sort $(wildcard openmpt123/*.cpp)) \
 
OPENMPT123_C_SOURCES += \
 $(sort $(wildcard openmpt123/*.c)) \
 
OPENMPT123_OBJECTS += $(OPENMPT123_CXX_SOURCES:.cpp=$(FLAVOUR_O).o) $(OPENMPT123_C_SOURCES:.c=$(FLAVOUR_O).o)
OPENMPT123_DEPENDS = $(OPENMPT123_OBJECTS:$(FLAVOUR_O).o=$(FLAVOUR_O).d)
ALL_OBJECTS += $(OPENMPT123_OBJECTS)
ALL_DEPENDS += $(OPENMPT123_DEPENDS)


LIBOPENMPTTEST_CXX_SOURCES += \
 test/libopenmpt_test.cpp \
 $(SOUNDLIB_CXX_SOURCES) \
 test/mpt_tests_base.cpp \
 test/mpt_tests_binary.cpp \
 test/mpt_tests_crc.cpp \
 test/mpt_tests_endian.cpp \
 test/mpt_tests_format.cpp \
 test/mpt_tests_io.cpp \
 test/mpt_tests_parse.cpp \
 test/mpt_tests_random.cpp \
 test/mpt_tests_string.cpp \
 test/mpt_tests_string_transcode.cpp \
 test/mpt_tests_uuid.cpp \
 test/test.cpp \
 test/TestToolsLib.cpp \
 
# test/mpt_tests_crypto.cpp \
# test/mpt_tests_uuid_namespace.cpp \

LIBOPENMPTTEST_C_SOURCES += \
 
LIBOPENMPTTEST_OBJECTS = $(LIBOPENMPTTEST_CXX_SOURCES:.cpp=.test$(FLAVOUR_O).o) $(LIBOPENMPTTEST_C_SOURCES:.c=.test$(FLAVOUR_O).o)
LIBOPENMPTTEST_DEPENDS = $(LIBOPENMPTTEST_CXX_SOURCES:.cpp=.test$(FLAVOUR_O).d) $(LIBOPENMPTTEST_C_SOURCES:.c=.test$(FLAVOUR_O).d)
ALL_OBJECTS += $(LIBOPENMPTTEST_OBJECTS)
ALL_DEPENDS += $(LIBOPENMPTTEST_DEPENDS)


EXAMPLES_CXX_SOURCES += $(sort $(wildcard examples/*.cpp))
EXAMPLES_C_SOURCES += $(sort $(wildcard examples/*.c))

EXAMPLES_OBJECTS += $(EXAMPLES_CXX_SOURCES:.cpp=$(FLAVOUR_O).o)
EXAMPLES_OBJECTS += $(EXAMPLES_C_SOURCES:.c=$(FLAVOUR_O).o)
EXAMPLES_DEPENDS = $(EXAMPLES_OBJECTS:$(FLAVOUR_O).o=$(FLAVOUR_O).d)
ALL_OBJECTS += $(EXAMPLES_OBJECTS)
ALL_DEPENDS += $(EXAMPLES_DEPENDS)


FUZZ_CXX_SOURCES += $(sort $(wildcard contrib/fuzzing/*.cpp))
FUZZ_C_SOURCES += $(sort $(wildcard contrib/fuzzing/*.c))

FUZZ_OBJECTS += $(FUZZ_CXX_SOURCES:.cpp=$(FLAVOUR_O).o)
FUZZ_OBJECTS += $(FUZZ_C_SOURCES:.c=$(FLAVOUR_O).o)
FUZZ_DEPENDS = $(FUZZ_OBJECTS:$(FLAVOUR_O).o=$(FLAVOUR_O).d)
ALL_OBJECTS += $(FUZZ_OBJECTS)
ALL_DEPENDS += $(FUZZ_DEPENDS)


.PHONY: all
all:

-include $(ALL_DEPENDS)

ifeq ($(DYNLINK),1)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX)
endif
ifeq ($(SHARED_LIB),1)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX)
endif
ifeq ($(STATIC_LIB),1)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt.a
endif
ifeq ($(IN_OPENMPT),1)
OUTPUTS += bin/$(FLAVOUR_DIR)in_openmpt$(SOSUFFIX)
endif
ifeq ($(XMP_OPENMPT),1)
OUTPUTS += bin/$(FLAVOUR_DIR)xmp-openmpt$(SOSUFFIX)
endif
ifeq ($(OPENMPT123),1)
OUTPUTS += bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX)
endif
ifeq ($(EXAMPLES),1)
ifeq ($(NO_PORTAUDIO),1)
else
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c$(EXESUFFIX)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_mem$(EXESUFFIX)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_unsafe$(EXESUFFIX)
endif
ifeq ($(NO_PORTAUDIOCPP),1)
else
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_cxx$(EXESUFFIX)
endif
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_pipe$(EXESUFFIX)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_stdout$(EXESUFFIX)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_probe$(EXESUFFIX)
endif
ifeq ($(FUZZ),1)
OUTPUTS += bin/$(FLAVOUR_DIR)fuzz$(EXESUFFIX)
endif
ifeq ($(TEST),1)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_test$(EXESUFFIX)
endif
ifeq ($(HOST),unix)
OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt.pc
endif
ifeq ($(OPENMPT123),1)
ifeq ($(MPT_WITH_HELP2MAN),1)
OUTPUTS += bin/$(FLAVOUR_DIR)openmpt123.1
endif
endif
ifeq ($(SHARED_SONAME),1)
LIBOPENMPT_LDFLAGS += -Wl,-soname,$(LIBOPENMPT_SONAME)
endif

MISC_OUTPUTS += bin/$(FLAVOUR_DIR)empty.cpp
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)empty.out
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_mem$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_probe$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_unsafe$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_cxx$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_pipe$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_stdout$(EXESUFFIX).norpath
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX)
MISC_OUTPUTS += bin/$(FLAVOUR_DIR).docs
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_test$(EXESUFFIX)
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_test.wasm
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_test.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_test.js.mem
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)made.docs
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)$(LIBOPENMPT_SONAME)
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt.wasm
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt.js.mem
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c.wasm 
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c.js.mem 
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_mem.wasm 
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_mem.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_mem.js.mem 
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_pipe.wasm
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_pipe.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_pipe.js.mem
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_probe.wasm
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_probe.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_probe.js.mem
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_stdout.wasm
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_stdout.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_stdout.js.mem
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_unsafe.wasm
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_unsafe.wasm.js
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_unsafe.js.mem
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)openmpt.a
#old
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_safe$(EXESUFFIX)
MISC_OUTPUTS += bin/$(FLAVOUR_DIR)libopenmpt_example_c_safe$(EXESUFFIX).norpath

MISC_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dest
MISC_OUTPUTDIRS += bin/$(FLAVOUR_DIR)docs

DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist.mk
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)svn_version_dist.h
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist-tar.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist-zip.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist-doc.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist-autotools.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist-js.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)dist-dos.tar
DIST_OUTPUTS += bin/$(FLAVOUR_DIR)made.docs

DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist-doc
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist-tar
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist-zip
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist-autotools
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist-js
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)dist-dos
DIST_OUTPUTDIRS += bin/$(FLAVOUR_DIR)docs



ifeq ($(ONLY_TEST),1)
all: bin/$(FLAVOUR_DIR)libopenmpt_test$(EXESUFFIX)
else
all: $(OUTPUTS)
endif

.PHONY: docs
docs: bin/$(FLAVOUR_DIR)made.docs

.PHONY: doc
doc: bin/$(FLAVOUR_DIR)made.docs

bin/$(FLAVOUR_DIR)made.docs:
	$(VERYSILENT)mkdir -p bin/$(FLAVOUR_DIR)docs
	$(INFO) [DOXYGEN] libopenmpt
ifeq ($(SILENT_DOCS),1)
	$(SILENT) ( cat libopenmpt/Doxyfile ; echo 'PROJECT_NUMBER = "$(DIST_LIBOPENMPT_VERSION)"' ; echo 'WARN_IF_DOC_ERROR = NO' ) | doxygen -
else
	$(SILENT) ( cat libopenmpt/Doxyfile ; echo 'PROJECT_NUMBER = "$(DIST_LIBOPENMPT_VERSION)"' ) | doxygen -
endif
	$(VERYSILENT)touch $@

.PHONY: check
check: test

.PHONY: test
test: bin/$(FLAVOUR_DIR)libopenmpt_test$(EXESUFFIX)
ifeq ($(REQUIRES_RUNPREFIX),1)
	cd bin/$(FLAVOUR_DIR) && $(RUNPREFIX) libopenmpt_test$(EXESUFFIX)
else
	bin/$(FLAVOUR_DIR)libopenmpt_test$(EXESUFFIX)
endif

bin/$(FLAVOUR_DIR)libopenmpt_test$(EXESUFFIX): $(LIBOPENMPTTEST_OBJECTS) 
	$(INFO) [LD-TEST] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(TEST_LDFLAGS) $(LIBOPENMPTTEST_OBJECTS) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPTTEST) -o $@

bin/$(FLAVOUR_DIR)libopenmpt.pc:
	$(INFO) [GEN] $@
	$(VERYSILENT)rm -rf $@
	$(VERYSILENT)echo > $@.tmp
	$(VERYSILENT)echo 'prefix=$(PREFIX)' >> $@.tmp
	$(VERYSILENT)echo 'exec_prefix=$${prefix}' >> $@.tmp
	$(VERYSILENT)echo 'libdir=$${exec_prefix}/lib' >> $@.tmp
	$(VERYSILENT)echo 'includedir=$${prefix}/include' >> $@.tmp
	$(VERYSILENT)echo '' >> $@.tmp
	$(VERYSILENT)echo 'Name: libopenmpt' >> $@.tmp
	$(VERYSILENT)echo 'Description: Tracker module player based on OpenMPT' >> $@.tmp
	$(VERYSILENT)echo 'Version: $(DIST_LIBOPENMPT_VERSION)' >> $@.tmp
	$(VERYSILENT)echo 'Requires.private: $(PC_REQUIRES_ZLIB) $(PC_REQUIRES_MPG123) $(PC_REQUIRES_OGG) $(PC_REQUIRES_VORBIS) $(PC_REQUIRES_VORBISFILE)' >> $@.tmp
	$(VERYSILENT)echo 'Libs: -L$${libdir} -lopenmpt' >> $@.tmp
	$(VERYSILENT)echo 'Libs.private: $(PC_LIBS_PRIVATE)' >> $@.tmp
	$(VERYSILENT)echo 'Cflags: -I$${includedir}' >> $@.tmp
	$(VERYSILENT)mv $@.tmp $@

.PHONY: install
install: $(OUTPUTS)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/include/libopenmpt
	$(INSTALL_DATA) libopenmpt/libopenmpt_config.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_config.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_version.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_version.h
	$(INSTALL_DATA) libopenmpt/libopenmpt.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_buffer.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_buffer.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_fd.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_fd.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_file.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_file.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_file_mingw.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_file_mingw.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_file_msvcrt.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_file_msvcrt.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_file_posix.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_file_posix.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_stream_callbacks_file_posix_lfs64.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_stream_callbacks_file_posix_lfs64.h
	$(INSTALL_DATA) libopenmpt/libopenmpt.hpp $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt.hpp
	$(INSTALL_DATA) libopenmpt/libopenmpt_ext.h $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_ext.h
	$(INSTALL_DATA) libopenmpt/libopenmpt_ext.hpp $(DESTDIR)$(PREFIX)/include/libopenmpt/libopenmpt_ext.hpp
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib/pkgconfig
	$(INSTALL_DATA) bin/$(FLAVOUR_DIR)libopenmpt.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/libopenmpt.pc
ifeq ($(SHARED_LIB),1)
ifeq ($(SHARED_SONAME),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_LIB) bin/$(FLAVOUR_DIR)$(LIBOPENMPT_SONAME) $(DESTDIR)$(PREFIX)/lib/$(LIBOPENMPT_SONAME)
	ln -sf $(LIBOPENMPT_SONAME) $(DESTDIR)$(PREFIX)/lib/libopenmpt$(SOSUFFIX)
else
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_LIB) bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX) $(DESTDIR)$(PREFIX)/lib/libopenmpt$(SOSUFFIX)
endif
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
endif
ifeq ($(STATIC_LIB),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/lib
	$(INSTALL_DATA) bin/$(FLAVOUR_DIR)libopenmpt.a $(DESTDIR)$(PREFIX)/lib/libopenmpt.a
endif
ifeq ($(OPENMPT123),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/bin
ifeq ($(SHARED_LIB),1)
	$(INSTALL_PROGRAM) bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX).norpath $(DESTDIR)$(PREFIX)/bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX)
else
	$(INSTALL_PROGRAM) bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX) $(DESTDIR)$(PREFIX)/bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX)
endif
ifeq ($(MPT_WITH_HELP2MAN),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_DATA) bin/$(FLAVOUR_DIR)openmpt123.1 $(DESTDIR)$(MANDIR)/man1/openmpt123.1
endif
endif
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/share/doc/libopenmpt
	$(INSTALL_DATA) LICENSE   $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/LICENSE
	$(INSTALL_DATA) README.md $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/README.md
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples
	$(INSTALL_DATA) examples/libopenmpt_example_c.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_mem.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_mem.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_probe.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_probe.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_unsafe.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_unsafe.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_pipe.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_pipe.c
	$(INSTALL_DATA) examples/libopenmpt_example_c_stdout.c $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_c_stdout.c
	$(INSTALL_DATA) examples/libopenmpt_example_cxx.cpp $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/examples/libopenmpt_example_cxx.cpp

.PHONY: install-doc
install-doc: bin/$(FLAVOUR_DIR)made.docs
ifeq ($(MPT_WITH_DOXYGEN),1)
	$(INSTALL_MAKE_DIR) $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/html/
	$(INSTALL_DATA_DIR) bin/$(FLAVOUR_DIR)docs/html $(DESTDIR)$(PREFIX)/share/doc/libopenmpt/html
endif

.PHONY: dist
dist: bin/$(FLAVOUR_DIR)dist-tar.tar bin/$(FLAVOUR_DIR)dist-zip.tar bin/$(FLAVOUR_DIR)dist-doc.tar

.PHONY: dist-tar
dist-tar: bin/$(FLAVOUR_DIR)dist-tar.tar

bin/$(FLAVOUR_DIR)dist-tar.tar: bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).makefile.tar.gz
	rm -rf bin/$(FLAVOUR_DIR)dist-tar.tar
	cd bin/$(FLAVOUR_DIR)dist-tar/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-tar/ && mkdir -p libopenmpt/src.makefile/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-tar/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).makefile.tar.gz libopenmpt/src.makefile/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-tar/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-tar.tar libopenmpt

.PHONY: dist-zip
dist-zip: bin/$(FLAVOUR_DIR)dist-zip.tar

bin/$(FLAVOUR_DIR)dist-zip.tar: bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION).msvc.zip
	rm -rf bin/$(FLAVOUR_DIR)dist-zip.tar
	cd bin/$(FLAVOUR_DIR)dist-zip/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-zip/ && mkdir -p libopenmpt/src.msvc/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-zip/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).msvc.zip libopenmpt/src.msvc/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-zip/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-zip.tar libopenmpt

.PHONY: dist-doc
dist-doc: bin/$(FLAVOUR_DIR)dist-doc.tar

bin/$(FLAVOUR_DIR)dist-doc.tar: bin/$(FLAVOUR_DIR)dist-doc/libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc.tar.gz
	rm -rf bin/$(FLAVOUR_DIR)dist-doc.tar
	cd bin/$(FLAVOUR_DIR)dist-doc/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-doc/ && mkdir -p libopenmpt/doc/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-doc/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc.tar.gz libopenmpt/doc/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-doc/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-doc.tar libopenmpt

.PHONY: dist-js
dist-js: bin/$(FLAVOUR_DIR)dist-js.tar

bin/$(FLAVOUR_DIR)dist-js.tar: bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION).dev.js.tar.gz
	rm -rf bin/$(FLAVOUR_DIR)dist-js.tar
	cd bin/$(FLAVOUR_DIR)dist-js/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-js/ && mkdir -p libopenmpt/dev.js/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-js/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).dev.js.tar.gz libopenmpt/dev.js/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-js/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-js.tar libopenmpt

.PHONY: dist-dos
dist-dos: bin/$(FLAVOUR_DIR)dist-dos.tar

bin/$(FLAVOUR_DIR)dist-dos.tar: bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.dos.zip
	rm -rf bin/$(FLAVOUR_DIR)dist-dos.tar
	cd bin/$(FLAVOUR_DIR)dist-dos/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-dos/ && mkdir -p libopenmpt/bin.dos/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-dos/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.dos.zip libopenmpt/bin.dos/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-dos/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-dos.tar libopenmpt

.PHONY: dist-retro-win98
dist-retro-win98: bin/$(FLAVOUR_DIR)dist-retro-win98.tar

bin/$(FLAVOUR_DIR)dist-retro-win98.tar: bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win98.zip
	rm -rf bin/$(FLAVOUR_DIR)dist-retro-win98.tar
	cd bin/$(FLAVOUR_DIR)dist-retro-win98/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-retro-win98/ && mkdir -p libopenmpt/bin.retro.win98/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-retro-win98/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win98.zip libopenmpt/bin.retro.win98/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-retro-win98/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-retro-win98.tar libopenmpt

.PHONY: dist-retro-win95
dist-retro-win95: bin/$(FLAVOUR_DIR)dist-retro-win95.tar

bin/$(FLAVOUR_DIR)dist-retro-win95.tar: bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win95.zip
	rm -rf bin/$(FLAVOUR_DIR)dist-retro-win95.tar
	cd bin/$(FLAVOUR_DIR)dist-retro-win95/ && rm -rf libopenmpt
	cd bin/$(FLAVOUR_DIR)dist-retro-win95/ && mkdir -p libopenmpt/bin.retro.win95/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-retro-win95/ && cp libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win95.zip libopenmpt/bin.retro.win95/$(DIST_LIBOPENMPT_TARBALL_VERSION)/
	cd bin/$(FLAVOUR_DIR)dist-retro-win95/ && tar cv --numeric-owner --owner=0 --group=0 -f ../dist-retro-win95.tar libopenmpt

.PHONY: bin/$(FLAVOUR_DIR)dist.mk
bin/$(FLAVOUR_DIR)dist.mk:
	rm -rf $@
	echo > $@.tmp
	echo 'MPT_SVNVERSION=$(MPT_SVNVERSION)' >> $@.tmp
	echo 'MPT_SVNURL=$(MPT_SVNURL)' >> $@.tmp
	echo 'MPT_SVNDATE=$(MPT_SVNDATE)' >> $@.tmp
	mv $@.tmp $@

.PHONY: bin/$(FLAVOUR_DIR)svn_version_dist.h
bin/$(FLAVOUR_DIR)svn_version_dist.h:
	rm -rf $@
	echo > $@.tmp
	echo '#pragma once' >> $@.tmp
	echo '#define OPENMPT_VERSION_SVNVERSION "$(MPT_SVNVERSION)"' >> $@.tmp
	echo '#define OPENMPT_VERSION_REVISION $(MPT_SVNREVISION)' >> $@.tmp
	echo '#define OPENMPT_VERSION_DIRTY $(MPT_SVNDIRTY)' >> $@.tmp
	echo '#define OPENMPT_VERSION_MIXEDREVISIONS $(MPT_SVNMIXED)' >> $@.tmp
	echo '#define OPENMPT_VERSION_URL "$(MPT_SVNURL)"' >> $@.tmp
	echo '#define OPENMPT_VERSION_DATE "$(MPT_SVNDATE)"' >> $@.tmp
	echo '#define OPENMPT_VERSION_IS_PACKAGE 1' >> $@.tmp
	echo >> $@.tmp
	mv $@.tmp $@

.PHONY: bin/$(FLAVOUR_DIR)dist-doc/libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc.tar
bin/$(FLAVOUR_DIR)dist-doc/libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc.tar: docs
	mkdir -p bin/$(FLAVOUR_DIR)dist-doc
	rm -rf bin/$(FLAVOUR_DIR)dist-doc/libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc
	mkdir -p bin/$(FLAVOUR_DIR)dist-doc/libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc
	cp -Rv bin/$(FLAVOUR_DIR)docs/html bin/$(FLAVOUR_DIR)dist-doc/libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc/docs
	cd bin/$(FLAVOUR_DIR)dist-doc/ && tar cv --numeric-owner --owner=0 --group=0 libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc > libopenmpt-$(DIST_LIBOPENMPT_VERSION).doc.tar

.PHONY: bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).makefile.tar
bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION).makefile.tar: bin/$(FLAVOUR_DIR)dist.mk bin/$(FLAVOUR_DIR)svn_version_dist.h
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar
	rm -rf bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt
	mkdir -p bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt
	svn export ./LICENSE            bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE
	svn export ./README.md          bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/README.md
	svn export ./Makefile           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Makefile
	svn export ./.clang-format      bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/.clang-format
	svn export ./bin                bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin
	svn export ./build/download_externals.sh bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/download_externals.sh
	svn export ./build/android_ndk  bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/android_ndk
	svn export ./build/make         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/make
	svn export ./build/svn_version  bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/svn_version
	svn export ./build/xcode-ios    bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/xcode-ios
	svn export ./build/xcode-macosx bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/xcode-macosx
	svn export ./common             bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/common
	svn export ./doc/contributing.md          bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc/contributing.md
	svn export ./doc/libopenmpt_styleguide.md bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc/libopenmpt_styleguide.md
	svn export ./doc/module_formats.md        bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc/module_formats.md
	svn export ./soundlib           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/soundlib
	svn export ./sounddsp           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/sounddsp
	svn export ./src/mpt/.clang-format bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/.clang-format
	svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/LICENSE.BSD-3-Clause.txt
	svn export ./src/mpt/LICENSE.BSL-1.0.txt bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/LICENSE.BSL-1.0.txt
	svn export ./src/mpt/arch           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/arch
	svn export ./src/mpt/audio          bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/audio
	svn export ./src/mpt/base           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/base
	svn export ./src/mpt/binary         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/binary
	svn export ./src/mpt/check          bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/check
	svn export ./src/mpt/crc            bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/crc
	#svn export ./src/mpt/crypto         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/crypto
	svn export ./src/mpt/detect         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/detect
	svn export ./src/mpt/endian         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/endian
	svn export ./src/mpt/environment    bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/environment
	svn export ./src/mpt/exception      bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/exception
	svn export ./src/mpt/format         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/format
	#svn export ./src/mpt/fs             bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/fs
	svn export ./src/mpt/io             bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io
	svn export ./src/mpt/io_file        bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file
	svn export ./src/mpt/io_file_adapter bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file_adapter
	svn export ./src/mpt/io_file_read   bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file_read
	svn export ./src/mpt/io_file_unique bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file_unique
	svn export ./src/mpt/io_read        bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_read
	svn export ./src/mpt/io_write       bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_write
	#svn export ./src/mpt/json           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/json
	#svn export ./src/mpt/library        bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/library
	svn export ./src/mpt/mutex          bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/mutex
	svn export ./src/mpt/out_of_memory  bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/out_of_memory
	svn export ./src/mpt/osinfo         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/osinfo
	svn export ./src/mpt/parse          bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/parse
	svn export ./src/mpt/path           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/path
	svn export ./src/mpt/random         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/random
	svn export ./src/mpt/string         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/string
	svn export ./src/mpt/string_transcode bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/string_transcode
	svn export ./src/mpt/system_error   bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/system_error
	svn export ./src/mpt/test           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/test
	svn export ./src/mpt/uuid           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/uuid
	#svn export ./src/mpt/uuid_namespace bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/uuid_namespace
	svn export ./src/openmpt/all        bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/all
	svn export ./src/openmpt/base       bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/base
	svn export ./src/openmpt/logging    bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/logging
	svn export ./src/openmpt/random     bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/random
	svn export ./src/openmpt/soundbase  bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/soundbase
	svn export ./test               bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test
	rm bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test/mpt_tests_crypto.cpp
	rm bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test/mpt_tests_uuid_namespace.cpp
	svn export ./libopenmpt         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/libopenmpt
	svn export ./examples           bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/examples
	svn export ./openmpt123         bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123
	svn export ./contrib            bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/contrib
	svn export ./include/allegro42  bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/allegro42
	svn export ./include/cwsdpmi    bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/cwsdpmi
	svn export ./include/minimp3    bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/minimp3
	svn export ./include/miniz      bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/miniz
	svn export ./include/stb_vorbis bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/stb_vorbis
	cp bin/$(FLAVOUR_DIR)dist.mk bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/dist.mk
	cp bin/$(FLAVOUR_DIR)svn_version_dist.h bin/$(FLAVOUR_DIR)dist-tar/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/svn_version/svn_version.h
	cd bin/$(FLAVOUR_DIR)dist-tar/ && tar cv --numeric-owner --owner=0 --group=0 libopenmpt-$(DIST_LIBOPENMPT_VERSION) > libopenmpt-$(DIST_LIBOPENMPT_VERSION).makefile.tar

.PHONY: bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION).msvc.zip
bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION).msvc.zip: bin/$(FLAVOUR_DIR)dist.mk bin/$(FLAVOUR_DIR)svn_version_dist.h
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip
	rm -rf bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/premake
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt
	svn export ./LICENSE               bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE               --native-eol CRLF
	svn export ./README.md             bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/README.md             --native-eol CRLF
	svn export ./Makefile              bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Makefile              --native-eol CRLF
	svn export ./.clang-format         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/.clang-format         --native-eol CRLF
	svn export ./bin                   bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin                   --native-eol CRLF
	svn export ./build/premake/def            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/premake/def            --native-eol CRLF
	svn export ./build/premake/inc            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/premake/inc            --native-eol CRLF
	svn export ./build/premake/lnk            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/premake/lnk            --native-eol CRLF
	svn export ./build/scriptlib              bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/scriptlib              --native-eol CRLF
	svn export ./build/svn_version            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/svn_version            --native-eol CRLF
	svn export ./build/vs                     bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs                     --native-eol CRLF
	svn export ./build/vs2017winxpansi        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2017winxpansi        --native-eol CRLF
	svn export ./build/vs2017winxp            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2017winxp            --native-eol CRLF
	svn export ./build/vs2019win7             bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2019win7             --native-eol CRLF
	svn export ./build/vs2019win81            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2019win81            --native-eol CRLF
	svn export ./build/vs2019win10            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2019win10            --native-eol CRLF
	svn export ./build/vs2019win10uwp         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2019win10uwp         --native-eol CRLF
	svn export ./build/vs2022win7             bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2022win7             --native-eol CRLF
	svn export ./build/vs2022win81            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2022win81            --native-eol CRLF
	svn export ./build/vs2022win10            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2022win10            --native-eol CRLF
	svn export ./build/vs2022win10uwp         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2022win10uwp         --native-eol CRLF
	svn export ./build/vs2022win10clang       bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/vs2022win10clang       --native-eol CRLF
	svn export ./build/download_externals.cmd bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/download_externals.cmd --native-eol CRLF
	svn export ./common                bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/common                --native-eol CRLF
	svn export ./doc/contributing.md          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc/contributing.md          --native-eol CRLF
	svn export ./doc/libopenmpt_styleguide.md bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc/libopenmpt_styleguide.md --native-eol CRLF
	svn export ./doc/module_formats.md        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/doc/module_formats.md        --native-eol CRLF
	svn export ./soundlib              bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/soundlib              --native-eol CRLF
	svn export ./sounddsp              bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/sounddsp              --native-eol CRLF
	svn export ./src/mpt/.clang-format bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/.clang-format --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/LICENSE.BSD-3-Clause.txt --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSL-1.0.txt bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/LICENSE.BSL-1.0.txt --native-eol CRLF
	svn export ./src/mpt/arch           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/arch           --native-eol CRLF
	svn export ./src/mpt/audio          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/audio          --native-eol CRLF
	svn export ./src/mpt/base           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/base           --native-eol CRLF
	svn export ./src/mpt/binary         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/binary         --native-eol CRLF
	svn export ./src/mpt/check          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/check          --native-eol CRLF
	svn export ./src/mpt/crc            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/crc            --native-eol CRLF
	#svn export ./src/mpt/crypto         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/crypto         --native-eol CRLF
	svn export ./src/mpt/detect         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/detect         --native-eol CRLF
	svn export ./src/mpt/endian         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/endian         --native-eol CRLF
	svn export ./src/mpt/environment    bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/environment    --native-eol CRLF
	svn export ./src/mpt/exception      bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/exception      --native-eol CRLF
	svn export ./src/mpt/format         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/format         --native-eol CRLF
	#svn export ./src/mpt/fs             bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/fs             --native-eol CRLF
	svn export ./src/mpt/io             bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io             --native-eol CRLF
	svn export ./src/mpt/io_file        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file        --native-eol CRLF
	svn export ./src/mpt/io_file_adapter bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file_adapter --native-eol CRLF
	svn export ./src/mpt/io_file_read   bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file_read   --native-eol CRLF
	svn export ./src/mpt/io_file_unique bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_file_unique --native-eol CRLF
	svn export ./src/mpt/io_read        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_read        --native-eol CRLF
	svn export ./src/mpt/io_write       bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/io_write       --native-eol CRLF
	#svn export ./src/mpt/json           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/json           --native-eol CRLF
	#svn export ./src/mpt/library        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/library        --native-eol CRLF
	svn export ./src/mpt/mutex          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/mutex          --native-eol CRLF
	svn export ./src/mpt/out_of_memory  bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/out_of_memory  --native-eol CRLF
	svn export ./src/mpt/osinfo         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/osinfo         --native-eol CRLF
	svn export ./src/mpt/parse          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/parse          --native-eol CRLF
	svn export ./src/mpt/path           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/path           --native-eol CRLF
	svn export ./src/mpt/random         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/random         --native-eol CRLF
	svn export ./src/mpt/string         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/string         --native-eol CRLF
	svn export ./src/mpt/string_transcode bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/string_transcode --native-eol CRLF
	svn export ./src/mpt/system_error   bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/system_error   --native-eol CRLF
	svn export ./src/mpt/test           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/test           --native-eol CRLF
	svn export ./src/mpt/uuid           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/uuid           --native-eol CRLF
	#svn export ./src/mpt/uuid_namespace bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/mpt/uuid_namespace --native-eol CRLF
	svn export ./src/openmpt/all        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/all       --native-eol CRLF
	svn export ./src/openmpt/base       bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/base      --native-eol CRLF
	svn export ./src/openmpt/logging    bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/logging   --native-eol CRLF
	svn export ./src/openmpt/random     bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/random    --native-eol CRLF
	svn export ./src/openmpt/soundbase  bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/src/openmpt/soundbase --native-eol CRLF
	svn export ./test                  bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test                  --native-eol CRLF
	rm bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test/mpt_tests_crypto.cpp
	rm bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/test/mpt_tests_uuid_namespace.cpp
	svn export ./libopenmpt            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/libopenmpt            --native-eol CRLF
	svn export ./examples              bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/examples              --native-eol CRLF
	svn export ./openmpt123            bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123            --native-eol CRLF
	svn export ./contrib               bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/contrib               --native-eol CRLF
	svn export ./include/minimp3       bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/minimp3       --native-eol CRLF
	svn export ./include/miniz         bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/miniz         --native-eol CRLF
	svn export ./include/mpg123        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/mpg123        --native-eol CRLF
	svn export ./include/flac          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/flac          --native-eol CRLF
	svn export ./include/portaudio     bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/portaudio     --native-eol CRLF
	svn export ./include/ogg           bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/ogg           --native-eol CRLF
	svn export ./include/pugixml       bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/pugixml       --native-eol CRLF
	svn export ./include/stb_vorbis    bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/stb_vorbis    --native-eol CRLF
	svn export ./include/vorbis        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/vorbis        --native-eol CRLF
	svn export ./include/winamp        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/winamp        --native-eol CRLF
	svn export ./include/xmplay        bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/xmplay        --native-eol CRLF
	svn export ./include/zlib          bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/include/zlib          --native-eol CRLF
	cp bin/$(FLAVOUR_DIR)dist.mk bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/dist.mk
	cp bin/$(FLAVOUR_DIR)svn_version_dist.h bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/build/svn_version/svn_version.h
	cd bin/$(FLAVOUR_DIR)dist-zip/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/ && zip -r ../libopenmpt-$(DIST_LIBOPENMPT_VERSION).msvc.zip --compression-method deflate -9 *

.PHONY: bin/$(FLAVOUR_DIR)dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip
bin/$(FLAVOUR_DIR)dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION).zip: bin/$(FLAVOUR_DIR)svn_version_dist.h
	mkdir -p bin/$(FLAVOUR_DIR)dist-zip
	rm -rf bin/$(FLAVOUR_DIR)dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)
	svn export ./ bin/$(FLAVOUR_DIR)dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/ --native-eol CRLF
	cp bin/$(FLAVOUR_DIR)svn_version_dist.h bin/$(FLAVOUR_DIR)dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/common/svn_version_default/svn_version.h
	cd bin/$(FLAVOUR_DIR)dist-zip/OpenMPT-src-$(DIST_OPENMPT_VERSION)/ && zip -r ../OpenMPT-src-$(DIST_OPENMPT_VERSION).zip --compression-method deflate -9 *

.PHONY: bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION).dev.js.tar
bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION).dev.js.tar:
	mkdir -p bin/$(FLAVOUR_DIR)dist-js
	rm -rf                                       bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/licenses
	svn export ./LICENSE                         bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/license.txt
	svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/licenses/license.mpt.BSD-3-Clause.txt
	svn export ./src/mpt/LICENSE.BSL-1.0.txt     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/licenses/license.mpt.BSL-1.0.txt
	svn export ./include/minimp3/LICENSE         bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/licenses/license.minimp3.txt
	svn export ./include/miniz/miniz.c           bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/licenses/license.miniz.txt
	svn export ./include/stb_vorbis/stb_vorbis.c bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/licenses/license.stb_vorbis.txt
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)all
	cp bin/$(FLAVOUR_DIR)stage/all/libopenmpt.js               bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)all/libopenmpt.js
	cp bin/$(FLAVOUR_DIR)stage/all/libopenmpt.wasm             bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)all/libopenmpt.wasm
	cp bin/$(FLAVOUR_DIR)stage/all/libopenmpt.wasm.js          bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)all/libopenmpt.wasm.js
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)wasm
	cp bin/$(FLAVOUR_DIR)stage/wasm/libopenmpt.js              bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)wasm/libopenmpt.js
	cp bin/$(FLAVOUR_DIR)stage/wasm/libopenmpt.wasm            bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)wasm/libopenmpt.wasm
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)js
	cp bin/$(FLAVOUR_DIR)stage/js/libopenmpt.js                bin/$(FLAVOUR_DIR)dist-js/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/bin/$(FLAVOUR_DIR)js/libopenmpt.js
	cd bin/$(FLAVOUR_DIR)dist-js/ && tar cv --numeric-owner --owner=0 --group=0 libopenmpt-$(DIST_LIBOPENMPT_VERSION) > libopenmpt-$(DIST_LIBOPENMPT_VERSION).dev.js.tar

.PHONY: bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.dos.zip
bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.dos.zip:
	mkdir -p bin/$(FLAVOUR_DIR)dist-dos
	rm -rf                                       bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES
	svn export ./LICENSE                         bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE.TXT          --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/MPT_BSD3.TXT --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSL-1.0.txt     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/MPT_BSL1.TXT --native-eol CRLF
	cp include/allegro42/readme.txt              bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/ALLEGRO.TXT
	cp include/cwsdpmi/bin/$(FLAVOUR_DIR)cwsdpmi.doc           bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/CWSDPMI.TXT
ifeq ($(ALLOW_LGPL),1)
	svn export ./include/mpg123/COPYING          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/MPG123.TXT   --native-eol CRLF
	svn export ./include/mpg123/AUTHORS          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/MPG123_A.TXT --native-eol CRLF
	svn export ./include/vorbis/COPYING          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/VORBIS.TXT   --native-eol CRLF
	svn export ./include/zlib/README             bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/ZLIB.TXT     --native-eol CRLF
else
	svn export ./include/minimp3/LICENSE         bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/MINIMP3.TXT --native-eol CRLF
	svn export ./include/miniz/miniz.c           bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/MINIZ.TXT   --native-eol CRLF
	svn export ./include/stb_vorbis/stb_vorbis.c bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/STBVORB.TXT --native-eol CRLF
endif
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/DJGPP
	cp $(shell dirname $(shell which i386-pc-msdosdjgpp-gcc))/../license/copying     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/DJGPP/COPYING
	cp $(shell dirname $(shell which i386-pc-msdosdjgpp-gcc))/../license/copying.dj  bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/DJGPP/COPYING.DJ
	cp $(shell dirname $(shell which i386-pc-msdosdjgpp-gcc))/../license/copying.lib bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/DJGPP/COPYING.LIB
	cp $(shell dirname $(shell which i386-pc-msdosdjgpp-gcc))/../license/source.txt  bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSES/DJGPP/SOURCE.TXT
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/SRC
	cp build/externals/csdpmi7s.zip              bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/SRC/CSDPMI7S.ZIP
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN
	cp bin/$(FLAVOUR_DIR)openmpt123.exe          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/OMPT123.EXE
	cp include/cwsdpmi/bin/cwsdpmi.doc           bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSDPMI.DOC
	cp include/cwsdpmi/bin/CWSDPMI.EXE           bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSDPMI.EXE
	cp include/cwsdpmi/bin/CWSDPR0.EXE           bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSDPR0.EXE
	cp include/cwsdpmi/bin/cwsparam.doc          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSPARAM.DOC
	cp include/cwsdpmi/bin/CWSPARAM.EXE          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSPARAM.EXE
	cp include/cwsdpmi/bin/CWSDSTUB.EXE          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSDSTUB.EXE
	cp include/cwsdpmi/bin/CWSDSTR0.EXE          bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/BIN/CWSDSTR0.EXE
	cd bin/$(FLAVOUR_DIR)dist-dos/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/ && zip -r ../libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.dos.zip --compression-method deflate -9 *

.PHONY: bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win98.zip
bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win98.zip:
	mkdir -p bin/$(FLAVOUR_DIR)dist-retro-win98
	rm -rf                                       bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses
	svn export ./LICENSE                         bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE.TXT --native-eol CRLF
	svn export ./doc/libopenmpt/changelog.md     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Changelog.txt --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/license.mpt.BSD-3-Clause.txt --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSL-1.0.txt     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/license.mpt.BSL-1.0.txt --native-eol CRLF
ifeq ($(ALLOW_LGPL),1)
	svn export ./include/mpg123/COPYING          bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.mpg123.txt --native-eol CRLF
	svn export ./include/mpg123/AUTHORS          bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.Authors.txt --native-eol CRLF
	svn export ./include/vorbis/COPYING          bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.Vorbis.txt --native-eol CRLF
	svn export ./include/zlib/README             bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.zlib.txt --native-eol CRLF
else
	svn export ./include/minimp3/LICENSE         bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.minimp3.txt --native-eol CRLF
	svn export ./include/miniz/miniz.c           bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.miniz.txt --native-eol CRLF
	svn export ./include/stb_vorbis/stb_vorbis.c bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.stb_vorbis.txt --native-eol CRLF
endif
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123
	cp bin/$(FLAVOUR_DIR)openmpt123.exe                        bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123/openmpt123.exe
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/XMPlay
	svn export ./libopenmpt/xmp-openmpt/xmp-openmpt.txt  bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/XMPlay/xmp-openmpt.txt --native-eol CRLF
	cp bin/$(FLAVOUR_DIR)xmp-openmpt.dll                       bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/XMPlay/xmp-openmpt.dll
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Winamp
	svn export ./libopenmpt/in_openmpt/in_openmpt.txt   bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Winamp/in_openmpt.txt --native-eol CRLF
	cp bin/$(FLAVOUR_DIR)in_openmpt.dll                        bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Winamp/in_openmpt.dll
	cd bin/$(FLAVOUR_DIR)dist-retro-win98/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/ && ../../../build/tools/7zip/7z a -tzip -mx=9 ../libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win98.zip *

.PHONY: bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win95.zip
bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win95.zip:
	mkdir -p bin/$(FLAVOUR_DIR)dist-retro-win95
	rm -rf                                       bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses
	svn export ./LICENSE                         bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/LICENSE.TXT --native-eol CRLF
	svn export ./doc/libopenmpt/changelog.md     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Changelog.txt --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSD-3-Clause.txt bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/license.mpt.BSD-3-Clause.txt --native-eol CRLF
	svn export ./src/mpt/LICENSE.BSL-1.0.txt     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/license.mpt.BSL-1.0.txt --native-eol CRLF
ifeq ($(ALLOW_LGPL),1)
	svn export ./include/mpg123/COPYING          bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.mpg123.txt --native-eol CRLF
	svn export ./include/mpg123/AUTHORS          bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.Authors.txt --native-eol CRLF
	svn export ./include/vorbis/COPYING          bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.Vorbis.txt --native-eol CRLF
	svn export ./include/zlib/README             bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.zlib.txt --native-eol CRLF
else
	svn export ./include/minimp3/LICENSE         bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.minimp3.txt --native-eol CRLF
	svn export ./include/miniz/miniz.c           bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.miniz.txt --native-eol CRLF
	svn export ./include/stb_vorbis/stb_vorbis.c bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Licenses/License.stb_vorbis.txt --native-eol CRLF
endif
	mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123
	cp bin/$(FLAVOUR_DIR)openmpt123.exe                        bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/openmpt123/openmpt123.exe
	#mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/XMPlay
	#svn export ./libopenmpt/xmp-openmpt/xmp-openmpt.txt  bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/XMPlay/xmp-openmpt.txt --native-eol CRLF
	#cp bin/$(FLAVOUR_DIR)xmp-openmpt.dll                       bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/XMPlay/xmp-openmpt.dll
	#mkdir -p                                     bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Winamp
	#svn export ./libopenmpt/in_openmpt/in_openmpt.txt   bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Winamp/in_openmpt.txt --native-eol CRLF
	#cp bin/$(FLAVOUR_DIR)in_openmpt.dll                        bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/Winamp/in_openmpt.dll
	cd bin/$(FLAVOUR_DIR)dist-retro-win95/libopenmpt-$(DIST_LIBOPENMPT_VERSION)/ && 7z a -tzip -mx=9 ../libopenmpt-$(DIST_LIBOPENMPT_VERSION).bin.retro.win95.zip *

bin/$(FLAVOUR_DIR)libopenmpt.a: $(LIBOPENMPT_OBJECTS)
	$(INFO) [AR] $@
	$(SILENT)$(AR) $(ARFLAGS) $@ $^

bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX): $(LIBOPENMPT_OBJECTS)
	$(INFO) [LD] $@
ifeq ($(NO_SHARED_LINKER_FLAG),1)
	$(SILENT)$(LINK.cc) $(LIBOPENMPT_LDFLAGS) $(SO_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
else
	$(SILENT)$(LINK.cc) -shared $(LIBOPENMPT_LDFLAGS) $(SO_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
endif
ifeq ($(SHARED_SONAME),1)
	$(SILENT)mv bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX) bin/$(FLAVOUR_DIR)$(LIBOPENMPT_SONAME)
	$(SILENT)ln -sf $(LIBOPENMPT_SONAME) bin/$(FLAVOUR_DIR)libopenmpt$(SOSUFFIX)
endif

bin/$(FLAVOUR_DIR)openmpt123.1: bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX) openmpt123/openmpt123.h2m
	$(INFO) [HELP2MAN] $@
	$(SILENT)help2man --no-discard-stderr --no-info --version-option=--man-version --help-option=--man-help --include=openmpt123/openmpt123.h2m $< > $@

bin/$(FLAVOUR_DIR)in_openmpt$(SOSUFFIX): $(INOPENMPT_OBJECTS) $(LIBOPENMPT_OBJECTS)
	$(INFO) [LD] $@
ifeq ($(NO_SHARED_LINKER_FLAG),1)
	$(SILENT)$(LINK.cc) $(LIBOPENMPT_LDFLAGS) $(SO_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
else
	$(SILENT)$(LINK.cc) -shared $(LIBOPENMPT_LDFLAGS) $(SO_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
endif

bin/$(FLAVOUR_DIR)xmp-openmpt$(SOSUFFIX): $(XMPOPENMPT_OBJECTS) $(LIBOPENMPT_OBJECTS)
	$(INFO) [LD] $@
ifeq ($(NO_SHARED_LINKER_FLAG),1)
	$(SILENT)$(LINK.cc) $(LIBOPENMPT_LDFLAGS) $(SO_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -lgdi32 -o $@
else
	$(SILENT)$(LINK.cc) -shared $(LIBOPENMPT_LDFLAGS) $(SO_LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -lgdi32 -o $@
endif

openmpt123/openmpt123$(FLAVOUR_O).o: openmpt123/openmpt123.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CXXFLAGS_OPENMPT123) $(CPPFLAGS) $(CPPFLAGS_OPENMPT123) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.cc) $(CXXFLAGS_OPENMPT123) $(CPPFLAGS_OPENMPT123) $(OUTPUT_OPTION) $<
bin/$(FLAVOUR_DIR)openmpt123$(EXESUFFIX): $(OPENMPT123_OBJECTS) $(DEPS_ALLEGRO42) $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_OPENMPT123) $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_OPENMPT123) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_OPENMPT123) $(OPENMPT123_OBJECTS) $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_OPENMPT123) -o $@
endif
endif

contrib/fuzzing/fuzz$(FLAVOUR_O).o: contrib/fuzzing/fuzz.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.cc) $(OUTPUT_OPTION) $<
bin/$(FLAVOUR_DIR)fuzz$(EXESUFFIX): contrib/fuzzing/fuzz$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_LIBOPENMPT) contrib/fuzzing/fuzz$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) contrib/fuzzing/fuzz$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif
endif

examples/libopenmpt_example_c$(FLAVOUR_O).o: examples/libopenmpt_example_c.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CFLAGS_PORTAUDIO) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIO) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(CFLAGS_PORTAUDIO) $(CPPFLAGS_PORTAUDIO) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_mem$(FLAVOUR_O).o: examples/libopenmpt_example_c_mem.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CFLAGS_PORTAUDIO) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIO) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(CFLAGS_PORTAUDIO) $(CPPFLAGS_PORTAUDIO) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_unsafe$(FLAVOUR_O).o: examples/libopenmpt_example_c_unsafe.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CFLAGS_PORTAUDIO) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIO) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(CFLAGS_PORTAUDIO) $(CPPFLAGS_PORTAUDIO) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_pipe$(FLAVOUR_O).o: examples/libopenmpt_example_c_pipe.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_stdout$(FLAVOUR_O).o: examples/libopenmpt_example_c_stdout.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_c_probe$(FLAVOUR_O).o: examples/libopenmpt_example_c_probe.c
	$(INFO) [CC] $<
	$(VERYSILENT)$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.c) $(OUTPUT_OPTION) $<
examples/libopenmpt_example_cxx$(FLAVOUR_O).o: examples/libopenmpt_example_cxx.cpp
	$(INFO) [CXX] $<
	$(VERYSILENT)$(CXX) $(CXXFLAGS) $(CXXFLAGS_PORTAUDIOCPP) $(CPPFLAGS) $(CPPFLAGS_PORTAUDIOCPP) $(TARGET_ARCH) -M -MT$@ $< > $*$(FLAVOUR_O).d
	$(SILENT)$(COMPILE.cc) $(CXXFLAGS_PORTAUDIOCPP) $(CPPFLAGS_PORTAUDIOCPP) $(OUTPUT_OPTION) $<
bin/$(FLAVOUR_DIR)libopenmpt_example_c$(EXESUFFIX): examples/libopenmpt_example_c$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(BIN_LDFLAGS)$(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
endif
endif
bin/$(FLAVOUR_DIR)libopenmpt_example_c_mem$(EXESUFFIX): examples/libopenmpt_example_c_mem$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_mem$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_mem$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
endif
endif
bin/$(FLAVOUR_DIR)libopenmpt_example_c_unsafe$(EXESUFFIX): examples/libopenmpt_example_c_unsafe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_unsafe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIO) examples/libopenmpt_example_c_unsafe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIO) -o $@
endif
endif
bin/$(FLAVOUR_DIR)libopenmpt_example_c_pipe$(EXESUFFIX): examples/libopenmpt_example_c_pipe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_pipe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_pipe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif
endif
bin/$(FLAVOUR_DIR)libopenmpt_example_c_stdout$(EXESUFFIX): examples/libopenmpt_example_c_stdout$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_stdout$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_stdout$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif
endif
bin/$(FLAVOUR_DIR)libopenmpt_example_c_probe$(EXESUFFIX): examples/libopenmpt_example_c_probe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_probe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) examples/libopenmpt_example_c_probe$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) -o $@
endif
endif
bin/$(FLAVOUR_DIR)libopenmpt_example_cxx$(EXESUFFIX): examples/libopenmpt_example_cxx$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(OUTPUT_LIBOPENMPT)
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIOCPP) examples/libopenmpt_example_cxx$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIOCPP) -o $@
ifeq ($(HOST),unix)
ifeq ($(SHARED_LIB),1)
	$(SILENT)mv $@ $@.norpath
	$(INFO) [LD] $@
	$(SILENT)$(LINK.cc) $(BIN_LDFLAGS) $(LDFLAGS_RPATH) $(LDFLAGS_LIBOPENMPT) $(LDFLAGS_PORTAUDIOCPP) examples/libopenmpt_example_cxx$(FLAVOUR_O).o $(OBJECTS_LIBOPENMPT) $(LOADLIBES) $(LDLIBS) $(LDLIBS_LIBOPENMPT) $(LDLIBS_PORTAUDIOCPP) -o $@
endif
endif

.PHONY: cppcheck-libopenmpt
cppcheck-libopenmpt:
	$(INFO) [CPPCHECK] libopenmpt
	$(SILENT)$(CPPCHECK) -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $(CPPCHECK_FLAGS) $(CPPCHECK_PLATFORM) --check-config --suppress=unmatchedSuppression $(LIBOPENMPT_CXX_SOURCES) $(LIBOPENMPT_C_SOURCES)
	$(SILENT)$(CPPCHECK) -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $(CPPCHECK_FLAGS) $(CPPCHECK_PLATFORM) $(LIBOPENMPT_CXX_SOURCES) $(LIBOPENMPT_C_SOURCES)

.PHONY: cppcheck-libopenmpt-test
cppcheck-libopenmpt-test:
	$(INFO) [CPPCHECK] libopenmpt-test
	$(SILENT)$(CPPCHECK) -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $(CPPCHECK_FLAGS) $(CPPCHECK_PLATFORM) -DLIBOPENMPT_BUILD_TEST --check-config --suppress=unmatchedSuppression $(LIBOPENMPTTEST_CXX_SOURCES) $(LIBOPENMPTTEST_C_SOURCES)
	$(SILENT)$(CPPCHECK) -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $(CPPCHECK_FLAGS) $(CPPCHECK_PLATFORM) -DLIBOPENMPT_BUILD_TEST $(LIBOPENMPTTEST_CXX_SOURCES) $(LIBOPENMPTTEST_C_SOURCES)

.PHONY: cppcheck-openmpt123
cppcheck-openmpt123:
	$(INFO) [CPPCHECK] openmpt123
	$(SILENT)$(CPPCHECK) -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $(CPPCHECK_FLAGS) $(CPPCHECK_PLATFORM) --check-config --suppress=unmatchedSuppression $(OPENMPT123_CXX_SOURCES)
	$(SILENT)$(CPPCHECK) -DCPPCHECK -DMPT_CPPCHECK_CUSTOM $(CPPCHECK_FLAGS) $(CPPCHECK_PLATFORM) $(OPENMPT123_CXX_SOURCES)

.PHONY: cppcheck
cppcheck: cppcheck-libopenmpt cppcheck-libopenmpt-test cppcheck-openmpt123

.PHONY: clean
clean:
	$(INFO) clean ...
	$(SILENT)$(RM) $(call FIXPATH,$(OUTPUTS) $(ALL_OBJECTS) $(ALL_DEPENDS) $(MISC_OUTPUTS))
	$(SILENT)$(RMTREE) $(call FIXPATH,$(MISC_OUTPUTDIRS))

.PHONY: clean-dist
clean-dist:
	$(INFO) clean-dist ...
	$(SILENT)$(RM) $(call FIXPATH,$(DIST_OUTPUTS))
	$(SILENT)$(RMTREE) $(call FIXPATH,$(DIST_OUTPUTDIRS))

.PHONY: distversion
distversion:
	$(SILENT)echo "$(DIST_LIBOPENMPT_VERSION)"

.PHONY: distversion-pure
distversion-pure:
	$(SILENT)echo "$(DIST_LIBOPENMPT_VERSION_PURE)"

.PHONY: distversion-tarball
distversion-tarball:
	$(SILENT)echo "$(DIST_LIBOPENMPT_TARBALL_VERSION)"
