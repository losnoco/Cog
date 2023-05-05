
ifeq ($(origin CC),default)
CC  = cc
endif
ifeq ($(origin CXX),default)
CXX = c++
endif
ifeq ($(origin LD),default)
LD  = $(CXX)
endif
ifeq ($(origin AR),default)
AR  = ar
endif

CXXFLAGS_STDCXX = -std=c++17
CFLAGS_STDC = -std=c11
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += 
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  := rcs

MPT_COMPILER_GENERIC=1
SHARED_LIB=0
DYNLINK=0

EXESUFFIX=

