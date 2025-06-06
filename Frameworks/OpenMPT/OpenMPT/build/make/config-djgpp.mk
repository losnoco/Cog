
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

DXE3GEN = i386-pc-msdosdjgpp-dxe3gen
DXE3RES = i386-pc-msdosdjgpp-dxe3res

# Note that we are using GNU extensions instead of 100% standards-compliant
# mode, because otherwise DJGPP-specific headers/functions are unavailable.
ifneq ($(STDCXX),)
CXXFLAGS_STDCXX = -std=$(STDCXX) -fexceptions -frtti -fpermissive
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=gnu++23 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++23' ; fi ), c++23)
CXXFLAGS_STDCXX = -std=gnu++23 -fexceptions -frtti -fpermissive
else ifeq ($(shell printf '\n' > bin/empty.cpp ; if $(CXX) -std=gnu++20 -c bin/empty.cpp -o bin/empty.out > /dev/null 2>&1 ; then echo 'c++20' ; fi ), c++20)
CXXFLAGS_STDCXX = -std=gnu++20 -fexceptions -frtti -fpermissive
else
CXXFLAGS_STDCXX = -std=gnu++17 -fexceptions -frtti -fpermissive
endif
ifneq ($(STDC),)
CFLAGS_STDC = -std=$(STDC)
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=gnu23 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c23' ; fi ), c23)
CFLAGS_STDC = -std=gnu23
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=gnu20 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c20' ; fi ), c20)
CFLAGS_STDC = -std=gnu20
else ifeq ($(shell printf '\n' > bin/empty.c ; if $(CC) -std=gnu17 -c bin/empty.c -o bin/empty.out > /dev/null 2>&1 ; then echo 'c17' ; fi ), c17)
CFLAGS_STDC = -std=gnu17
else
CFLAGS_STDC = -std=gnu11
endif
CXXFLAGS += $(CXXFLAGS_STDCXX) 
CFLAGS   += $(CFLAGS_STDC)
OVERWRITE_CXXFLAGS += -fallow-store-data-races -fno-threadsafe-statics
OVERWRITE_CFLAGS   += -fallow-store-data-races

CPU?=generic/compatible

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
	FPU_3DSSE  := -m80387 -mmmx -m3dnow -m3dnowa -mfxsr -msse -mfpmath=sse,387
	FPU_3DSSE2 := -m80387 -mmmx -m3dnow -m3dnowa -mfxsr -msse -msse2 -mfpmath=sse
	FPU_3DSSE3 := -m80387 -mmmx -m3dnow -m3dnowa -mfxsr -msse -msse2 -msse3 -mfpmath=sse
	FPU_3DSSE4 := -m80387 -mmmx -m3dnow -m3dnowa -mfxsr -msse -msse2 -msse3 -msse4a -mfpmath=sse
	FPU_SSE    := -m80387 -mmmx -mfxsr -msse -mfpmath=sse,387
	FPU_SSE2   := -m80387 -mmmx -mfxsr -msse -msse2 -mfpmath=sse
	FPU_SSE3   := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mfpmath=sse
	FPU_SSSE3  := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -mfpmath=sse
	FPU_SSE4_1 := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -msse4.1 -mfpmath=sse
	FPU_SSE4_2 := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mfpmath=sse
	FPU_SSE4A  := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -msse4a -mfpmath=sse
	FPU_SSSE4A := -m80387 -mmmx -mfxsr -msse -msse2 -msse3 -mssse3 -msse4a -mfpmath=sse
else
	FPU_NONE   := -mno-80387
	FPU_287    := -m80387 -mfpmath=387 -mno-fancy-math-387
	FPU_387    := -m80387 -mfpmath=387
	FPU_MMX    := -m80387 -mmmx -mfpmath=387
	FPU_3DNOW  := -m80387 -mmmx -m3dnow -mfpmath=387
	FPU_3DNOWA := -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DSSE  := -mno-sse -mno-fxsr -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DSSE2 := -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DSSE3 := -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_3DSSE4 := -mno-sse4a -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -m3dnow -m3dnowa -mfpmath=387
	FPU_SSE    := -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE2   := -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE3   := -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSSE3  := -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE4_1 := -mno-sse4.1 -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE4_2 := -mno-sse4.2 -mno-sse4.1 -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSE4A  := -mno-sse4a -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
	FPU_SSSE4A := -mno-sse4a -mno-ssse3 -mno-sse3 -mno-sse2 -mno-sse -mno-fxsr -m80387 -mmmx -mfpmath=387
