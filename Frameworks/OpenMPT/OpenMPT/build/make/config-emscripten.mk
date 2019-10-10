
CC  = emcc
CXX = em++
LD  = em++
AR  = emar

EMSCRIPTEN_TARGET?=default

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

else ifeq ($(EMSCRIPTEN_TARGET),wasm)
# emits native wasm AND an emulator for running wasm in asmjs/js with full wasm optimizations.
# as of emscripten 1.38, this is equivalent to default.
CPPFLAGS += -DMPT_BUILD_WASM
CXXFLAGS += -s WASM=1 -s BINARYEN_METHOD='native-wasm'
CFLAGS   += -s WASM=1 -s BINARYEN_METHOD='native-wasm'
LDFLAGS  += -s WASM=1 -s BINARYEN_METHOD='native-wasm'

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

else ifeq ($(EMSCRIPTEN_TARGET),asmjs128m)
# emits only asmjs
CPPFLAGS += -DMPT_BUILD_ASMJS
CXXFLAGS += -s WASM=0 -s ASM_JS=1
CFLAGS   += -s WASM=0 -s ASM_JS=1
LDFLAGS  += -s WASM=0 -s ASM_JS=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=0 -s ABORTING_MALLOC=0 -s TOTAL_MEMORY=134217728

else ifeq ($(EMSCRIPTEN_TARGET),asmjs)
# emits only asmjs
CPPFLAGS += -DMPT_BUILD_ASMJS
CXXFLAGS += -s WASM=0 -s ASM_JS=1
CFLAGS   += -s WASM=0 -s ASM_JS=1
LDFLAGS  += -s WASM=0 -s ASM_JS=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=0 -s ABORTING_MALLOC=0

else ifeq ($(EMSCRIPTEN_TARGET),js)
# emits only plain javascript with plain javascript focused optimizations.
CPPFLAGS += -DMPT_BUILD_ASMJS
CXXFLAGS += -s WASM=0 -s ASM_JS=2 -s LEGACY_VM_SUPPORT=1
CFLAGS   += -s WASM=0 -s ASM_JS=2 -s LEGACY_VM_SUPPORT=1
LDFLAGS  += -s WASM=0 -s ASM_JS=2 -s LEGACY_VM_SUPPORT=1

LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

endif

CXXFLAGS += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -ffast-math
CFLAGS   += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -ffast-math -fno-strict-aliasing
LDFLAGS  += -s DISABLE_EXCEPTION_CATCHING=0 -s ERROR_ON_UNDEFINED_SYMBOLS=1 -s EXPORT_NAME="'libopenmpt'"

CFLAGS_SILENT += -Wno-unused-parameter -Wno-unused-function -Wno-cast-qual

CXXFLAGS_WARNINGS += -Wmissing-declarations
CFLAGS_WARNINGS   += -Wmissing-prototypes

REQUIRES_RUNPREFIX=1

EXESUFFIX=.js
SOSUFFIX=.js
RUNPREFIX=node 
TEST_LDFLAGS= --pre-js build/make/test-pre.js 

DYNLINK=0
SHARED_LIB=1
STATIC_LIB=0
EXAMPLES=1
OPENMPT123=0
SHARED_SONAME=0

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

