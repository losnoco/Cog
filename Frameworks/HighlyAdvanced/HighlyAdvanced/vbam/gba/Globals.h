#ifndef GLOBALS_H
#define GLOBALS_H

#ifdef EMU_COMPILE
#include "../common/Types.h"
#include "GBA.h"
#else
#include <HighlyAdvanced/Types.h>
#include <HighlyAdvanced/GBA.h>
#endif

#define VERBOSE_SWI                  1
#define VERBOSE_UNALIGNED_MEMORY     2
#define VERBOSE_ILLEGAL_WRITE        4
#define VERBOSE_ILLEGAL_READ         8
#define VERBOSE_DMA0                16
#define VERBOSE_DMA1                32
#define VERBOSE_DMA2                64
#define VERBOSE_DMA3               128
#define VERBOSE_UNDEFINED          256
#define VERBOSE_AGBPRINT           512
#define VERBOSE_SOUNDOUTPUT       1024

#endif // GLOBALS_H