endif



ifeq ($(OPTIMIZE),default)

OPT_UARCH_EMUL := -Os   # interpreter
OPT_UARCH_CISC := -Os   # non-pipelined, scalar,       in-order,     optimize for size  i386       am386
OPT_UARCH_PIPE := -Os   # pipelined,     scalar,       in-order,     optimize for size  i486       am486 cx486slc
OPT_UARCH_SCAL := -O2   # pipelined,     super-scalar, in-order,     optimize for speed pentium          cx5x86
OPT_UARCH_OOOE := -O2   # pipelined,     super-scalar, out-of-order, optimize for speed pentiumpro k5    cx6x86
OPT_UARCH_COMP := -O2   # recompiler

# vectorize for MMX/3DNOW (64bit wide) (unsupported by GCC)
OPT_UARCH_EMUL_64 := -Os   # interpreter
OPT_UARCH_CISC_64 := -Os   # non-pipelined, scalar,       in-order,     optimize for size
OPT_UARCH_PIPE_64 := -Os   # pipelined,     scalar,       in-order,     optimize for size
OPT_UARCH_SCAL_64 := -O2   # pipelined,     super-scalar, in-order,     optimize for speed pentium-mmx
OPT_UARCH_OOOE_64 := -O2   # pipelined,     super-scalar, out-of-order, optimize for speed pentium2    k6 m2
OPT_UARCH_COMP_64 := -O2   # recompiler

# vectorize for SSE (128bit wide)
ifeq ($(SSE),0)
OPT_UARCH_EMUL_128 := -Os   # interpreter
OPT_UARCH_CISC_128 := -Os   # non-pipelined, scalar,       in-order,     optimize for size
OPT_UARCH_PIPE_128 := -Os   # pipelined,     scalar,       in-order,     optimize for size
OPT_UARCH_SCAL_128 := -O2   # pipelined,     super-scalar, in-order,     optimize for speed
OPT_UARCH_OOOE_128 := -O2   # pipelined,     super-scalar, out-of-order, optimize for speed
OPT_UARCH_COMP_128 := -O2   # recompiler
else
OPT_UARCH_EMUL_128 := -O3   # interpreter
OPT_UARCH_CISC_128 := -O3   # non-pipelined, scalar,       in-order,     optimize for size
OPT_UARCH_PIPE_128 := -O3   # pipelined,     scalar,       in-order,     optimize for size
OPT_UARCH_SCAL_128 := -O3   # pipelined,     super-scalar, in-order,     optimize for speed
OPT_UARCH_OOOE_128 := -O3   # pipelined,     super-scalar, out-of-order, optimize for speed
OPT_UARCH_COMP_128 := -O3   # recompiler
endif

else

OPT_UARCH_EMUL := 
OPT_UARCH_CISC := 
OPT_UARCH_PIPE := 
OPT_UARCH_SCAL := 
OPT_UARCH_OOOE := 
OPT_UARCH_COMP := 

endif



CACHE_386 :=64  # 0/32/64/128
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


CACHE_ATHLON     :=512  # 512
CACHE_ATHLONXP   :=256  # 256/512
CACHE_ATHLON64   :=512  # 256/512/1024

CACHE_DURON      :=64   # 64
CACHE_DURONXP    :=64   # 64
CACHE_SEMPRON64  :=128  # 128/256/512



TUNE_586    :=-mtune=pentium
TUNE_586MMX :=-mtune=pentium-mmx
TUNE_5863DN :=-mtune=k6-2
TUNE_686    :=-mtune=pentiumpro
TUNE_686MMX :=-mtune=pentium2
TUNE_6863DN :=-mtune=athlon
TUNE_686SSE :=-mtune=pentium3
TUNE_686SSE2:=-mtune=pentium-m
TUNE_686SSE3:=-mtune=pentium-m



