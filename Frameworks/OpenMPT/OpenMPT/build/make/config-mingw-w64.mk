
ifeq ($(WINDOWS_ARCH),)
MINGW_ARCH = i686
else ifeq ($(WINDOWS_ARCH),x86)
MINGW_ARCH = i686
else ifeq ($(WINDOWS_ARCH),amd64)
MINGW_ARCH = x86_64
#else ifeq ($(WINDOWS_ARCH),arm)
#MINGW_ARCH = 
#else ifeq ($(WINDOWS_ARCH),arm64)
#MINGW_ARCH = 
else
$(error unknown WINDOWS_ARCH)
endif

ifeq ($(origin CC),default)
CC  = $(MINGW_ARCH)-w64-mingw32-gcc$(MINGW_FLAVOUR)
endif
ifeq ($(origin CXX),default)
CXX = $(MINGW_ARCH)-w64-mingw32-g++$(MINGW_FLAVOUR)
endif
ifeq ($(origin LD),default)
LD  = $(CXX)
endif
ifeq ($(origin AR),default)
AR  = $(MINGW_ARCH)-w64-mingw32-ar$(MINGW_FLAVOUR)
endif

CXXFLAGS_STDCXX = -std=c++17 -fexceptions -frtti
CFLAGS_STDC = -std=c99
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS +=
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

ifeq ($(WINDOWS_FAMILY),)
# nothing
else ifeq ($(WINDOWS_FAMILY),desktop-app)
# nothing
else ifeq ($(WINDOWS_FAMILY),app)
CPPFLAGS += -DWINAPI_FAMILY=2
OPENMPT123=0
else ifeq ($(WINDOWS_FAMILY),phone-app)
CPPFLAGS += -DWINAPI_FAMILY=3
OPENMPT123=0
else ifeq ($(WINDOWS_FAMILY),pc-app)
CPPFLAGS += -DWINAPI_FAMILY=2
OPENMPT123=0
else
$(error unknown WINDOWS_FAMILY)
endif

ifeq ($(WINDOWS_VERSION),)
# nothing
else ifeq ($(WINDOWS_VERSION),win95)
CPPFLAGS += -D_WIN32_WINDOWS=0x0400
else ifeq ($(WINDOWS_VERSION),win98)
CPPFLAGS += -D_WIN32_WINDOWS=0x0410
else ifeq ($(WINDOWS_VERSION),winme)
CPPFLAGS += -D_WIN32_WINDOWS=0x0490
else ifeq ($(WINDOWS_VERSION),winnt4)
CPPFLAGS += -D_WIN32_WINNT=0x0400
else ifeq ($(WINDOWS_VERSION),win2000)
CPPFLAGS += -D_WIN32_WINNT=0x0500
else ifeq ($(WINDOWS_VERSION),winxp)
CPPFLAGS += -D_WIN32_WINNT=0x0501
else ifeq ($(WINDOWS_VERSION),winxp64)
CPPFLAGS += -D_WIN32_WINNT=0x0502
else ifeq ($(WINDOWS_VERSION),winvista)
CPPFLAGS += -DNTDDI_VERSION=0x06000000 -D_WIN32_WINNT=0x0600
else ifeq ($(WINDOWS_VERSION),win7)
CPPFLAGS += -DNTDDI_VERSION=0x06010000 -D_WIN32_WINNT=0x0601
else ifeq ($(WINDOWS_VERSION),win8)
CPPFLAGS += -DNTDDI_VERSION=0x06020000 -D_WIN32_WINNT=0x0602
else ifeq ($(WINDOWS_VERSION),win8.1)
CPPFLAGS += -DNTDDI_VERSION=0x06030000 -D_WIN32_WINNT=0x0603
else ifeq ($(WINDOWS_VERSION),win10)
CPPFLAGS += -DNTDDI_VERSION=0x0A000000 -D_WIN32_WINNT=0x0A00
else
$(error unknown WINDOWS_VERSION)
endif

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

ifeq ($(HOST_FLAVOUR),MSYS2)

else

IS_CROSS=1

NO_ZLIB=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL2=1
NO_SNDFILE=1
NO_FLAC=1

endif
