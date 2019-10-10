
CC  = clang$(TOOLCHAIN_SUFFIX) 
CXX = clang++$(TOOLCHAIN_SUFFIX) 
LD  = clang++$(TOOLCHAIN_SUFFIX) 
AR  = ar$(TOOLCHAIN_SUFFIX) 

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX)
else
ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++17 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++17' ; fi ), c++17)
CXXFLAGS_STDCXX = -std=c++17
else
ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++14 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++14' ; fi ), c++14)
CXXFLAGS_STDCXX = -std=c++14
else
ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++11 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++11' ; fi ), c++11)
CXXFLAGS_STDCXX = -std=c++11
endif
endif
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

CXXFLAGS_WARNINGS += -Wmissing-declarations -Wshift-count-negative -Wshift-count-overflow -Wshift-overflow -Wshift-sign-overflow -Wshift-op-parentheses
CFLAGS_WARNINGS   += -Wmissing-prototypes   -Wshift-count-negative -Wshift-count-overflow -Wshift-overflow -Wshift-sign-overflow -Wshift-op-parentheses

#CXXFLAGS_WARNINGS += -Wdocumentation
#CXXFLAGS_WARNINGS += -Wconversion
#CXXFLAGS_WARNINGS += -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-shadow -Wno-sign-conversion -Wno-weak-vtables

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=lld
CXXFLAGS_WARNINGS += -Wpedantic -Wframe-larger-than=20000
#CXXFLAGS_WARNINGS += -Wdouble-promotion -Wframe-larger-than=16000
CFLAGS_WARNINGS   += -Wpedantic -Wframe-larger-than=4000
#CFLAGS_WARNINGS   += -Wdouble-promotion
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
CXXFLAGS_WARNINGS += -Wdeprecated -Wextra-semi -Wnon-virtual-dtor -Wreserved-id-macro
endif

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-cast-qual

EXESUFFIX=
