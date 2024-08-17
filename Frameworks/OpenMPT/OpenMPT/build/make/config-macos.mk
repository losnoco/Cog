
NO_PULSEAUDIO?=1
include build/make/config-clang.mk
# Mac OS X overrides
DYNLINK=0
SHARED_SONAME=0
MPT_COMPILER_NOSECTIONS=1
MPT_COMPILER_NOGCSECTIONS=1

# 10.13 .. 
ifeq ($(MACOSX_VERSION_MIN),)
else
CFLAGS   += -mmacosx-version-min=$(MACOSX_VERSION_MIN)
CXXFLAGS += -mmacosx-version-min=$(MACOSX_VERSION_MIN)
LDFLAGS  += -mmacosx-version-min=$(MACOSX_VERSION_MIN)
endif

# arm64/x86_64/i386
ifeq ($(ARCH),)
else
IS_CROSS=1
CFLAGS   += -arch $(ARCH)
CXXFLAGS += -arch $(ARCH)
LDFLAGS  += -arch $(ARCH)
endif

