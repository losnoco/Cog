
ifeq ($(origin CC),default)
CC  = i386-pc-msdosdjgpp-gcc
endif
ifeq ($(origin CXX),default)
CXX = i386-pc-msdosdjgpp-g++
endif
ifeq ($(origin LD),default)
LD  = $(CXX)
endif
ifeq ($(origin AR),default)
AR  = i386-pc-msdosdjgpp-ar
endif

# Note that we are using GNU extensions instead of 100% standards-compliant
# mode, because otherwise DJGPP-specific headers/functions are unavailable.
ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX) -fexceptions -frtti -fpermissive
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=gnu++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=gnu++20 -fexceptions -frtti -fpermissive
else
CXXFLAGS_STDCXX = -std=gnu++17 -fexceptions -frtti -fpermissive
endif
ifneq ($(STDC),)
CFLAGS_STDC = -std=$(STDC)
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=gnu17 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c17' ; fi ), c17)
CFLAGS_STDC = -std=gnu17
else
CFLAGS_STDC = -std=gnu11
endif
CXXFLAGS += $(CXXFLAGS_STDCXX) -fallow-store-data-races -fno-threadsafe-statics
CFLAGS   += $(CFLAGS_STDC)     -fallow-store-data-races

CPU?=generic/common

# Enable 128bit SSE registers.
# This requires pure DOS with only CWSDPMI as DOS extender.
# It will not work in a Win9x DOS window, or in WinNT NTVDM.
# It will also not work with almost all other VCPI or DPMI hosts (e.g. EMM386.EXE).
SSE?=0

ifneq ($(SSE),0)
	FPU_NONE   := -mno-80387
	FPU_287    := -m80387 -mfpmath=387 -mno-fancy-math-387
	FPU_387    := -m80387 -mfpmath=387
	FPU_MMX    := -m80387 -mmmx -mfpmath=387
	FPU_3DNOW  := -m80387 -mmmx -m3dnow -mfpmath=387
	FPU_3DNOWA := -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DASSE := -m80387 -mmmx -m3dnow -m3dnowa -mfxsr -msse -mfpmath=sse,387
	FPU_SSE    := -m80387 -mmmx -mfxsr -msse -mfpmath=sse,387
	FPU_SSE2   := -m80387 -mmmx -mfxsr -msse -msse2 -mfpmath=sse
	FPU_SSE3   := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mfpmath=sse
	FPU_SSSE3  := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -mfpmath=sse
	FPU_SSE4_1 := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -msse4.1 -mfpmath=sse
	FPU_SSE4_2 := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mfpmath=sse
	FPU_SSE4A  := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -msse4a -mfpmath=sse
else
	FPU_NONE   := -mno-80387
	FPU_287    := -m80387 -mfpmath=387 -mno-fancy-math-387
	FPU_387    := -m80387 -mfpmath=387
	FPU_MMX    := -m80387 -mmmx -mfpmath=387
	FPU_3DNOW  := -m80387 -mmmx -m3dnow -mfpmath=387
	FPU_3DNOWA := -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DASSE := -mno-sse -mno-fxsr -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_SSE    := -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE2   := -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE3   := -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSSE3  := -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE4_1 := -mno-sse4.1 -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE4_2 := -mno-sse4.2 -mno-sse4.1 -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE4A  := -mno-sse4a -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
endif

OPT_DEF  := -Os
OPT_SIMD := -O3



CACHE_386 :=64  # 0/64/128
CACHE_486 :=128 # 0/64/128/256
CACHE_S7  :=256 # 128/256/512
CACHE_SS7 :=512 # 256/512/1024


CACHE_PENTIUMPRO :=512  # 256/512/1024
CACHE_PENTIUM2   :=512  # 256/512
CACHE_PENTIUM3   :=256  # 256/512
CACHE_PENTIUM4   :=256  # 256/512
CACHE_PENTIUM41  :=512  # 512/1024
CACHE_CORE       :=2048 # 512/1024/2048
CACHE_CORE2      :=2048 # 1024/2048/3072/4096/6144

CACHE_CELERON    :=0    # 0/128/256
CACHE_PENTIUMM   :=1024 # 1024/2048
CACHE_ATOM       :=512  # 512


CACHE_K63        :=256  # 128/256
CACHE_ATHLON     :=512  # 512
CACHE_ATHLONXP   :=256  # 256/512
CACHE_ATHLON64   :=512  # 256/512/1024