generic/early        := $(XXX) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)

generic/compatible   := $(XXX) -march=i386        $(FPU_387)    -mtune=pentium     $(OPT_UARCH_CISC)
generic/common       := $(XXX) -march=i486        $(FPU_387)    -mtune=pentium     $(OPT_UARCH_CISC)

generic/late         := $(XXX) -march=i686        $(FPU_SSE2)   -mtune=generic     $(OPT_UARCH_OOOE_128)



virtual/ibmulator    := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_EMUL)

virtual/ao486        := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)

virtual/bochs        := $(___) -march=i686        $(FPU_387)    -mtune=generic     $(OPT_UARCH_EMUL)

virtual/qemu         := $(___) -march=i686        $(FPU_SSE2)   -mtune=generic     $(OPT_UARCH_COMP_128)

virtual/varcem       := $(___) -march=i686        $(FPU_387)    -mtune=generic     $(OPT_UARCH_COMP)
virtual/pcem         := $(___) -march=i686        $(FPU_3DNOW)  -mtune=generic     $(OPT_UARCH_COMP_64)
virtual/86box        := $(___) -march=i686        $(FPU_3DNOW)  -mtune=generic     $(OPT_UARCH_COMP_64)
virtual/pcbox        := $(___) -march=i686        $(FPU_SSE2)   -mtune=generic     $(OPT_UARCH_COMP_128)

virtual/unipcemu     := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_EMUL)

virtual/dosbox       := $(___) -march=i486        $(FPU_387)    -mtune=i386        $(OPT_UARCH_EMUL)
virtual/dosbox-svn   := $(___) -march=i486        $(FPU_387)    -mtune=i386        $(OPT_UARCH_EMUL)
virtual/dosbox-ece   := $(___) -march=i486        $(FPU_387)    -mtune=i386        $(OPT_UARCH_EMUL)
virtual/dosbox-sta   := $(___) -march=i486        $(FPU_387)    -mtune=i386        $(OPT_UARCH_COMP)
virtual/dosbox-x     := $(___) -march=i686        $(FPU_SSE)    -mtune=generic     $(OPT_UARCH_EMUL_128)



generic/nofpu        := $(X__) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)       # 386SX, 386DX, 486SX, Cyrix Cx486SLC..Cx486S, NexGen Nx586

generic/386          := $(X__) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_UARCH_CISC)       # 386+387

generic/486          := $(XX_) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)       # 486DX, AMD Am5x86, Cyrix Cx4x86DX..6x86L, NexGen Nx586-PF

generic/586          := $(XXX) -march=i586        $(FPU_387)    -mtune=pentium     $(OPT_UARCH_SCAL)       # Intel Pentium, AMD K5
generic/586-mmx      := $(XX_) -march=pentium-mmx $(FPU_MMX)    -mtune=pentium-mmx $(OPT_UARCH_SCAL_64)    # Intel Pentium-MMX, AMD K6, IDT WinChip-C6, Rise mP6
generic/586-3dnow    := $(XX_) -march=k6-2        $(FPU_3DNOW)  -mtune=k6-2        $(OPT_UARCH_SCAL_64)    # AMD K6-2..K6-3+, IDT WinChip-2, VIA-C3-Samuel..VIA C3-Ezra

generic/686          := $(___) -march=pentiumpro  $(FPU_387)    -mtune=pentiumpro  $(OPT_UARCH_OOOE)       # Intel Pentium-Pro
generic/686-mmx      := $(XX_) -march=i686        $(FPU_MMX)    -mtune=pentium2    $(OPT_UARCH_OOOE_64)    # Intel Pentium-2.., AMD Bulldozer.., VIA C3-Nehemiah.., Cyrix 6x86MX.., Transmeta Crusoe.., NSC Geode-GX1..
generic/686-3dnow    := $(___) -march=i686        $(FPU_3DNOW)  -mtune=athlon      $(OPT_UARCH_OOOE_64)    # VIA Cyrix-3-Joshua
generic/686-3dnowa   := $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_UARCH_OOOE_64)    # AMD Athlon..K10

