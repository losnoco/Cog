
CC  = $(TOOLCHAIN_PREFIX)gcc$(TOOLCHAIN_SUFFIX) 
CXX = $(TOOLCHAIN_PREFIX)g++$(TOOLCHAIN_SUFFIX) 
LD  = $(TOOLCHAIN_PREFIX)g++($TOOLCHAIN_SUFFIX) 
AR  = $(TOOLCHAIN_PREFIX)ar$(TOOLCHAIN_SUFFIX) 

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX)
else
ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++17 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++17' ; fi ), c++17)
CXXFLAGS_STDCXX = -std=c++17
endif
endif
CFLAGS_STDC = -std=c99
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += 
CXXFLAGS += -fPIC 
CFLAGS   += -fPIC 
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs

ifeq ($(OPTIMIZE_LTO),1)
CXXFLAGS += -flto
CFLAGS   += -flto
endif

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

include build/make/warnings-gcc.mk

EXESUFFIX=

