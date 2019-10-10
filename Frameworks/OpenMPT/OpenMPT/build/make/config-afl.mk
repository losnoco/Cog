
CC  = contrib/fuzzing/afl/afl-clang-fast
CXX = contrib/fuzzing/afl/afl-clang-fast++
LD  = contrib/fuzzing/afl/afl-clang-fast++
AR  = ar 

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
CXXFLAGS += -fPIC -fno-strict-aliasing
CFLAGS   += -fPIC -fno-strict-aliasing
LDFLAGS  += 
LDLIBS   += -lm
ARFLAGS  := rcs

CXXFLAGS_WARNINGS += -Wmissing-declarations
CFLAGS_WARNINGS   += -Wmissing-prototypes

ifeq ($(CHECKED_ADDRESS),1)
CXXFLAGS += -fsanitize=address
CFLAGS   += -fsanitize=address
endif

ifeq ($(CHECKED_UNDEFINED),1)
CXXFLAGS += -fsanitize=undefined
CFLAGS   += -fsanitize=undefined
endif

EXESUFFIX=

FUZZ=1
CPPFLAGS += -DMPT_BUILD_FUZZER -DMPT_BUILD_FATAL_ASSERTS

