//
//  r8bstate.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 6/24/22.
//

#include <stdint.h>
#include <stdlib.h>

#ifndef r8bstate_h
#define r8bstate_h

#ifdef __cplusplus
extern "C" {
#endif

void *r8bstate_new(int channelCount, int bufferCapacity, double srcRate,
                   double dstRate);
void r8bstate_delete(void *);

double r8bstate_latency(void *);

int r8bstate_resample(void *, const float *input, size_t inCount, size_t *inDone,
                      float *output, size_t outMax);

int r8bstate_flush(void *, float *output, size_t outMax);

#ifdef __cplusplus
}
#endif

#endif /* r8bstate_h */
