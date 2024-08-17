
ifeq ($(HOST),unix)

ifeq ($(HOST_FLAVOUR),MACOSX)

include build/make/config-macos.mk

else ifeq ($(HOST_FLAVOUR),MSYS2)

ifeq ($(MSYSTEM),MINGW64)
WINDOWS_ARCH=amd64
include build/make/config-mingw-w64.mk
else ifeq ($(MSYSTEM),MINGW32)
WINDOWS_ARCH=x86
include build/make/config-mingw-w64.mk
else ifeq ($(MSYSTEM),UCRT64)
WINDOWS_ARCH=amd64
include build/make/config-mingw-w64.mk
else ifeq ($(MSYSTEM),CLANG64)
WINDOWS_ARCH=amd64
MINGW_COMPILER=clang
include build/make/config-mingw-w64.mk
else
WINDOWS_ARCH=x86
include build/make/config-mingw-w64.mk
endif

else ifeq ($(HOST_FLAVOUR),LINUX)

include build/make/config-gcc.mk

else ifeq ($(HOST_FLAVOUR),NETBSD)

include build/make/config-gcc.mk
MPT_COMPILER_NOALLOCAH=1
NO_PORTAUDIOCPP?=1

else ifeq ($(HOST_FLAVOUR),FREEBSD)

include build/make/config-clang.mk
MPT_COMPILER_NOALLOCAH=1
NO_PORTAUDIOCPP?=1

else ifeq ($(HOST_FLAVOUR),OPENBSD)

NO_PORTAUDIOCPP?=1
NO_PULSEAUDIO?=1
include build/make/config-clang.mk
MPT_COMPILER_NOALLOCAH=1

else ifeq ($(HOST_FLAVOUR),HAIKU)

# In Haiku x86 32bit (but not 64bit),
# modern compilers need a -x86 suffix.
UNAME_P:=$(shell uname -p)
ifeq ($(UNAME_P),x86)
TOOLCHAIN_SUFFIX=-x86
endif
include build/make/config-gcc.mk

else

include build/make/config-unknown.mk

endif

else

include build/make/config-unknown.mk

endif

