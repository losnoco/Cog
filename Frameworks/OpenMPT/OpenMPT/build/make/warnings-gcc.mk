
CXXFLAGS_WARNINGS += -Wcast-align -Wcast-qual -Wfloat-conversion -Wframe-larger-than=16000 -Winit-self -Wlogical-op -Wmissing-declarations -Wpointer-arith -Wstrict-aliasing -Wsuggest-override -Wundef
CFLAGS_WARNINGS   += -Wcast-align -Wcast-qual -Wfloat-conversion                                       -Wlogical-op                                                                             -Wundef

CXXFLAGS_WARNINGS += -Wno-psabi

ifeq ($(MODERN),1)
LDFLAGS  += -fuse-ld=gold
CXXFLAGS_WARNINGS += 
CFLAGS_WARNINGS   += -Wframe-larger-than=4000
#CXXFLAGS_WARNINGS += -Wstrict-aliasing -Wpointer-arith -Winit-self -Wshadow -Wswitch-enum -Wstrict-prototypes
LDFLAGS_WARNINGS  += -Wl,-no-undefined -Wl,--detect-odr-violations
# re-renable after 1.29 branch
#CXXFLAGS_WARNINGS += -Wdouble-promotion
#CFLAGS_WARNINGS   += -Wdouble-promotion
endif

CFLAGS_SILENT += -Wno-cast-qual
CFLAGS_SILENT += -Wno-empty-body
CFLAGS_SILENT += -Wno-float-conversion
CFLAGS_SILENT += -Wno-implicit-fallthrough
CFLAGS_SILENT += -Wno-old-style-declaration
CFLAGS_SILENT += -Wno-sign-compare
CFLAGS_SILENT += -Wno-type-limits
CFLAGS_SILENT += -Wno-unused-but-set-variable
CFLAGS_SILENT += -Wno-unused-function
CFLAGS_SILENT += -Wno-unused-parameter
