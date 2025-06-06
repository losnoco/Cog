
CXXFLAGS_WARNINGS += -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-conversion -Wframe-larger-than=16000 -Winit-self -Wlogical-op -Wmissing-declarations -Wpointer-arith -Wstrict-aliasing                     -Wsuggest-override -Wundef
CFLAGS_WARNINGS   += -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-conversion                                       -Wlogical-op                                                          -Wstrict-prototypes                    -Wundef

CXXFLAGS_WARNINGS += -Wno-psabi

ifneq ($(NO_NO_UNDEFINED_LINKER_FLAG),1)
LDFLAGS_WARNINGS  += -Wl,--no-undefined
endif

ifeq ($(MODERN),1)
# GCC >= 12
# -Wconversion is way too noisy for earlier GCC versions
CFLAGS_WARNINGS += -Wframe-larger-than=4000
#CXXFLAGS_WARNINGS += -Wshadow -Wswitch-enum 
CXXFLAGS_WARNINGS += -Wconversion
# gold
LDFLAGS_WARNINGS  += -Wl,--detect-odr-violations
# GCC 8
CXXFLAGS_WARNINGS += -Wcast-align=strict
CFLAGS_WARNINGS   += -Wcast-align=strict
endif

CFLAGS_SILENT += -Wno-cast-qual
CFLAGS_SILENT += -Wno-double-promotion
CFLAGS_SILENT += -Wno-empty-body
CFLAGS_SILENT += -Wno-float-conversion
CFLAGS_SILENT += -Wno-implicit-fallthrough
CFLAGS_SILENT += -Wno-old-style-declaration
CFLAGS_SILENT += -Wno-sign-compare
CFLAGS_SILENT += -Wno-stringop-overflow
CFLAGS_SILENT += -Wno-type-limits
CFLAGS_SILENT += -Wno-unused-but-set-variable
CFLAGS_SILENT += -Wno-unused-function
CFLAGS_SILENT += -Wno-unused-parameter

FASTMATH_STYLE=gcc