generic/sse          := $(___) -march=i686        $(FPU_SSE)    -mtune=pentium3    $(OPT_UARCH_OOOE_128)   # Intel Pentium-3, AMD Athlon-XP, VIA C3-Nehemiah, DM&P Vortex86DX3..

generic/sse2         := $(X__) -march=i686        $(FPU_SSE2)   -mtune=generic     $(OPT_UARCH_OOOE_128)   # Intel Pentium-4.., AMD Athlon-64.., VIA C7-Esther.., Transmeta Efficeon..
generic/sse3         := $(___) -march=i686        $(FPU_SSE3)   -mtune=generic     $(OPT_UARCH_OOOE_128)   # Intel Core.., AMD Athlon-64-X2.., VIA C7-Esther.., Transmeta Efficeon-88xx..
generic/ssse3        := $(___) -march=i686        $(FPU_SSSE3)  -mtune=generic     $(OPT_UARCH_OOOE_128)   # Intel Core-2.., AMD Bobcat.., Via Nano-1000..
generic/sse4_1       := $(___) -march=i686        $(FPU_SSE4_1) -mtune=generic     $(OPT_UARCH_OOOE_128)   # Intel Core-1st, AMD Bulldozer.., Via Nano-3000..
generic/sse4_2       := $(___) -march=i686        $(FPU_SSE4_2) -mtune=generic     $(OPT_UARCH_OOOE_128)   # Intel Core-1st, AMD Bulldozer.., Via Nano-C..



intel/i386           := $(X__) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
intel/i486sx         := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
intel/i386+80287     := $(___) -march=i386        $(FPU_287)    -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)

intel/i386+80387     := $(X__) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
intel/i486sx+i487sx  := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
intel/i486dx         := $(XXX) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
intel/pentium        := $(XXX) -march=pentium     $(FPU_387)    -mtune=pentium     $(OPT_UARCH_SCAL)     --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
intel/pentium-mmx    := $(XXX) -march=pentium-mmx $(FPU_MMX)    -mtune=pentium-mmx $(OPT_UARCH_SCAL_64)  --param l1-cache-size=16 --param l2-cache-size=$(CACHE_S7)
intel/pentium-pro    := $(___) -march=pentiumpro  $(FPU_387)    -mtune=pentiumpro  $(OPT_UARCH_OOOE)     --param l1-cache-size=8  --param l2-cache-size=$(CACHE_PENTIUMPRO)
intel/pentium2       := $(XX_) -march=pentium2    $(FPU_MMX)    -mtune=pentium2    $(OPT_UARCH_OOOE_64)  --param l1-cache-size=16 --param l2-cache-size=$(CACHE_PENTIUM2)
intel/pentium3       := $(X__) -march=pentium3    $(FPU_SSE)    -mtune=pentium3    $(OPT_UARCH_OOOE_128) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_PENTIUM3)
intel/pentium4       := $(XX_) -march=pentium4    $(FPU_SSE2)   -mtune=pentium4    $(OPT_UARCH_OOOE_128) --param l1-cache-size=8  --param l2-cache-size=$(CACHE_PENTIUM4)
intel/pentium4.1     := $(___) -march=prescott    $(FPU_SSE3)   -mtune=prescott    $(OPT_UARCH_OOOE_128) --param l1-cache-size=8  --param l2-cache-size=$(CACHE_PENTIUM41)
intel/core2          := $(___) -march=core2       $(FPU_SSSE3)  -mtune=core2       $(OPT_UARCH_OOOE_128) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_CORE2)

intel/celeron        := $(___) -march=pentium2    $(FPU_MMX)    -mtune=pentium2    $(OPT_UARCH_OOOE_64)  --param l1-cache-size=16 --param l2-cache-size=$(CACHE_CELERON)
intel/pentium-m      := $(___) -march=pentium-m   $(FPU_SSE2)   -mtune=pentium-m   $(OPT_UARCH_OOOE_128) --param l1-cache-size=16 --param l2-cache-size=$(CACHE_PENTIUMM)
intel/core           := $(___) -march=pentium-m   $(FPU_SSE3)   -mtune=core2       $(OPT_UARCH_OOOE_128) --param l1-cache-size=32 --param l2-cache-size=$(CACHE_CORE)
intel/atom           := $(___) -march=bonnell     $(FPU_SSSE3)  -mtune=bonnell     $(OPT_UARCH_SCAL_128) --param l1-cache-size=24 --param l2-cache-size=$(CACHE_ATOM)

