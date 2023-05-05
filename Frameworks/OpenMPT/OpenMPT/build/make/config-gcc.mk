
ifeq ($(origin CC),default)
CC  = $(TOOLCHAIN_PREFIX)gcc$(TOOLCHAIN_SUFFIX)
endif
ifeq ($(origin CXX),default)
CXX = $(TOOLCHAIN_PREFIX)g++$(TOOLCHAIN_SUFFIX)
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
LDLIBS   += -lm
ARFLAGS  := rcs

ifeq ($(NATIVE),1)
CXXFLAGS += -march=native
CFLAGS   += -march=native
endif

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=gold
endif

ifeq ($(OPTIMIZE_LTO),1)
CXXFLAGS += -flto
CFLAGS   += -flto
endif

ifeq ($(ANALYZE),1)
CXXFLAGS += -fanalyzer -Wno-analyzer-malloc-leak -Wno-analyzer-null-dereference -Wno-analyzer-possible-null-argument -Wno-analyzer-possible-null-dereference 
CFLAGS   += -fanalyzer -Wno-analyzer-malloc-leak -Wno-analyzer-null-dereference -Wno-analyzer-possible-null-argument -Wno-analyzer-possible-null-dereference
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

