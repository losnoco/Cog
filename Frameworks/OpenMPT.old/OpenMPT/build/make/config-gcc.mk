
CC  = gcc$(TOOLCHAIN_SUFFIX) 
CXX = g++$(TOOLCHAIN_SUFFIX) 
LD  = g++($TOOLCHAIN_SUFFIX) 
AR  = ar$(TOOLCHAIN_SUFFIX) 

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

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

CXXFLAGS_WARNINGS += -Wsuggest-override -Wno-psabi

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=gold
CXXFLAGS_WARNINGS += -Wpedantic -Wlogical-op -Wframe-larger-than=16000
CFLAGS_WARNINGS   += -Wpedantic -Wlogical-op -Wframe-larger-than=4000
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
# re-renable after 1.29 branch
#CXXFLAGS_WARNINGS += -Wdouble-promotion
#CFLAGS_WARNINGS   += -Wdouble-promotion
endif

CFLAGS_SILENT += -Wno-cast-qual
CFLAGS_SILENT += -Wno-empty-body
CFLAGS_SILENT += -Wno-implicit-fallthrough
CFLAGS_SILENT += -Wno-old-style-declaration
CFLAGS_SILENT += -Wno-sign-compare
CFLAGS_SILENT += -Wno-type-limits
CFLAGS_SILENT += -Wno-unused-but-set-variable
CFLAGS_SILENT += -Wno-unused-function
CFLAGS_SILENT += -Wno-unused-parameter

EXESUFFIX=

