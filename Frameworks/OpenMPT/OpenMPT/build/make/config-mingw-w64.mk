
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

ifeq ($(WINDOWS_CRT),)
MINGW_CRT = mingw32
else ifeq ($(WINDOWS_CRT),crtdll)
MINGW_CRT = mingw32crt
else ifeq ($(WINDOWS_CRT),msvcrt)
MINGW_CRT = mingw32
else ifeq ($(WINDOWS_CRT),ucrt)
MINGW_CRT = mingw32ucrt
endif

ifeq ($(origin CC),default)
CC  = $(MINGW_ARCH)-w64-$(MINGW_CRT)-gcc$(MINGW_FLAVOUR)
endif
ifeq ($(origin CXX),default)
CXX = $(MINGW_ARCH)-w64-$(MINGW_CRT)-g++$(MINGW_FLAVOUR)
endif
ifeq ($(origin LD),default)
LD  = $(CXX)
endif
ifeq ($(origin AR),default)
AR  = $(MINGW_ARCH)-w64-$(MINGW_CRT)-ar$(MINGW_FLAVOUR)
endif

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX) -fexceptions -frtti
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=c++20 -fexceptions -frtti
else
CXXFLAGS_STDCXX = -std=c++17 -fexceptions -frtti
endif
ifneq ($(STDC),)
CFLAGS_STDC = -std=$(STDC)
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=c17 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c17' ; fi ), c17)
CFLAGS_STDC = -std=c17
else
CFLAGS_STDC = -std=c11
endif
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += -DNOMINMAX
ifeq ($(MINGW_COMPILER),clang)
CXXFLAGS += -municode
CFLAGS   += -municode
LDFLAGS  += -mconsole -mthreads
else
CXXFLAGS += -municode -mthreads
CFLAGS   += -municode -mthreads
LDFLAGS  += -mconsole
endif
LDLIBS   += -lm
ARFLAGS  := rcs

LDLIBS_LIBOPENMPTTEST += -lole32 -lrpcrt4
LDLIBS_OPENMPT123 += -lwinmm

PC_LIBS_PRIVATE += -lole32 -lrpcrt4

ifeq ($(WINDOWS_FAMILY),)
# nothing
else ifeq ($(WINDOWS_FAMILY),desktop-app)
# nothing
else ifeq ($(WINDOWS_FAMILY),app)
CPPFLAGS += -DWINAPI_FAMILY=2
else ifeq ($(WINDOWS_FAMILY),phone-app)
CPPFLAGS += -DWINAPI_FAMILY=3
else ifeq ($(WINDOWS_FAMILY),pc-app)
CPPFLAGS += -DWINAPI_FAMILY=2
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
CPPFLAGS += -DNTDDI_VERSION=0x05000000 -D_WIN32_WINNT=0x0500
else ifeq ($(WINDOWS_VERSION),winxp)
CPPFLAGS += -DNTDDI_VERSION=0x05010000 -D_WIN32_WINNT=0x0501
else ifeq ($(WINDOWS_VERSION),winxp64)
CPPFLAGS += -DNTDDI_VERSION=0x05020100 -D_WIN32_WINNT=0x0502
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
else ifeq ($(WINDOWS_VERSION),win11)
CPPFLAGS += -DNTDDI_VERSION=0x0A00000B -D_WIN32_WINNT=0x0A00
else
$(error unknown WINDOWS_VERSION)
endif

ifneq ($(MINGW_COMPILER),clang)
# See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115049>.
MPT_COMPILER_NOIPARA=1
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
