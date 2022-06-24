//
//  r8bstate.cpp
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 6/24/22.
//

#include "r8bstate.h"
#include "r8bstate.hpp"

void *r8bstate_new(int channelCount, int bufferCapacity, double srcRate,
                   double dstRate) {
	return (void *)new r8bstate(channelCount, bufferCapacity, srcRate, dstRate);
}

void r8bstate_delete(void *state) {
	delete(r8bstate *)state;
}

double r8bstate_latency(void *state) {
	return ((r8bstate *)state)->latency();
}

int r8bstate_resample(void *state, const float *input, size_t inCount, size_t *inDone,
                      float *output, size_t outMax) {
	return ((r8bstate *)state)->resample(input, inCount, inDone, output, outMax);
}

int r8bstate_flush(void *state, float *output, size_t outMax) {
	return ((r8bstate *)state)->flush(output, outMax);
}
