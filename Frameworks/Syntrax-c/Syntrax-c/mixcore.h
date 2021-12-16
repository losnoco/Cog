#ifndef MIXCORE_H
#define MIXCORE_H
#include <stdint.h>
#include "jaytrax.h"

#define CLAMP(x, low, high) (x = ((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
//#define CLAMP16(x) if ((int16_t)(x) != x) x = 0x7FFF ^ (x >> 31) //fast 32 -> 16 bit clamp
#define CLAMP16(x) (int16_t)(x) != x ? 0x7FFF ^ (x >> 31) : x //fast 32 -> 16 bit clamp
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CHKPOS(VC) ((VC->samplepos >= 0) && ((VC->curdirecflg && ((VC->samplepos & 0xFFFFFF00) >= VC->looppoint)) || ((VC->samplepos & 0xFFFFFF00) < VC->endpoint)))

void    jaymix_mixCore(JT1Player* SELF, int32_t numSamples);
uint8_t jaymix_setInterp(Interpolator** out, uint8_t id);

#endif