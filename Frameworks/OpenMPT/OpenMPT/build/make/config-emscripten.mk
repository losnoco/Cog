
CC  = emcc -c
CXX = em++ -c
LD  = em++
AR  = emar
LINK.cc = em++ $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)

EMSCRIPTEN_TARGET?=default

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
LDLIBS   +=
ARFLAGS  := rcs

CXXFLAGS += -Os
CFLAGS   += -Os
LDFLAGS  += -Os

ifeq ($(EMSCRIPTEN_TARGET),default)
# emits whatever is emscripten's default, currently (1.38.8) this is the same as "wasm" below.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += 
CFLAGS   += 
LDFLAGS  += 

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),all)
# emits native wasm AND javascript with full wasm optimizations.
# as of emscripten 1.38, this is equivalent to default.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += -s WASM=2 -s LEGACY_VM_SUPPORT=1
CFLAGS   += -s WASM=2 -s LEGACY_VM_SUPPORT=1
LDFLAGS  += -s WASM=2 -s LEGACY_VM_SUPPORT=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),audioworkletprocessor)
# emits an es6 module in a single file suitable for use in an AudioWorkletProcessor
CPPFLAGS += -DMPT_BUILD_WASM -DMPT_BUILD_AUDIOWORKLETPROCESSOR
CXXFLAGS += -s WASM=1 -s WASM_ASYNC_COMPILATION=0 -s MODULARIZE=1 -s EXPORT_ES6=1 -s SINGLE_FILE=1
CFLAGS   += -s WASM=1 -s WASM_ASYNC_COMPILATION=0 -s MODULARIZE=1 -s EXPORT_ES6=1 -s SINGLE_FILE=1
LDFLAGS  += -s WASM=1 -s WASM_ASYNC_COMPILATION=0 -s MODULARIZE=1 -s EXPORT_ES6=1 -s SINGLE_FILE=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),wasm)
# emits native wasm.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += -s WASM=1
CFLAGS   += -s WASM=1
LDFLAGS  += -s WASM=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),js)
# emits only plain javascript with plain javascript focused optimizations.
CPPFLAGS += -DMPT_BUILD_ASMJS
CXXFLAGS += -s WASM=0 -s LEGACY_VM_SUPPORT=1
CFLAGS   += -s WASM=0 -s LEGACY_VM_SUPPORT=1
LDFLAGS  += -s WASM=0 -s LEGACY_VM_SUPPORT=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

endif

CXXFLAGS += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s ERROR_ON_MISSING_LIBRARIES=1 -ffast-math
CFLAGS   += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s ERROR_ON_MISSING_LIBRARIES=1 -ffast-math -fno-strict-aliasing
LDFLAGS  += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s ERROR_ON_MISSING_LIBRARIES=1 -s EXPORT_NAME="'libopenmpt'"

CFLAGS_SILENT += -Wno-\#warnings
CFLAGS_SILENT += -Wno-cast-align
CFLAGS_SILENT += -Wno-cast-qual
CFLAGS_SILENT += -Wno-format
CFLAGS_SILENT += -Wno-missing-prototypes
CFLAGS_SILENT += -Wno-sign-compare
CFLAGS_SILENT += -Wno-unused-function
CFLAGS_SILENT += -Wno-unused-parameter
CFLAGS_SILENT += -Wno-unused-variable

CXXFLAGS_WARNINGS += -Wmissing-declarations
CFLAGS_WARNINGS   += -Wmissing-prototypes

REQUIRES_RUNPREFIX=1

EXESUFFIX=.js
SOSUFFIX=.js
RUNPREFIX=node 
TEST_LDFLAGS= --pre-js build/make/test-pre.js -lnodefs.js 

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
OPTIMIZE=0
OPTIMIZE_SIZE=0

IS_CROSS=1

NO_ZLIB=1
NO_LTDL=1
NO_DL=1
NO_MPG123=1
NO_OGG=1
NO_VORBIS=1
NO_VORBISFILE=1
NO_PORTAUDIO=1
NO_PORTAUDIOCPP=1
NO_PULSEAUDIO=1
NO_SDL=1
NO_SDL2=1
NO_FLAC=1
NO_SNDFILE=1