intel/late           := $(XX_) -march=i686        $(FPU_SSSE3)  -mtune=intel       $(OPT_UARCH_OOOE_128)



amd/am386            := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
amd/am486sx          := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)

amd/am386+80387      := $(___) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
amd/am486sx+am487sx  := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
amd/am486dx          := $(XX_) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
amd/am486dxe         := $(XX_) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)
amd/am5x86           := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)
amd/k5               := $(X__) -march=i586        $(FPU_387)    -mtune=i586        $(OPT_UARCH_OOOE)     --param l1-cache-size=8  --param l2-cache-size=$(CACHE_S7)
amd/k6               := $(XX_) -march=k6          $(FPU_MMX)    -mtune=k6          $(OPT_UARCH_OOOE_64)  --param l1-cache-size=32 --param l2-cache-size=$(CACHE_S7)
amd/k6-2             := $(XXX) -march=k6-2        $(FPU_3DNOW)  -mtune=k6-2        $(OPT_UARCH_OOOE_64)  --param l1-cache-size=32 --param l2-cache-size=$(CACHE_SS7)
amd/k6-3             := $(___) -march=k6-3        $(FPU_3DNOW)  -mtune=k6-3        $(OPT_UARCH_OOOE_64)  --param l1-cache-size=32 --param l2-cache-size=256
amd/k6-2+            := $(___) -march=k6-3        $(FPU_3DNOW)  -mtune=k6-3        $(OPT_UARCH_OOOE_64)  --param l1-cache-size=32 --param l2-cache-size=128
amd/k6-3+            := $(___) -march=k6-3        $(FPU_3DNOW)  -mtune=k6-3        $(OPT_UARCH_OOOE_64)  --param l1-cache-size=32 --param l2-cache-size=256
amd/athlon           := $(XX_) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_UARCH_OOOE_64)  --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLON)
amd/athlon-xp        := $(XXX) -march=athlon-xp   $(FPU_3DSSE)  -mtune=athlon-xp   $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLONXP)
amd/athlon64         := $(X__) -march=k8          $(FPU_3DSSE2) -mtune=k8          $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLON64)
amd/athlon64-sse3    := $(___) -march=k8-sse3     $(FPU_3DSSE3) -mtune=k8-sse3     $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_ATHLON64)
amd/k10              := $(___) -march=amdfam10    $(FPU_3DSSE4) -mtune=amdfam10    $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=512

amd/duron            := $(X__) -march=athlon      $(FPU_3DNOWA) -mtune=athlon      $(OPT_UARCH_OOOE_64)  --param l1-cache-size=64 --param l2-cache-size=$(CACHE_DURON)
amd/duron-xp         := $(___) -march=athlon-xp   $(FPU_3DSSE)  -mtune=athlon-xp   $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_DURONXP)
amd/sempron64        := $(___) -march=k8          $(FPU_3DSSE2) -mtune=k8          $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=$(CACHE_SEMPRON64)

amd/geode-gx         := $(___) -march=geode       $(FPU_3DNOWA) -mtune=geode       $(OPT_UARCH_OOOE_64)  --param l1-cache-size=16 --param l2-cache-size=0
amd/geode-lx         := $(___) -march=geode       $(FPU_3DNOWA) -mtune=geode       $(OPT_UARCH_OOOE_64)  --param l1-cache-size=64 --param l2-cache-size=128
amd/geode-nx         := $(___) -march=athlon-xp   $(FPU_3DSSE)  -mtune=athlon-xp   $(OPT_UARCH_OOOE_128) --param l1-cache-size=64 --param l2-cache-size=256
amd/bobcat           := $(X__) -march=btver1      $(FPU_SSSE4A) -mtune=btver1      $(OPT_UARCH_OOOE_128) --param l1-cache-size=32 --param l2-cache-size=512
amd/jaguar           := $(___) -march=btver2      $(FPU_SSSE4A) -mtune=btver2      $(OPT_UARCH_OOOE_128) --param l1-cache-size=32 --param l2-cache-size=1024

