//
//  rsstate.cpp
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/4/23.
//

#include "rsstate.h"
#include "rsstate.hpp"

void *rsstate_new(int channelCount, double srcRate, double dstRate) {
	return (void *)new rsstate(channelCount, srcRate, dstRate);
}

void rsstate_delete(void *state) {
	delete(rsstate *)state;
}

double rsstate_latency(void *state) {
	return ((rsstate *)state)->latency();
}

int rsstate_resample(void *state, const float *input, size_t inCount, size_t *inDone,
                      float *output, size_t outMax) {
	return ((rsstate *)state)->resample(input, inCount, inDone, output, outMax);
}

int rsstate_flush(void *state, float *output, size_t outMax) {
	return ((rsstate *)state)->flush(output, outMax);
}