CACHE_DURON      :=64   # 64
CACHE_DURONXP    :=64   # 64
CACHE_SEMPRON64  :=128  # 128/256/512



TUNE_586    :=-mtune=pentium
TUNE_586MMX :=-mtune=pentium-mmx
TUNE_686    :=-mtune=pentiumpro
TUNE_686MMX :=-mtune=pentium2
TUNE_686SSE :=-mtune=pentium3
TUNE_686SSE2:=-mtune=pentium-m
TUNE_686SSE3:=-mtune=pentium-m



generic/early     := $(XXX) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)

generic/common    := $(XXX) -march=i386        $(FPU_387)    -mtune=pentium     $(OPT_DEF)
generic/late      := $(XXX) -march=i686        $(FPU_SSSE3)  -mtune=generic     $(OPT_SIMD)



generic/nofpu     := $(X__) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)    # 386SX, 386DX, 486SX, Cyrix Cx486SLC..Cx486S, NexGen Nx586

generic/386       := $(X__) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_DEF)    # 386+387

generic/486       := $(XX_) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)    # 486DX, AMD Am5x86, Cyrix Cx4x86DX..6x86L, NexGen Nx586-PF
generic/486-mmx   := $(___) -march=i486        $(FPU_MMX)    -mtune=winchip-c6  $(OPT_SIMD)   # IDT WinChip-C6, Rise mP6
generic/486-3dnow := $(___) -march=i486        $(FPU_3DNOW)  -mtune=winchip2    $(OPT_SIMD)   # IDT WinChip-2

generic/586       := $(XX_) -march=i586        $(FPU_387)    -mtune=pentium     $(OPT_DEF)    # Intel Pentium, AMD K5
generic/586-mmx   := $(XX_) -march=pentium-mmx $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD)   # Intel Pentium-MMX, AMD K6
generic/586-3dnow := $(XX_) -march=k6-2        $(FPU_3DNOW)  -mtune=k6-2        $(OPT_SIMD)   # AMD K6-2..K6-3

generic/686       := $(___) -march=pentiumpro  $(FPU_387)    -mtune=pentiumpro  $(OPT_DEF)    # Intel Pentium-Pro
generic/686-mmx   := $(XXX) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD)   # Intel Pentium-2.., AMD Bulldozer.., VIA C3-Nehemiah.., Cyrix 6x86MX.., Transmeta Crusoe.., NSC Geode-GX1..
generic/686-3dnow := $(___) -march=i686        $(FPU_3DNOW)  -mtune=c3          $(OPT_SIMD)   # VIA Cyrix-3..C3-Ezra
generic/686-3dnowa:= $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_SIMD)   # AMD Athlon..K10


generic/sse       := $(X__) -march=i686        $(FPU_SSE)    -mtune=pentium3    $(OPT_SIMD)   # Intel Pentium-3.., AMD Athlon-XP.., VIA C3-Nehemiah.., Transmeta Efficeon.., DM&P Vortex86DX3..
generic/sse2      := $(XX_) -march=i686        $(FPU_SSE2)   -mtune=generic     $(OPT_SIMD)   # Intel Pentium-4.., AMD Athlon-64.., VIA C7-Esther.., Transmeta Efficeon..
generic/sse3      := $(___) -march=i686        $(FPU_SSE3)   -mtune=generic     $(OPT_SIMD)   # Intel Core.., AMD Athlon-64-X2.., VIA C7-Esther.., Transmeta Efficeon-88xx..
generic/ssse3     := $(___) -march=i686        $(FPU_SSSE3)  -mtune=generic     $(OPT_SIMD)   # Intel Core-2.., AMD Bobcat.., Via Nano-1000..
generic/sse4_1    := $(___) -march=i686        $(FPU_SSE4_1) -mtune=generic     $(OPT_SIMD)   # Intel Core-1st, AMD Bulldozer.., Via Nano-3000..
generic/sse4_2    := $(___) -march=i686        $(FPU_SSE4_2) -mtune=generic     $(OPT_SIMD)   # Intel Core-1st, AMD Bulldozer.., Via Nano-C..



intel/i386        := $(XX_) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
intel/i486sx      := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
intel/i386+80287  := $(___) -march=i386        $(FPU_287)    -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)

