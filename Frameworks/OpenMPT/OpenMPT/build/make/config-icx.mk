
ifeq ($(origin CC),default)
CC  = $(TOOLCHAIN_PREFIX)icx$(TOOLCHAIN_SUFFIX)
endif
ifeq ($(origin CXX),default)
CXX = $(TOOLCHAIN_PREFIX)icpx$(TOOLCHAIN_SUFFIX)
endif
ifeq ($(origin LD),default)
LD  = $(CXX)
endif
ifeq ($(origin AR),default)
AR  = $(TOOLCHAIN_PREFIX)ar$(TOOLCHAIN_SUFFIX)
endif

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX) -fexceptions -frtti -pthread
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=c++20 -fexceptions -frtti -pthread
else
CXXFLAGS_STDCXX = -std=c++17 -fexceptions -frtti -pthread
endif
ifneq ($(STDC),)
CFLAGS_STDC = -std=$(STDC) -pthread
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=c17 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c17' ; fi ), c17)
CFLAGS_STDC = -std=c17 -pthread
else
CFLAGS_STDC = -std=c11 -pthread
endif
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)
LDFLAGS += -pthread

CPPFLAGS +=
CXXFLAGS += -fPIC
CFLAGS   += -fPIC
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  := rcs

MODERN=0
NATIVE=0
OPTIMIZE=vectorize
OPTIMIZE_FASTMATH=0
OPTIMIZE_LTO=1

FASTMATH_STYLE=

CXXFLAGS += -fp-model=precise
CFLAGS   += -fp-model=precise

ifeq ($(OPTIMIZE_LTO),1)
CXXFLAGS += -ipo
CFLAGS   += -ipo
LDFLAGS  += -ipo
endif

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

include build/make/warnings-clang.mk

EXESUFFIX=
