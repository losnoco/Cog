
ifeq ($(origin CC),default)
CC  = i686-w64-mingw32-gcc$(MINGW_FLAVOUR)
endif
ifeq ($(origin CXX),default)
CXX = i686-w64-mingw32-g++$(MINGW_FLAVOUR)
endif
ifeq ($(origin LD),default)
LD  = $(CXX)
endif
ifeq ($(origin AR),default)
AR  = i686-w64-mingw32-ar$(MINGW_FLAVOUR)
endif

CXXFLAGS_STDCXX = -std=c++17
CFLAGS_STDC = -std=c99
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += -DWIN32 -D_WIN32 -DWINAPI_FAMILY=0x2 -D_WIN32_WINNT=0x0602
ifeq ($(MINGW_COMPILER),clang)
CXXFLAGS += -municode
CFLAGS   += -municode
LDFLAGS  += -mconsole -mthreads
else
CXXFLAGS += -municode -mthreads
CFLAGS   += -municode -mthreads
LDFLAGS  += -mconsole
endif
LDLIBS   += -lm -lole32 -lrpcrt4 -lwinmm
ARFLAGS  := rcs

PC_LIBS_PRIVATE += -lole32 -lrpcrt4

ifeq ($(MINGW_COMPILER),clang)
include build/make/warnings-clang.mk
else
include build/make/warnings-gcc.mk
endif

EXESUFFIX=.exe
SOSUFFIX=.dll
SOSUFFIXWINDOWS=1

DYNLINK=0
SHARED_LIB=1
STATIC_LIB=0
SHARED_SONAME=0

IS_CROSS=1

OPENMPT123=0

NO_ZLIB=1
NO_LTDL=1
NO_DL=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL=1
NO_SDL2=1
NO_SNDFILE=1
NO_FLAC=1
