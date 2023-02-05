//
//  rsstate.hpp
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/3/23.
//

#ifndef rsstate_hpp
#define rsstate_hpp

#include "soxr.h"

#include <cmath>
#include <vector>

struct rsstate {
	int channelCount;
	int bufferCapacity;
	size_t remainder;
	uint64_t inProcessed;
	uint64_t outProcessed;
	double sampleRatio;
	double dstRate;
	std::vector<float> SilenceBuf;
	soxr_t Resampler;
	rsstate(int _channelCount, double srcRate, double _dstRate)
	: channelCount(_channelCount), inProcessed(0), outProcessed(0), remainder(0), dstRate(_dstRate) {
		SilenceBuf.resize(1024 * channelCount);
		memset(&SilenceBuf[0], 0, 1024 * channelCount * sizeof(float));
		Resampler = soxr_create(srcRate, dstRate, channelCount, NULL, NULL, NULL, NULL);
		sampleRatio = dstRate / srcRate;
	}

	~rsstate() {
		soxr_delete(Resampler);
	}

	double latency() {
		return (((double)inProcessed * sampleRatio) - (double)outProcessed) / dstRate;
	}

	int resample(const float *input, size_t inCount, size_t *inDone, float *output, size_t outMax) {
		size_t outDone = 0;
		soxr_error_t errmsg = soxr_process(Resampler, (soxr_in_t)input, inCount, inDone, (soxr_out_t)output, outMax, &outDone);
		if(!errmsg) {
			inProcessed += *inDone;
			outProcessed += outDone;
			return (int)outDone;
		} else {
			return 0;
		}
	}

	int flush(float *output, size_t outMax) {
		size_t outTotal = 0;
		uint64_t outputWanted = std::ceil(inProcessed * sampleRatio);
		while(outProcessed < outputWanted) {
			size_t outWanted = outputWanted - outProcessed;
			if(outWanted > outMax) {
				outWanted = outMax;
			}
			size_t outDone = 0;
			size_t inDone = 0;
			soxr_error_t errmsg = soxr_process(Resampler, (soxr_in_t)(&SilenceBuf[0]), 1024, &inDone, (soxr_out_t)output, outWanted, &outDone);
			if(!errmsg) {
				outProcessed += outDone;
				outTotal += outDone;
				output += outDone * channelCount;
				outMax -= outDone;
				if(!outMax || outProcessed == outputWanted) {
					return (int)outTotal;
				}
			} else {
				return 0;
			}
		}
	}
};

#endif /* r8bstate_h */
