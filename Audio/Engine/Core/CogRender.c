//
//  CogRender.c
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

#include "CogRender.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// MARK: - Gain

struct CogGain {
	// Written by the controlling thread. The command packs the target's bits
	// and the ramp length into one word so the render thread never sees a
	// target from one request with the length of another.
	_Atomic uint64_t command;
	_Atomic uint64_t generation;

	// Owned by the render thread.
	uint64_t seenGeneration;
	float level;
	float step;
	float rampTarget;
	uint32_t remaining;

	// Published by the render thread.
	_Atomic float published;
	_Atomic bool settled;
};

static uint64_t pack_command(float target, uint32_t frames) {
	uint32_t bits;
	memcpy(&bits, &target, sizeof(bits));
	return ((uint64_t)bits << 32) | frames;
}

static void unpack_command(uint64_t command, float *target, uint32_t *frames) {
	const uint32_t bits = (uint32_t)(command >> 32);
	memcpy(target, &bits, sizeof(bits));
	*frames = (uint32_t)command;
}

CogGain *cog_gain_create(float initial) {
	CogGain *gain = calloc(1, sizeof(CogGain));
	if(!gain) return NULL;
	atomic_init(&gain->command, pack_command(initial, 0));
	atomic_init(&gain->generation, 0);
	gain->level = initial;
	gain->rampTarget = initial;
	atomic_init(&gain->published, initial);
	atomic_init(&gain->settled, true);
	return gain;
}

void cog_gain_destroy(CogGain *gain) {
	free(gain);
}

void cog_gain_ramp_to(CogGain *gain, float target, uint32_t frames) {
	atomic_store_explicit(&gain->command, pack_command(target, frames), memory_order_relaxed);
	atomic_store_explicit(&gain->settled, false, memory_order_relaxed);
	atomic_fetch_add_explicit(&gain->generation, 1, memory_order_release);
}

float cog_gain_target(const CogGain *gain) {
	float target;
	uint32_t frames;
	unpack_command(atomic_load_explicit(&gain->command, memory_order_relaxed), &target, &frames);
	return target;
}

float cog_gain_current(const CogGain *gain) {
	return atomic_load_explicit(&gain->published, memory_order_relaxed);
}

bool cog_gain_settled(const CogGain *gain) {
	return atomic_load_explicit(&gain->settled, memory_order_acquire);
}

static void gain_take_command(CogGain *gain) {
	const uint64_t generation = atomic_load_explicit(&gain->generation, memory_order_acquire);
	if(generation == gain->seenGeneration) return;
	gain->seenGeneration = generation;

	float target;
	uint32_t frames;
	unpack_command(atomic_load_explicit(&gain->command, memory_order_relaxed), &target, &frames);
	gain->rampTarget = target;
	if(!frames || gain->level == target) {
		gain->level = target;
		gain->remaining = 0;
		gain->step = 0.0f;
	} else {
		gain->remaining = frames;
		gain->step = (target - gain->level) / (float)frames;
	}
}

void cog_gain_apply(CogGain *gain, float *frames, size_t count, uint32_t channels) {
	gain_take_command(gain);

	size_t frame = 0;
	while(frame < count && gain->remaining) {
		const float level = gain->level;
		float *sample = frames + frame * channels;
		for(uint32_t channel = 0; channel < channels; ++channel) {
			sample[channel] *= level;
		}
		gain->level += gain->step;
		++frame;
		if(!--gain->remaining) {
			// Land exactly on the target rather than wherever float
			// accumulation left the ramp.
			gain->level = gain->rampTarget;
		}
	}

	const float level = gain->level;
	if(frame < count && level != 1.0f) {
		float *sample = frames + frame * channels;
		const size_t samples = (count - frame) * channels;
		if(level == 0.0f) {
			memset(sample, 0, samples * sizeof(float));
		} else {
			for(size_t i = 0; i < samples; ++i) {
				sample[i] *= level;
			}
		}
	}

	atomic_store_explicit(&gain->published, gain->level, memory_order_relaxed);
	if(!gain->remaining && gain->seenGeneration == atomic_load_explicit(&gain->generation, memory_order_relaxed)) {
		atomic_store_explicit(&gain->settled, true, memory_order_release);
	}
}

