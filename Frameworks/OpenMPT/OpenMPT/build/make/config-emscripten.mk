
ifeq ($(origin CC),default)
CC  = emcc
endif
ifeq ($(origin CXX),default)
CXX = em++
endif
ifeq ($(origin LD),default)
LD  = em++
endif
ifeq ($(origin AR),default)
AR  = emar
endif
LINK.cc = em++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)

EMSCRIPTEN_TARGET?=default
EMSCRIPTEN_THREADS?=0
EMSCRIPTEN_PORTS?=0

ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX)
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=c++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=c++20
else
CXXFLAGS_STDCXX = -std=c++17
endif
ifneq ($(STDC),)
CFLAGS_STDC = -std=$(STDC)
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=c17 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c17' ; fi ), c17)
CFLAGS_STDC = -std=c17
else
CFLAGS_STDC = -std=c11
endif
CXXFLAGS += $(CXXFLAGS_STDCXX)
CFLAGS += $(CFLAGS_STDC)

CPPFLAGS +=
CXXFLAGS += -fPIC
CFLAGS   += -fPIC
LDFLAGS  +=
LDLIBS   +=
ARFLAGS  := rcs

ifeq ($(EMSCRIPTEN_THREADS),1)
CXXFLAGS += -pthread
CFLAGS   += -pthread
LDFLAGS  += -pthread
endif

ifeq ($(EMSCRIPTEN_PORTS),1)
CXXFLAGS += -s USE_ZLIB=1 -sUSE_MPG123=1 -sUSE_OGG=1 -sUSE_VORBIS=1 -DMPT_WITH_ZLIB -DMPT_WITH_MPG123 -DMPT_WITH_VORBIS -DMPT_WITH_VORBISFILE -DMPT_WITH_OGG
CFLAGS   += -s USE_ZLIB=1 -sUSE_MPG123=1 -sUSE_OGG=1 -sUSE_VORBIS=1 -DMPT_WITH_ZLIB -DMPT_WITH_MPG123 -DMPT_WITH_VORBIS -DMPT_WITH_VORBISFILE -DMPT_WITH_OGG
LDFLAGS  += -s USE_ZLIB=1 -sUSE_MPG123=1 -sUSE_OGG=1 -sUSE_VORBIS=1
NO_MINIZ=1
NO_MINIMP3=1
NO_STBVORBIS=1
endif

CXXFLAGS += -Oz
CFLAGS   += -Oz
LDFLAGS  += -Oz

# Enable LTO as recommended by Emscripten
#CXXFLAGS += -flto=thin
#CFLAGS   += -flto=thin
#LDFLAGS  += -flto=thin -Wl,--thinlto-jobs=all
# As per recommendation in <https://github.com/emscripten-core/emscripten/issues/15638#issuecomment-982772770>,
# thinLTO is not as well tested as full LTO. Stick to full LTO for now.
CXXFLAGS += -flto
CFLAGS   += -flto
LDFLAGS  += -flto

# Work-around <https://github.com/emscripten-core/emscripten/issues/20810>.
# The warning with emscripten 3.1.50 sounds very dangerous,
# and since it is apparently caused by removing whitespace,
# additional whitespace is a small price to pay for correctness.
LDFLAGS  += -g1

ifeq ($(EMSCRIPTEN_TARGET),default)
# emits whatever is emscripten's default, currently (1.38.8) this is the same as "wasm" below.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),all)
# emits native wasm AND javascript with full wasm optimizations.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += -s WASM=2 -s LEGACY_VM_SUPPORT=1 -Wno-transpile

# work-around <https://github.com/emscripten-core/emscripten/issues/17897>.
CXXFLAGS += -fno-inline-functions
CFLAGS   += -fno-inline-functions
LDFLAGS  += -fno-inline-functions

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),audioworkletprocessor)
# emits an es6 module in a single file suitable for use in an AudioWorkletProcessor
CPPFLAGS += -DMPT_BUILD_WASM -DMPT_BUILD_AUDIOWORKLETPROCESSOR
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += -s WASM=1 -s WASM_ASYNC_COMPILATION=0 -s MODULARIZE=1 -s EXPORT_ES6=1 -s SINGLE_FILE=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),wasm)
# emits native wasm.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += -s WASM=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),js)
# emits only plain javascript with plain javascript focused optimizations.
CPPFLAGS += -DMPT_BUILD_ASMJS
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += -s WASM=0 -s LEGACY_VM_SUPPORT=1 -Wno-transpile

# work-around <https://github.com/emscripten-core/emscripten/issues/17897>.
CXXFLAGS += -fno-inline-functions
CFLAGS   += -fno-inline-functions
LDFLAGS  += -fno-inline-functions

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

endif

CXXFLAGS += -s DISABLE_EXCEPTION_CATCHING=0
CFLAGS   += -s DISABLE_EXCEPTION_CATCHING=0 -fno-strict-aliasing
LDFLAGS  += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s ERROR_ON_MISSING_LIBRARIES=1 -s EXPORT_NAME="'libopenmpt'"
SO_LDFLAGS += -s EXPORTED_FUNCTIONS="['_malloc','_free']"

include build/make/warnings-clang.mk

REQUIRES_RUNPREFIX=1

EXESUFFIX=.js
SOSUFFIX=.js
RUNPREFIX=node 
TEST_LDFLAGS= --pre-js build/make/test-pre.js -lnodefs.js 

ifeq ($(EMSCRIPTEN_THREADS),1)
RUNPREFIX+=--experimental-wasm-threads --experimental-wasm-bulk-memory 
endif

DYNLINK=0
SHARED_LIB=1
STATIC_LIB=0
EXAMPLES=1
OPENMPT123=0
SHARED_SONAME=0
NO_SHARED_LINKER_FLAG=1

# Disable the generic compiler optimization flags as emscripten is sufficiently different.
# Optimization flags are hard-coded for emscripten in this file.
DEBUG=0
OPTIMIZE=none

IS_CROSS=1

ifeq ($(ALLOW_LGPL),1)
LOCAL_ZLIB=1
LOCAL_MPG123=1
LOCAL_OGG=1
LOCAL_VORBIS=1
else
NO_ZLIB=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
endif
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL2=1
NO_FLAC=1
NO_SNDFILE=1