amd/late             := $(XX_) -march=i686        $(FPU_SSSE4A) -mtune=generic     $(OPT_UARCH_OOOE_128)



ct/38600             := $(___) -march=i386        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
ct/38605             := $(___) -march=i386        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)



nexgen/nx586         := $(___) -march=i486        $(FPU_NONE)   $(TUNE_586)        $(OPT_UARCH_OOOE)     --param l1-cache-size=16 --param l2-cache-size=$(CACHE_486)

nexgen/nx586pf       := $(___) -march=i486        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_OOOE)     --param l1-cache-size=16 --param l2-cache-size=$(CACHE_486)



ibm/386slc           := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_386)
ibm/486slc           := $(___) -march=i486        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_386)
ibm/486bl            := $(___) -march=i486        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)

ibm/386slc+fasmath   := $(___) -march=i386        $(FPU_387)    -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_386)
ibm/486slc+fasmath   := $(___) -march=i486        $(FPU_387)    -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_386)
ibm/486bl+fasmath    := $(___) -march=i486        $(FPU_387)    -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)



cyrix/cx486slc       := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)
cyrix/cx486dlc       := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)
cyrix/cx4x86s        := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=2  --param l2-cache-size=$(CACHE_486)

cyrix/cx486slc+80387 := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)
cyrix/cx486dlc+80387 := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=1  --param l2-cache-size=$(CACHE_386)
cyrix/cx4x86s+cx487s := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=2  --param l2-cache-size=$(CACHE_486)
cyrix/cx4x86dx       := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
cyrix/cx5x86         := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_SCAL)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_486)
cyrix/6x86           := $(XXX) -march=i486        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_OOOE)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_S7)
cyrix/6x86l          := $(___) -march=i486        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_OOOE)     --param l1-cache-size=12 --param l2-cache-size=$(CACHE_S7)
cyrix/6x86mx         := $(XX_) -march=i686        $(FPU_MMX)    $(TUNE_686MMX)     $(OPT_UARCH_OOOE_64)  --param l1-cache-size=48 --param l2-cache-size=$(CACHE_SS7)

cyrix/mediagx-gx     := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_SCAL)     --param l1-cache-size=9  --param l2-cache-size=0
cyrix/mediagx-gxm    := $(___) -march=i686        $(FPU_MMX)    $(TUNE_686MMX)     $(OPT_UARCH_SCAL_64)  --param l1-cache-size=9  --param l2-cache-size=0



nsc/geode-gx1        := $(___) -march=i686        $(FPU_MMX)    $(TUNE_686MMX)     $(OPT_UARCH_SCAL_64)  --param l1-cache-size=9  --param l2-cache-size=0
nsc/geode-gx2        := $(___) -march=geode       $(FPU_3DNOWA) -mtune=geode       $(OPT_UARCH_OOOE_64)  --param l1-cache-size=16 --param l2-cache-size=0



idt/winchip-c6       := $(X__) -march=i586        $(FPU_MMX)    -mtune=winchip-c6  $(OPT_UARCH_PIPE_64)  --param l1-cache-size=32 --param l2-cache-size=$(CACHE_S7)
idt/winchip2         := $(X__) -march=i586        $(FPU_3DNOW)  -mtune=winchip2    $(OPT_UARCH_SCAL_64)  --param l1-cache-size=32 --param l2-cache-size=$(CACHE_SS7)



