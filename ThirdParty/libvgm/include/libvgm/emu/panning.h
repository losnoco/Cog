#ifndef __EMU_PANNING_H__
#define __EMU_PANNING_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "../stdtype.h"

#define PANNING_BITS	16	// 16.16 fixed point
#define PANNING_NORMAL	(1 << PANNING_BITS)

// apply panning value to x, x should be within +-16384
#define APPLY_PANNING_S(x, panval)	((x * panval) >> PANNING_BITS)
// apply panning value to x, version for larger values
#define APPLY_PANNING_L(x, panval)	(INT32)(((INT64)x * panval) >> PANNING_BITS)

void Panning_Calculate(INT32 channels[2], INT16 position);
void Panning_Centre(INT32 channels[2]);

#ifdef __cplusplus
}
#endif

#endif	// __EMU_PANNING_H__