intel/i386+80387  := $(XX_) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
intel/i486dx      := $(XXX) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
intel/pentium     := $(XXX) -march=pentium     $(FPU_387)    -mtune=pentium     $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
intel/pentium-mmx := $(XXX) -march=pentium-mmx $(FPU_MMX)    -mtune=pentium-mmx $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_S7)
intel/pentium-pro := $(___) -march=pentiumpro  $(FPU_387)    -mtune=pentiumpro  $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_PENTIUMPRO)
intel/pentium2    := $(___) -march=pentium2    $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_PENTIUM2)
intel/pentium3    := $(___) -march=pentium3    $(FPU_SSE)    -mtune=pentium3    $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_PENTIUM3)
intel/pentium4    := $(___) -march=pentium4    $(FPU_SSE2)   -mtune=pentium4    $(OPT_SIMD) --param l1-cache-size=8  --param l2-cache-size=$(CACHE_PENTIUM4)
intel/pentium4.1  := $(___) -march=prescott    $(FPU_SSE3)   -mtune=prescott    $(OPT_SIMD) --param l1-cache-size=8  --param l2-cache-size=$(CACHE_PENTIUM41)
intel/core2       := $(___) -march=core2       $(FPU_SSSE3)  -mtune=core2       $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_CORE2)

intel/celeron     := $(___) -march=pentium2    $(FPU_MMX)    -mtune=pentium2    $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_CELERON)
intel/pentium-m   := $(___) -march=pentium-m   $(FPU_SSE2)   -mtune=pentium-m   $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_PENTIUMM)
intel/core        := $(___) -march=pentium-m   $(FPU_SSE3)   -mtune=core2       $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_CORE)
intel/atom        := $(___) -march=bonnell     $(FPU_SSSE3)  -mtune=bonnell     $(OPT_SIMD) --param l1-cache-size=24 --param l2-cache-size=$(CACHE_ATOM)

intel/late        := $(XX_) -march=i686        $(FPU_SSSE3)  -mtune=intel       $(OPT_SIMD) 



amd/am386         := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
amd/am486sx       := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)

amd/am386+80387   := $(___) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
amd/am486dx       := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
amd/am486dxe      := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)
amd/am5x86        := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)
amd/k5            := $(XXX) -march=i586        $(FPU_387)    -mtune=i586        $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
amd/k5-pentium    := $(XXX) -march=i586        $(FPU_387)    -mtune=pentium     $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
amd/k5-pentiumpro := $(XXX) -march=i586        $(FPU_387)    -mtune=pentiumpro  $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
amd/k5-pentium2   := $(XXX) -march=i586        $(FPU_387)    -mtune=pentium2    $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
amd/k5-k6         := $(XXX) -march=i586        $(FPU_387)    -mtune=k6          $(OPT_DEF)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
amd/k6            := $(XXX) -march=k6          $(FPU_MMX)    -mtune=k6          $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_S7)
amd/k6-2          := $(XXX) -march=k6-2        $(FPU_3DNOW)  -mtune=k6-2        $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_SS7)
amd/k6-3          := $(___) -march=k6-3        $(FPU_3DNOW)  -mtune=k6-3        $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_K63)
amd/athlon        := $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLON)
amd/athlon-xp     := $(XX_) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLONXP)
amd/athlon64      := $(X__) -march=k8          $(FPU_SSE2)   -mtune=k8          $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLON64)
amd/athlon64-sse3 := $(___) -march=k8-sse3     $(FPU_SSE3)   -mtune=k8-sse3     $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLON64)
amd/k10           := $(___) -march=amdfam10    $(FPU_SSE4A)  -mtune=amdfam10    $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=512

amd/duron         := $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_DURON)
amd/duron-xp      := $(___) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_DURONXP)
amd/sempron64     := $(___) -march=k8          $(FPU_SSE2)   -mtune=k8          $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_SEMPRON64)

amd/geode-gx      := $(___) -march=geode       $(FPU_3DNOW)  -mtune=geode       $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=0
amd/geode-lx      := $(___) -march=geode       $(FPU_3DNOW)  -mtune=geode       $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=128
amd/geode-nx      := $(___) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=256
amd/bobcat        := $(___) -march=btver1      $(FPU_SSE4A)  -mtune=btver1      $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=512
amd/jaguar        := $(___) -march=btver2      $(FPU_SSE4A)  -mtune=btver2      $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=1024

amd/late-3dnow    := $(XX_) -march=athlon-xp   $(FPU_3DASSE) -mtune=athlon-xp   $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=512
amd/late          := $(XX_) -march=i686        $(FPU_SSE4A)  -mtune=generic     $(OPT_SIMD) 



nexgen/nx586      := $(___) -march=i486        $(FPU_NONE)    $(TUNE_586)       $(OPT_DEF)  --param l1-cache-size=16 --param l2-cache-size=$(CACHE_486)

