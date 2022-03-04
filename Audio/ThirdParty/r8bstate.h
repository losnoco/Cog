//
//  r8bstate.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 3/3/22.
//

#ifndef r8bstate_h
#define r8bstate_h

#include "CDSPResampler.h"
#include "r8bbase.h"

struct r8bstate {
	int channelCount;
	int bufferCapacity;
	size_t remainder;
	uint64_t inProcessed;
	uint64_t outProcessed;
	double sampleRatio;
	r8b::CFixedBuffer<double> InBuf;
	r8b::CFixedBuffer<double> *OutBuf;
	r8b::CDSPResampler24 **Resamps;
	r8bstate(int _channelCount, int _bufferCapacity, double srcRate, double dstRate)
	: channelCount(_channelCount), bufferCapacity(_bufferCapacity), inProcessed(0), outProcessed(0), remainder(0) {
		InBuf.alloc(bufferCapacity);
		OutBuf = new r8b::CFixedBuffer<double>[channelCount];
		Resamps = new r8b::CDSPResampler24 *[channelCount];
		for(int i = 0; i < channelCount; ++i) {
			Resamps[i] = new r8b::CDSPResampler24(srcRate, dstRate, bufferCapacity);
		}
		sampleRatio = dstRate / srcRate;
	}

	~r8bstate() {
		delete[] OutBuf;
		for(int i = 0; i < channelCount; ++i) {
			delete Resamps[i];
		}
		delete[] Resamps;
	}

	int resample(const float *input, size_t inCount, size_t *inDone, float *output, size_t outMax) {
		int ret = 0;
		int i;
		if(inDone) *inDone = 0;
		while(remainder > 0) {
			size_t blockCount = remainder;
			if(blockCount > outMax)
				blockCount = outMax;
			for(i = 0; i < channelCount; ++i) {
				vDSP_vdpsp(&OutBuf[i][0], 1, output + i, channelCount, blockCount);
			}
			remainder -= blockCount;
			output += channelCount * blockCount;
			outMax -= blockCount;
			ret += blockCount;
			if(!outMax)
				return ret;
		}
		while(inCount > 0) {
			size_t blockCount = inCount;
			if(blockCount > bufferCapacity)
				blockCount = bufferCapacity;
			int outputDone;
			for(i = 0; i < channelCount; ++i) {
				double *outputPointer;
				vDSP_vspdp(input + i, channelCount, &InBuf[0], 1, blockCount);
				outputDone = Resamps[i]->process(InBuf, (int)blockCount, outputPointer);
				if(outputDone) {
					if(outputDone > outMax) {
						vDSP_vdpsp(outputPointer, 1, output + i, channelCount, outMax);
						remainder = outputDone - outMax;
						OutBuf[i].alloc((int)remainder);
						memcpy(&OutBuf[i][0], outputPointer + outMax, remainder);
					} else {
						vDSP_vdpsp(outputPointer, 1, output + i, channelCount, outputDone);
					}
				}
			}
			input += channelCount * blockCount;
			output += channelCount * outputDone;
			inCount -= blockCount;
			if(inDone) *inDone += blockCount;
			inProcessed += blockCount;
			outProcessed += outputDone;
			ret += outputDone;
			if(remainder)
				break;
		}
		return ret;
	}

	int flush(float *output, size_t outMax) {
		int ret = 0;
		int i;
		if(remainder > 0) {
			size_t blockCount = remainder;
			if(blockCount > outMax)
				blockCount = outMax;
			for(i = 0; i < channelCount; ++i) {
				vDSP_vdpsp(&OutBuf[i][0], 1, output + i, channelCount, blockCount);
			}
			remainder -= blockCount;
			output += channelCount * blockCount;
			outMax -= blockCount;
			ret += blockCount;
			if(!outMax)
				return ret;
		}
		uint64_t outputWanted = ceil(inProcessed * sampleRatio);
		memset(&InBuf[0], 0, sizeof(double) * bufferCapacity);
		while(outProcessed < outputWanted) {
			int outputDone = 0;
			for(int i = 0; i < channelCount; ++i) {
				double *outputPointer;
				outputDone = Resamps[i]->process(InBuf, bufferCapacity, outputPointer);
				if(outputDone) {
					if(outputDone > (outputWanted - outProcessed))
						outputDone = (int)(outputWanted - outProcessed);
					vDSP_vdpsp(outputPointer, 1, output + i, channelCount, outputDone);
				}
			}
			outProcessed += outputDone;
			output += channelCount * outputDone;
			ret += outputDone;
		}
		return ret;
	}
};

#endif /* r8bstate_h */
