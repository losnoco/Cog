
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
CFLAGS_STDC = -std=c17
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS += -DMPT_COMPILER_GENERIC
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 
LDLIBS   += 
ARFLAGS  := rcs

MPT_COMPILER_GENERIC=1
SHARED_LIB=0
DYNLINK=0

EXESUFFIX=

