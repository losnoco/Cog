//
//  rsstate.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/4/23.
//

#include <stdint.h>
#include <stdlib.h>

#ifndef rsstate_h
#define rsstate_h

#ifdef __cplusplus
extern "C" {
#endif

void *rsstate_new(int channelCount, double srcRate, double dstRate);
void rsstate_delete(void *);

double rsstate_latency(void *);

int rsstate_resample(void *, const float *input, size_t inCount, size_t *inDone,
                      float *output, size_t outMax);

int rsstate_flush(void *, float *output, size_t outMax);

#ifdef __cplusplus
}
#endif

#endif /* rsstate_h */