via/cyrix3-joshua    := $(___) -march=i686        $(FPU_3DNOW)  $(TUNE_6863DN)     $(OPT_UARCH_OOOE_64)  --param l1-cache-size=48 --param l2-cache-size=256
via/c3-samuel        := $(___) -march=c3          $(FPU_3DNOW)  -mtune=c3          $(OPT_UARCH_SCAL_64)  --param l1-cache-size=64 --param l2-cache-size=0
via/c3-samuel2       := $(___) -march=samuel-2    $(FPU_3DNOW)  -mtune=samuel-2    $(OPT_UARCH_SCAL_64)  --param l1-cache-size=64 --param l2-cache-size=64
via/c3-ezra          := $(___) -march=samuel-2    $(FPU_3DNOW)  -mtune=samuel-2    $(OPT_UARCH_SCAL_64)  --param l1-cache-size=64 --param l2-cache-size=64
via/c3-nehemiah      := $(___) -march=nehemiah    $(FPU_SSE)    -mtune=nehemiah    $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=64
via/c7-esther        := $(XX_) -march=esther      $(FPU_SSE3)   -mtune=esther      $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=128
via/eden-x2          := $(___) -march=eden-x2     $(FPU_SSE3)   -mtune=eden-x2     $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=64
via/nano             := $(___) -march=nano        $(FPU_SSSE3)  -mtune=nano        $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/nano-1000        := $(___) -march=nano-1000   $(FPU_SSSE3)  -mtune=nano-1000   $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/nano-2000        := $(___) -march=nano-2000   $(FPU_SSSE3)  -mtune=nano-2000   $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/nano-3000        := $(___) -march=nano-3000   $(FPU_SSE4_1) -mtune=nano-3000   $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/nano-4000        := $(___) -march=nano-4000   $(FPU_SSE4_1) -mtune=nano-4000   $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/nano-x2          := $(___) -march=nano-x2     $(FPU_SSE4_1) -mtune=nano-x2     $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/nano-x4          := $(___) -march=nano-x4     $(FPU_SSE4_1) -mtune=nano-x4     $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=1024
via/eden-x4          := $(___) -march=eden-x4     $(FPU_SSE4_2) -mtune=eden-x4     $(OPT_UARCH_SCAL_128) --param l1-cache-size=64 --param l2-cache-size=2048

via/late             := $(XX_) -march=eden-x4     $(FPU_SSE4_2) -mtune=eden-x4     $(OPT_UARCH_SCAL_128)



umc/u5s              := $(___) -march=i486        $(FPU_NONE)   -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)
umc/u5d              := $(___) -march=i486        $(FPU_387)    -mtune=i486        $(OPT_UARCH_PIPE)     --param l1-cache-size=6  --param l2-cache-size=$(CACHE_486)



transmeta/crusoe     := $(X__) -march=i686        $(FPU_MMX)    $(TUNE_686MMX)     $(OPT_UARCH_COMP)     --param l1-cache-size=64 --param l2-cache-size=256
transmeta/efficeon   := $(___) -march=i686        $(FPU_SSE2)   $(TUNE_686SSE2)    $(OPT_UARCH_COMP)     --param l1-cache-size=64 --param l2-cache-size=1024
transmeta/tm8800     := $(___) -march=i686        $(FPU_SSE3)   $(TUNE_686SSE3)    $(OPT_UARCH_COMP)     --param l1-cache-size=64 --param l2-cache-size=1024



uli/m6117c           := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)



rise/mp6             := $(X__) -march=i586        $(FPU_MMX)    $(TUNE_586MMX)     $(OPT_UARCH_SCAL_64)  --param l1-cache-size=8  --param l2-cache-size=$(CACHE_SS7)



sis/55x              := $(___) -march=i586        $(FPU_MMX)    $(TUNE_586MMX)     $(OPT_UARCH_SCAL_64)  --param l1-cache-size=8  --param l2-cache-size=0



