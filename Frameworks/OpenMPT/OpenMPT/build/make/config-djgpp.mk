
CC  = i386-pc-msdosdjgpp-gcc
CXX = i386-pc-msdosdjgpp-g++
LD  = i386-pc-msdosdjgpp-g++
AR  = i386-pc-msdosdjgpp-ar

# Note that we are using GNU extensions instead of 100% standards-compliant
# mode, because otherwise DJGPP-specific headers/functions are unavailable.
CXXFLAGS_STDCXX = -std=gnu++17
CFLAGS_STDC = -std=gnu99
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += 
CXXFLAGS += -march=i386 -m80387 -mtune=pentium -ffast-math
CFLAGS   += -march=i386 -m80387 -mtune=pentium -ffast-math
LDFLAGS  +=
LDLIBS   += -lm
ARFLAGS  := rcs

include build/make/warnings-gcc.mk

EXESUFFIX=.exe

DYNLINK=0
SHARED_LIB=0
STATIC_LIB=1
SHARED_SONAME=0

DEBUG=0
OPTIMIZE=0
OPTIMIZE_SIZE=1

IS_CROSS=1

# generates warnings
MPT_COMPILER_NOVISIBILITY=1

# causes crashes on process shutdown,
# makes memory locking difficult
MPT_COMPILER_NOGCSECTIONS=1

ifeq ($(ALLOW_LGPL),1)
LOCAL_ZLIB=1
LOCAL_MPG123=1
LOCAL_OGG=1
LOCAL_VORBIS=1
else
NO_ZLIB=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
endif

NO_LTDL=1
NO_DL=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL=1
NO_SDL2=1
NO_SNDFILE=1
NO_FLAC=1

ifeq ($(BUNDLED_ALLEGRO42),1)

CPPFLAGS_ALLEGRO42 := -Iinclude/allegro42/include -DALLEGRO_HAVE_STDINT_H -DLONG_LONG="long long"
LDFLAGS_ALLEGRO42 := 
LDLIBS_ALLEGRO42 := include/allegro42/lib/djgpp/liballeg.a

include/allegro42/lib/djgpp/liballeg.a:
	+cd include/allegro42 && ./xmake.sh clean
	+cd include/allegro42 && ./xmake.sh lib

bin/openmpt123$(EXESUFFIX): include/allegro42/lib/djgpp/liballeg.a

MISC_OUTPUTS += include/allegro42/lib/djgpp/liballeg.a

endif