// MARK: - Renderer

struct CogRenderer {
	CogRing *ring;
	CogGain *volume;
	CogGain *transport;

	bool starved;

	_Atomic uint64_t framesRendered;
	_Atomic uint64_t silentFrames;
	_Atomic uint64_t underrunEvents;
};

CogRenderer *cog_renderer_create(CogRing *ring) {
	CogRenderer *renderer = calloc(1, sizeof(CogRenderer));
	if(!renderer) return NULL;
	renderer->ring = ring;
	renderer->volume = cog_gain_create(1.0f);
	renderer->transport = cog_gain_create(1.0f);
	if(!renderer->volume || !renderer->transport) {
		cog_renderer_destroy(renderer);
		return NULL;
	}
	// Nothing has played yet, so an empty ring at start-up is not an underrun.
	renderer->starved = true;
	atomic_init(&renderer->framesRendered, 0);
	atomic_init(&renderer->silentFrames, 0);
	atomic_init(&renderer->underrunEvents, 0);
	return renderer;
}

void cog_renderer_destroy(CogRenderer *renderer) {
	if(!renderer) return;
	cog_gain_destroy(renderer->volume);
	cog_gain_destroy(renderer->transport);
	free(renderer);
}

CogRing *cog_renderer_ring(const CogRenderer *renderer) {
	return renderer->ring;
}

CogGain *cog_renderer_volume(const CogRenderer *renderer) {
	return renderer->volume;
}

CogGain *cog_renderer_transport(const CogRenderer *renderer) {
	return renderer->transport;
}

size_t cog_renderer_render(CogRenderer *renderer, float *out, size_t frames) {
	CogRing *ring = renderer->ring;
	const uint32_t channels = cog_ring_channels(ring);

	cog_ring_honour_flush(ring);
	const size_t got = cog_ring_read(ring, out, frames);
	if(got < frames) {
		memset(out + got * channels, 0, (frames - got) * channels * sizeof(float));
		atomic_fetch_add_explicit(&renderer->silentFrames, frames - got, memory_order_relaxed);
		// Running dry after having had audio, in this call or the last, is an
		// underrun; staying dry is not a new one.
		if(got || !renderer->starved) {
			atomic_fetch_add_explicit(&renderer->underrunEvents, 1, memory_order_relaxed);
		}
	}
	renderer->starved = got < frames;

	cog_gain_apply(renderer->transport, out, frames, channels);
	cog_gain_apply(renderer->volume, out, frames, channels);

	atomic_fetch_add_explicit(&renderer->framesRendered, frames, memory_order_relaxed);
	return got;
}

OSStatus cog_renderer_audio_unit_render(void *inRefCon,
                                        AudioUnitRenderActionFlags *ioActionFlags,
                                        const AudioTimeStamp *inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList *ioData) {
	(void)ioActionFlags;
	(void)inTimeStamp;
	(void)inBusNumber;
	CogRenderer *renderer = (CogRenderer *)inRefCon;
	if(!renderer || !ioData || !ioData->mNumberBuffers || !ioData->mBuffers[0].mData) return noErr;

	const UInt32 bytesPerFrame = (UInt32)(sizeof(float) * cog_ring_channels(renderer->ring));
	const UInt32 capacity = ioData->mBuffers[0].mDataByteSize / bytesPerFrame;
	const UInt32 frames = inNumberFrames < capacity ? inNumberFrames : capacity;
	cog_renderer_render(renderer, (float *)ioData->mBuffers[0].mData, frames);
	ioData->mBuffers[0].mDataByteSize = frames * bytesPerFrame;
	return noErr;
}

uint64_t cog_renderer_frames_rendered(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->framesRendered, memory_order_relaxed);
}

uint64_t cog_renderer_silent_frames(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->silentFrames, memory_order_relaxed);
}

uint64_t cog_renderer_underrun_events(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->underrunEvents, memory_order_relaxed);
}