dmnp/m6117d          := $(___) -march=i386        $(FPU_NONE)   -mtune=i386        $(OPT_UARCH_CISC)     --param l1-cache-size=0  --param l2-cache-size=$(CACHE_386)
dmnp/vortex86        := $(___) -march=i586        $(FPU_MMX)    $(TUNE_586MMX)     $(OPT_UARCH_SCAL_64)  --param l1-cache-size=8  --param l2-cache-size=0
dmnp/vortex86sx      := $(___) -march=i586        $(FPU_NONE)   $(TUNE_586)        $(OPT_UARCH_SCAL)     --param l1-cache-size=16 --param l2-cache-size=0
dmnp/vortex86dx      := $(___) -march=i586        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_SCAL)     --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86mx      := $(___) -march=i586        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_SCAL)     --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86mxp     := $(___) -march=i586        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_SCAL)     --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86dx2     := $(___) -march=i586        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_SCAL)     --param l1-cache-size=16 --param l2-cache-size=256
dmnp/vortex86ex      := $(___) -march=i586        $(FPU_387)    $(TUNE_586)        $(OPT_UARCH_SCAL)     --param l1-cache-size=16 --param l2-cache-size=128
dmnp/vortex86dx3     := $(___) -march=i686        $(FPU_SSE)    $(TUNE_686SSE)     $(OPT_UARCH_SCAL_128) --param l1-cache-size=32 --param l2-cache-size=256
dmnp/vortex86ex2     := $(___) -march=i686        $(FPU_SSE)    $(TUNE_686SSE)     $(OPT_UARCH_SCAL_128) --param l1-cache-size=32 --param l2-cache-size=128



ifeq ($($(CPU)),)
$(error unknown CPU)
endif
CPUFLAGS := $($(CPU))

# parse CPU optimization options
ifeq ($(findstring -O3,$(CPUFLAGS)),-O3)
MPT_COMPILER_NO_O=1
endif
ifeq ($(findstring -O2,$(CPUFLAGS)),-O2)
MPT_COMPILER_NO_O=1
endif
ifeq ($(findstring -Os,$(CPUFLAGS)),-Os)
MPT_COMPILER_NO_O=1
endif
ifeq ($(findstring -Oz,$(CPUFLAGS)),-Oz)
MPT_COMPILER_NO_O=1
endif
ifeq ($(findstring -O1,$(CPUFLAGS)),-O1)
MPT_COMPILER_NO_O=1
endif
ifeq ($(findstring -O0,$(CPUFLAGS)),-O0)
MPT_COMPILER_NO_O=1
endif
ifeq ($(findstring -Og,$(CPUFLAGS)),-Og)
MPT_COMPILER_NO_O=1
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
SOSUFFIX=.dxe
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
SOSUFFIX=.dxe
else
EXESUFFIX:=.exe
SOSUFFIX=.dxe
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
EXESUFFIX:=-SSE$(EXESUFFIX)
SOSUFFIX:=-SSE$(SOSUFFIX)
endif
ifeq ($(OPTIMIZE),some)
EXESUFFIX:=-O1$(EXESUFFIX)
SOSUFFIX:=-O1$(SOSUFFIX)
else ifeq ($(OPTIMIZE),extrasize)
EXESUFFIX:=-Oz$(EXESUFFIX)
SOSUFFIX:=-Oz$(SOSUFFIX)
else ifeq ($(OPTIMIZE),size)
EXESUFFIX:=-Os$(EXESUFFIX)
SOSUFFIX:=-Os$(SOSUFFIX)
else ifeq ($(OPTIMIZE),speed)
EXESUFFIX:=-O2$(EXESUFFIX)
SOSUFFIX:=-O2$(SOSUFFIX)
else ifeq ($(OPTIMIZE),vectorize)
EXESUFFIX:=-O3$(EXESUFFIX)
SOSUFFIX:=-O3$(SOSUFFIX)
endif
EXESUFFIX:=-$(subst /,-,$(CPU))$(EXESUFFIX)
SOSUFFIX:=-$(subst /,-,$(CPU))$(SOSUFFIX)
endif
ifeq ($(findstring -msse,$(CPUFLAGS)),-msse)
FLAVOUR_O=.$(subst /,-,$(CPU)-sse)
else
FLAVOUR_O=.$(subst /,-,$(CPU))
endif

else

EXESUFFIX=.exe
SOSUFFIX=.dxe
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

ALLOW_LGPL=1
DYNLINK=0
SHARED_LIB=0
STATIC_LIB=1
SHARED_SONAME=0

DEBUG=0

IS_CROSS=1

# generates warnings
MPT_COMPILER_NOVISIBILITY=1

# causes crashes on process shutdown with liballegro
MPT_COMPILER_NOSECTIONS=1
MPT_COMPILER_NOGCSECTIONS=1

MPT_COMPILER_NOALLOCAH=1

NO_SHARED_LINKER_FLAG=1

ENABLE_DXE=1

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
