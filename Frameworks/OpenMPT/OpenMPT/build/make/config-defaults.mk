
ifeq ($(HOST),unix)

ifeq ($(HOST_FLAVOUR),MACOSX)

include build/make/config-clang.mk
# Mac OS X overrides
DYNLINK=0
SHARED_SONAME=0

else ifeq ($(HOST_FLAVOUR),LINUX)

include build/make/config-gcc.mk

else ifeq ($(HOST_FLAVOUR),FREEBSD)

include build/make/config-clang.mk
NO_LTDL?=1
NO_PORTAUDIOCPP?=1

else ifeq ($(HOST_FLAVOUR),HAIKU)

# In Haiku x86 32bit (but not 64bit),
# modern compilers need a -x86 suffix.
UNAME_P:=$(shell uname -p)
ifeq ($(UNAME_P),x86)
TOOLCHAIN_SUFFIX=-x86
endif
include build/make/config-gcc.mk

else

include build/make/config-generic.mk

endif

else

include build/make/config-generic.mk

endif