nexgen/nx586pf    := $(___) -march=i486        $(FPU_387)     $(TUNE_586)       $(OPT_DEF)  --param l1-cache-size=16 --param l2-cache-size=$(CACHE_486)



ibm/386slc        := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_386)
ibm/486slc        := $(___) -march=i486        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_386)
ibm/486bl         := $(___) -march=i486        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)



cyrix/cx486slc    := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)  --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)
cyrix/cx486dlc    := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)  --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)
cyrix/cx4x86s     := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)  --param l1-cache-size=2  --param l2-cache-size=$(CACHE_486)

cyrix/cx4x86dx    := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
cyrix/cx5x86      := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)
cyrix/6x86        := $(XXX) -march=i486        $(FPU_387)     $(TUNE_586)       $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_S7)
cyrix/6x86l       := $(___) -march=i486        $(FPU_387)     $(TUNE_586)       $(OPT_DEF)  --param l1-cache-size=12 --param l2-cache-size=$(CACHE_S7)
cyrix/6x86mx      := $(___) -march=i686        $(FPU_MMX)     $(TUNE_686MMX)    $(OPT_SIMD) --param l1-cache-size=48 --param l2-cache-size=$(CACHE_SS7)

cyrix/mediagx-gx  := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=9  --param l2-cache-size=0
cyrix/mediagx-gxm := $(___) -march=i686        $(FPU_MMX)     $(TUNE_686MMX)    $(OPT_SIMD) --param l1-cache-size=9  --param l2-cache-size=0



nsc/geode-gx1     := $(___) -march=i686        $(FPU_MMX)     $(TUNE_686MMX)    $(OPT_SIMD) --param l1-cache-size=9  --param l2-cache-size=0
nsc/geode-gx2     := $(___) -march=geode       $(FPU_3DNOW)  -mtune=geode       $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=0



idt/winchip-c6    := $(XX_) -march=winchip-c6  $(FPU_MMX)    -mtune=winchip-c6  $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_S7)
idt/winchip2      := $(XX_) -march=winchip2    $(FPU_3DNOW)  -mtune=winchip2    $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_SS7)



via/cyrix3-joshua := $(XX_) -march=i686        $(FPU_3DNOW)   $(TUNE_686MMX)    $(OPT_SIMD) --param l1-cache-size=48 --param l2-cache-size=256
via/cyrix3-samuel := $(___) -march=c3          $(FPU_3DNOW)  -mtune=c3          $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=0
via/c3-samuel2    := $(___) -march=samuel-2    $(FPU_3DNOW)  -mtune=samuel-2    $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=64
via/c3-ezra       := $(___) -march=samuel-2    $(FPU_3DNOW)  -mtune=samuel-2    $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=64
via/c3-nehemiah   := $(XX_) -march=nehemiah    $(FPU_SSE)    -mtune=nehemiah    $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=64
via/c7-esther     := $(XX_) -march=esther      $(FPU_SSE3)   -mtune=esther      $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=128

via/late          := $(XX_) -march=i686        $(FPU_SSE3)   -mtune=esther      $(OPT_SIMD) 



umc/u5s           := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
umc/u5d           := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)



transmeta/crusoe  := $(___) -march=i686        $(FPU_MMX)     $(TUNE_686MMX)    $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=256
transmeta/efficeon:= $(___) -march=i686        $(FPU_SSE2)    $(TUNE_686SSE2)   $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=1024
transmeta/tm8800  := $(___) -march=i686        $(FPU_SSE3)    $(TUNE_686SSE3)   $(OPT_SIMD) --param l1-cache-size=64 --param l2-cache-size=1024



uli/m6117c        := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)



rise/mp6          := $(XX_) -march=i586        $(FPU_MMX)     $(TUNE_586MMX)    $(OPT_SIMD) --param l1-cache-size=8  --param l2-cache-size=$(CACHE_SS7)



sis/55x           := $(___) -march=i586        $(FPU_MMX)     $(TUNE_586MMX)    $(OPT_SIMD) --param l1-cache-size=8  --param l2-cache-size=0



dmnp/m6117d       := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
dmnp/vortex86sx   := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_DEF)  --param l1-cache-size=16 --param l2-cache-size=0

