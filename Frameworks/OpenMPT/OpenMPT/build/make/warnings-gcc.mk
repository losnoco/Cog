
CXXFLAGS_WARNINGS += -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-conversion -Wframe-larger-than=16000 -Winit-self -Wlogical-op -Wmissing-declarations -Wpointer-arith -Wstrict-aliasing                     -Wsuggest-override -Wundef
CFLAGS_WARNINGS   += -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-conversion                                       -Wlogical-op                                                          -Wstrict-prototypes                    -Wundef

CXXFLAGS_WARNINGS += -Wno-psabi

ifeq ($(MODERN),1)
CFLAGS_WARNINGS += -Wframe-larger-than=4000
#CXXFLAGS_WARNINGS += -Wshadow -Wswitch-enum 
# gold
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
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
