//
//  CogSignalsmith.cpp
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

#include "CogSignalsmith.h"

#include <signalsmith-stretch/signalsmith-stretch.h>

using Stretch = signalsmith::stretch::SignalsmithStretch<float>;

struct CogSignalsmith {
	Stretch stretch;
	int channels;
};

namespace {
// Signalsmith takes anything indexable by channel, then by sample.
struct ConstChannels {
	const float *const *pointers;
	const float *operator[](int channel) const { return pointers[channel]; }
};
struct Channels {
	float *const *pointers;
	float *operator[](int channel) const { return pointers[channel]; }
};
}

CogSignalsmith *cog_signalsmith_create(int channels, float sampleRate) {
	CogSignalsmith *result = new(std::nothrow) CogSignalsmith();
	if(!result) return nullptr;
	result->channels = channels;
	result->stretch.presetDefault(channels, sampleRate);
	return result;
}

void cog_signalsmith_destroy(CogSignalsmith *stretch) {
	delete stretch;
}

void cog_signalsmith_set_transpose(CogSignalsmith *stretch, float factor, float sampleRate) {
	// The node's tonality limit: formants above 8 kHz are left alone.
	stretch->stretch.setTransposeFactor(factor, 8000.0f / sampleRate);
}

void cog_signalsmith_reset(CogSignalsmith *stretch) {
	stretch->stretch.reset();
}

int cog_signalsmith_input_latency(const CogSignalsmith *stretch) {
	return stretch->stretch.inputLatency();
}

int cog_signalsmith_output_latency(const CogSignalsmith *stretch) {
	return stretch->stretch.outputLatency();
}

int cog_signalsmith_output_seek_length(const CogSignalsmith *stretch, float rate) {
	return stretch->stretch.outputSeekLength(rate);
}

void cog_signalsmith_output_seek(CogSignalsmith *stretch, const float *const *inputs, int frames) {
	stretch->stretch.outputSeek(ConstChannels{inputs}, frames);
}

void cog_signalsmith_process(CogSignalsmith *stretch, const float *const *inputs, int inputFrames, float *const *outputs, int outputFrames) {
	stretch->stretch.process(ConstChannels{inputs}, inputFrames, Channels{outputs}, outputFrames);
}

void cog_signalsmith_flush(CogSignalsmith *stretch, float *const *outputs, int outputFrames) {
	stretch->stretch.flush(Channels{outputs}, outputFrames);
}