dmnp/vortex86dx   := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86mx   := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_DEF)  --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86     := $(___) -march=i586        $(FPU_MMX)     $(TUNE_586MMX)    $(OPT_SIMD) --param l1-cache-size=8  --param l2-cache-size=0
dmnp/vortex86dx2  := $(___) -march=i586        $(FPU_MMX)     $(TUNE_586MMX)    $(OPT_SIMD) --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86dx3  := $(___) -march=i686        $(FPU_SSE)     $(TUNE_686SSE)    $(OPT_SIMD) --param l1-cache-size=32 --param l2-cache-size=512



ifeq ($($(CPU)),)
$(error unknown CPU)
endif
CPUFLAGS := $($(CPU))

# parse CPU optimization options
ifeq ($(findstring -O3,$(CPUFLAGS)),-O3)
OPTIMIZE=vectorize
CPUFLAGS := $(filter-out -O3,$(CPUFLAGS))
endif
ifeq ($(findstring -Os,$(CPUFLAGS)),-Os)
OPTIMIZE=size
CPUFLAGS := $(filter-out -Os,$(CPUFLAGS))
endif

# Handle the no-FPU case by linking DJGPP's own emulator.
# DJGPP does not provide a suitable soft-float library for -mno-80397.
ifeq ($(findstring -mno-80387,$(CPUFLAGS)),-mno-80387)
CPU_CFLAGS  := $(filter-out -mno-80387,$(CPUFLAGS)) -m80387
CPU_LDFLAGS :=
CPU_LDLIBS  := -lemu
else ifeq ($(findstring -mno-fancy-math-387,$(CPUFLAGS)),-mno-fancy-math-387)
CPU_CFLAGS  := $(filter-out -mno-fancy-math-387,$(CPUFLAGS))
CPU_LDFLAGS :=
CPU_LDLIBS  := -lemu
else
CPU_CFLAGS  := $(CPUFLAGS)
CPU_LDFLAGS :=
CPU_LDLIBS  :=
endif

ifeq ($(FLAVOURED_DIR),1)

EXESUFFIX=.exe
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
FLAVOUR_DIR=$(CPU)-sse/
FLAVOUR_O=.$(subst /,-,$(CPU)-sse)
else
FLAVOUR_DIR=$(CPU)/
FLAVOUR_O=.$(subst /,-,$(CPU))
endif
FLAVOUR_DIR_MADE:=$(shell $(MKDIR_P) bin/$(FLAVOUR_DIR))

else ifeq ($(FLAVOURED_EXE),1)

ifeq ($(CPU),generic/common)
EXESUFFIX=.exe
else
EXESUFFIX:=.exe
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
EXESUFFIX:=-SSE$(EXESUFFIX)
endif
ifeq ($(OPTIMIZE),size)
EXESUFFIX:=-Os$(EXESUFFIX)
else ifeq ($(OPTIMIZE),speed)
EXESUFFIX:=-O2$(EXESUFFIX)
else ifeq ($(OPTIMIZE),vectorize)
EXESUFFIX:=-O3$(EXESUFFIX)
endif
EXESUFFIX:=-$(subst /,-,$(CPU))$(EXESUFFIX)
endif
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
FLAVOUR_O=.$(subst /,-,$(CPU)-sse)
else
FLAVOUR_O=.$(subst /,-,$(CPU))
endif

else

EXESUFFIX=.exe
FLAVOUR_DIR=
FLAVOUR_O=

endif

CPPFLAGS += 
CXXFLAGS += $(CPU_CFLAGS)
CFLAGS   += $(CPU_CFLAGS)
LDFLAGS  += $(CPU_LDFLAGS)
LDLIBS   += -lm $(CPU_LDLIBS)
ARFLAGS  := rcs

OPTIMIZE_FASTMATH=1

# See <https://gcc.gnu.org/bugzilla/show_bug.cgi?id=115049>.
MPT_COMPILER_NOIPARA=1

include build/make/warnings-gcc.mk

DYNLINK=0
SHARED_LIB=0
STATIC_LIB=1
SHARED_SONAME=0

DEBUG=0

IS_CROSS=1

# generates warnings
MPT_COMPILER_NOVISIBILITY=1

# causes crashes on process shutdown with liballegro
MPT_COMPILER_NOGCSECTIONS=1

MPT_COMPILER_NOALLOCAH=1

ifeq ($(OPTIMIZE_LTO),1)
CXXFLAGS += -flto=auto -Wno-attributes
CFLAGS   += -flto=auto -Wno-attributes
endif

ifneq ($(DEBUG),1)
LDFLAGS  += -s
endif

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
NO_SNDFILE=1
NO_FLAC=1

USE_ALLEGRO42=1
