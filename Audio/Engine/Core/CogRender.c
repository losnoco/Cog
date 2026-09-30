//
//  CogRender.c
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

#include "CogRender.h"

#include <math.h>

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// MARK: - DoP

static uint8_t dop_marker(float sample) {
	const int32_t packed = (int32_t)llrint((double)sample * 2147483648.0);
	return (uint8_t)(((uint32_t)packed) >> 24);
}

bool cog_dop_validate(const float *frames, size_t channels, size_t count, uint8_t *nextMarker) {
	if(!channels || !count) return false;
	uint8_t previous = 0;
	for(size_t frame = 0; frame < count; ++frame) {
		const uint8_t marker = dop_marker(frames[frame * channels]);
		if(marker != 0x05 && marker != 0xFA) return false;
		if(frame && marker == previous) return false;
		for(size_t channel = 1; channel < channels; ++channel) {
			if(dop_marker(frames[frame * channels + channel]) != marker) return false;
		}
		previous = marker;
	}
	if(nextMarker) {
		*nextMarker = (previous == 0x05) ? 0xFA : 0x05;
	}
	return true;
}

void cog_dop_fill_silence(float *frames, size_t channels, size_t count, uint8_t *marker) {
	uint8_t next = (*marker == 0xFA) ? 0xFA : 0x05;
	for(size_t frame = 0; frame < count; ++frame) {
		const uint32_t packed = ((uint32_t)next << 24) | (0x69U << 16) | (0x69U << 8);
		int32_t word;
		memcpy(&word, &packed, sizeof(word));
		const float silence = (float)((double)word / 2147483648.0);
		for(size_t channel = 0; channel < channels; ++channel) {
			frames[frame * channels + channel] = silence;
		}
		next = (next == 0x05) ? 0xFA : 0x05;
	}
	*marker = next;
}

// MARK: - Gain

struct CogGain {
	// Written by the controlling thread under a sequence lock: the sequence is
	// odd while a request is being written, and the render thread takes a
	// request only if it reads the same even sequence before and after it,
	// so it never mixes the fields of two requests.
	_Atomic uint32_t sequence;
	_Atomic uint32_t fromBits;
	_Atomic uint32_t targetBits;
	_Atomic uint32_t frames;

	// Owned by the render thread.
	uint32_t seenSequence;
	float level;
	float step;
	float rampTarget;
	uint32_t remaining;

	// Published by the render thread.
	_Atomic float published;
	_Atomic bool settled;
};

static uint32_t float_bits(float value) {
	uint32_t bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static float bits_float(uint32_t bits) {
	float value;
	memcpy(&value, &bits, sizeof(value));
	return value;
}

CogGain *cog_gain_create(float initial) {
	CogGain *gain = calloc(1, sizeof(CogGain));
	if(!gain) return NULL;
	atomic_init(&gain->sequence, 0);
	atomic_init(&gain->fromBits, float_bits(NAN));
	atomic_init(&gain->targetBits, float_bits(initial));
	atomic_init(&gain->frames, 0);
	gain->level = initial;
	gain->rampTarget = initial;
	atomic_init(&gain->published, initial);
	atomic_init(&gain->settled, true);
	return gain;
}

void cog_gain_destroy(CogGain *gain) {
	free(gain);
}

void cog_gain_ramp(CogGain *gain, float from, float target, uint32_t frames) {
	const uint32_t sequence = atomic_load_explicit(&gain->sequence, memory_order_relaxed);
	atomic_store_explicit(&gain->sequence, sequence + 1, memory_order_relaxed);
	atomic_thread_fence(memory_order_release);
	atomic_store_explicit(&gain->fromBits, float_bits(from), memory_order_relaxed);
	atomic_store_explicit(&gain->targetBits, float_bits(target), memory_order_relaxed);
	atomic_store_explicit(&gain->frames, frames, memory_order_relaxed);
	atomic_store_explicit(&gain->settled, false, memory_order_relaxed);
	atomic_store_explicit(&gain->sequence, sequence + 2, memory_order_release);
}

void cog_gain_ramp_to(CogGain *gain, float target, uint32_t frames) {
	cog_gain_ramp(gain, NAN, target, frames);
}

float cog_gain_target(const CogGain *gain) {
	return bits_float(atomic_load_explicit(&gain->targetBits, memory_order_relaxed));
}

float cog_gain_current(const CogGain *gain) {
	return atomic_load_explicit(&gain->published, memory_order_relaxed);
}

bool cog_gain_settled(const CogGain *gain) {
	return atomic_load_explicit(&gain->settled, memory_order_acquire);
}

static void gain_take_command(CogGain *gain) {
	const uint32_t before = atomic_load_explicit(&gain->sequence, memory_order_acquire);
	if(before == gain->seenSequence || (before & 1)) return;

	const float from = bits_float(atomic_load_explicit(&gain->fromBits, memory_order_relaxed));
	const float target = bits_float(atomic_load_explicit(&gain->targetBits, memory_order_relaxed));
	const uint32_t frames = atomic_load_explicit(&gain->frames, memory_order_relaxed);
	atomic_thread_fence(memory_order_acquire);
	if(atomic_load_explicit(&gain->sequence, memory_order_relaxed) != before) {
		// Rewritten while being read; take it on the next render.
		return;
	}
	gain->seenSequence = before;

	if(!isnan(from)) {
		gain->level = from;
	}
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

/// Moves the gain through `count` frames without applying it, for audio
/// that must pass untouched. Returns the level reached.
static float gain_advance(CogGain *gain, size_t count) {
	gain_take_command(gain);
	if(gain->remaining) {
		if(count >= gain->remaining) {
			gain->remaining = 0;
			gain->level = gain->rampTarget;
		} else {
			gain->remaining -= (uint32_t)count;
			gain->level += gain->step * (float)count;
		}
	}
	atomic_store_explicit(&gain->published, gain->level, memory_order_relaxed);
	if(!gain->remaining && gain->seenSequence == atomic_load_explicit(&gain->sequence, memory_order_relaxed)) {
		atomic_store_explicit(&gain->settled, true, memory_order_release);
	}
	return gain->level;
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
	if(!gain->remaining && gain->seenSequence == atomic_load_explicit(&gain->sequence, memory_order_relaxed)) {
		atomic_store_explicit(&gain->settled, true, memory_order_release);
	}
}

// MARK: - Renderer

struct CogRenderer {
	CogRing *ring;
	CogGain *volume;
	CogGain *transport;

	bool starved;

	// Seek crossfade, owned by the render thread once running.
	size_t fadeFrames;
	/// cos(pi/2 * k / fadeFrames) for k in 0...fadeFrames.
	float *fadeTable;
	/// The discarded audio fading out, and a buffer to build the next in.
	float *tail;
	float *scratch;
	size_t tailLength;
	size_t tailPosition;
	/// Frames of new audio faded in so far; fadeFrames when not fading in.
	size_t fadeInPosition;
	_Atomic bool crossfadeEnabled;
	_Atomic bool held;

	// DoP, owned by the render thread.
	bool dopActive;
	/// The marker the next DoP frame must carry.
	uint8_t dopMarker;
	/// Float frames rendered before conversion, for integer output.
	float *integerScratch;
	size_t integerScratchFrames;

	_Atomic uint64_t framesRendered;
	_Atomic uint64_t silentFrames;
	_Atomic uint64_t underrunEvents;
	// The device's sample time, render thread only.
	Float64 nextDeviceTime;
	_Atomic bool deviceTimeKnown;
	_Atomic uint64_t deviceDiscontinuities;
	_Atomic int64_t lastDeviceJump;
	_Atomic float peak;
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
	atomic_init(&renderer->peak, 0.0f);
	atomic_init(&renderer->crossfadeEnabled, true);
	renderer->dopMarker = 0x05;
	return renderer;
}

bool cog_renderer_set_integer_output(CogRenderer *renderer, size_t maximumFrames) {
	free(renderer->integerScratch);
	renderer->integerScratch = NULL;
	renderer->integerScratchFrames = 0;
	if(!maximumFrames) return true;
	renderer->integerScratch = calloc(maximumFrames * cog_ring_channels(renderer->ring), sizeof(float));
	if(!renderer->integerScratch) return false;
	renderer->integerScratchFrames = maximumFrames;
	return true;
}

static void renderer_free_crossfade(CogRenderer *renderer) {
	free(renderer->fadeTable);
	free(renderer->tail);
	free(renderer->scratch);
	renderer->fadeTable = NULL;
	renderer->tail = NULL;
	renderer->scratch = NULL;
	renderer->fadeFrames = 0;
	renderer->tailLength = 0;
	renderer->tailPosition = 0;
	renderer->fadeInPosition = 0;
}

bool cog_renderer_set_crossfade_frames(CogRenderer *renderer, size_t frames) {
	renderer_free_crossfade(renderer);
	if(!frames) return true;

	const size_t channels = cog_ring_channels(renderer->ring);
	renderer->fadeTable = malloc((frames + 1) * sizeof(float));
	renderer->tail = calloc(frames * channels, sizeof(float));
	renderer->scratch = calloc(frames * channels, sizeof(float));
	if(!renderer->fadeTable || !renderer->tail || !renderer->scratch) {
		renderer_free_crossfade(renderer);
		return false;
	}
	for(size_t k = 0; k <= frames; ++k) {
		renderer->fadeTable[k] = (float)cos(M_PI_2 * (double)k / (double)frames);
	}
	renderer->fadeTable[frames] = 0.0f;
	renderer->fadeFrames = frames;
	renderer->fadeInPosition = frames;
	return true;
}

void cog_renderer_set_held(CogRenderer *renderer, bool held) {
	atomic_store_explicit(&renderer->held, held, memory_order_relaxed);
}

void cog_renderer_set_crossfade_enabled(CogRenderer *renderer, bool enabled) {
	atomic_store_explicit(&renderer->crossfadeEnabled, enabled, memory_order_relaxed);
}

/// The fade-out gain `position` frames into a tail of `length` frames.
static float fade_out_gain(const CogRenderer *renderer, size_t position, size_t length) {
	return renderer->fadeTable[(uint64_t)position * renderer->fadeFrames / length];
}

/// The fade-in gain `position` frames into the new audio.
static float fade_in_gain(const CogRenderer *renderer, size_t position) {
	return position >= renderer->fadeFrames ? 1.0f : renderer->fadeTable[renderer->fadeFrames - position];
}

/// Honours a pending flush of the ring, keeping what it discards to fade out
/// when crossfading.
static void renderer_take_flush(CogRenderer *renderer, uint32_t channels) {
	CogRing *ring = renderer->ring;
	const size_t fadeFrames = renderer->fadeFrames;
	// A paused transport has nothing audible to fade out.
	// DoP cannot be mixed, so a flush during it is a cut.
	const bool crossfade = fadeFrames && !renderer->dopActive && renderer->transport->level > 0.0f &&
	                       atomic_load_explicit(&renderer->crossfadeEnabled, memory_order_relaxed);

	const uint64_t before = cog_ring_flush_acknowledged(ring);
	size_t kept = 0;
	const size_t discarded = cog_ring_honour_flush_keeping(ring, crossfade ? renderer->scratch : NULL, crossfade ? fadeFrames : 0, &kept);
	if(cog_ring_flush_acknowledged(ring) == before) return;

	// Emptied on purpose, so running dry now is not an underrun.
	if(discarded) renderer->starved = true;

	if(!crossfade) {
		renderer->tailLength = 0;
		renderer->tailPosition = 0;
		renderer->fadeInPosition = fadeFrames;
		return;
	}

	// The new tail is what was about to be heard, as it would have been:
	// the kept frames at the fade-in gain if one was under way, plus the rest
	// of an earlier tail still fading out. It then fades out from there, so
	// a seek during a crossfade does not step either.
	size_t earlier = renderer->tailLength - renderer->tailPosition;
	if(earlier > fadeFrames) earlier = fadeFrames;
	const size_t length = kept > earlier ? kept : earlier;
	for(size_t k = 0; k < length; ++k) {
		float *frame = renderer->scratch + k * channels;
		if(k < kept) {
			const float in = fade_in_gain(renderer, renderer->fadeInPosition + k);
			for(uint32_t channel = 0; channel < channels; ++channel) {
				frame[channel] *= in;
			}
		} else {
			memset(frame, 0, channels * sizeof(float));
		}
		if(k < earlier) {
			const size_t position = renderer->tailPosition + k;
			const float out = fade_out_gain(renderer, position, renderer->tailLength);
			const float *old = renderer->tail + position * channels;
			for(uint32_t channel = 0; channel < channels; ++channel) {
				frame[channel] += old[channel] * out;
			}
		}
	}

	float *swap = renderer->tail;
	renderer->tail = renderer->scratch;
	renderer->scratch = swap;
	renderer->tailLength = length;
	renderer->tailPosition = 0;
	renderer->fadeInPosition = 0;
}

void cog_renderer_destroy(CogRenderer *renderer) {
	if(!renderer) return;
	renderer_free_crossfade(renderer);
	free(renderer->integerScratch);
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

/// The DoP side of a render: `got` frames were read into `out`, the rest
/// zeroed. Returns true if it produced the output (DoP, or DoP silence while
/// DoP is playing); false leaves `out` to the PCM path.
static bool renderer_render_dop(CogRenderer *renderer, float *out, size_t frames, size_t got, uint32_t channels) {
	uint8_t nextMarker = 0x05;
	const bool isDoP = got && cog_dop_validate(out, channels, got, &nextMarker);
	if(got && !isDoP) {
		// PCM again.
		renderer->dopActive = false;
		return false;
	}
	if(!isDoP && !renderer->dopActive) return false;

	if(isDoP) {
		// The marker this slice starts on.
		const uint8_t first = (got % 2) ? ((nextMarker == 0x05) ? 0xFA : 0x05) : nextMarker;
		if(renderer->dopActive && first != renderer->dopMarker) {
			// Dropping one carrier frame is better than repeating a marker,
			// which can make the DAC lose DoP lock at the join.
			memmove(out, out + channels, (got - 1) * channels * sizeof(float));
			--got;
		}
		renderer->dopActive = true;
		renderer->dopMarker = nextMarker;
		// Whatever was fading out cannot be mixed into it.
		renderer->tailLength = 0;
		renderer->tailPosition = 0;
		renderer->fadeInPosition = renderer->fadeFrames;
	}

	// The transport can only pass DoP or silence it: once a pause has
	// ramped it to nothing, what is read is replaced by DoP silence.
	const float transport = gain_advance(renderer->transport, frames);
	gain_advance(renderer->volume, frames);
	if(transport == 0.0f) {
		got = 0;
	}
	if(got < frames) {
		cog_dop_fill_silence(out + got * channels, channels, frames - got, &renderer->dopMarker);
	}
	return true;
}

size_t cog_renderer_render(CogRenderer *renderer, float *out, size_t frames) {
	CogRing *ring = renderer->ring;
	const uint32_t channels = cog_ring_channels(ring);

	renderer_take_flush(renderer, channels);
	if(atomic_load_explicit(&renderer->held, memory_order_relaxed)) {
		// Paused, device running: nothing is read, so nothing is lost.
		memset(out, 0, frames * channels * sizeof(float));
		renderer_render_dop(renderer, out, frames, 0, channels);
		atomic_fetch_add_explicit(&renderer->framesRendered, frames, memory_order_relaxed);
		return 0;
	}
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

	if(renderer_render_dop(renderer, out, frames, got, channels)) {
		atomic_fetch_add_explicit(&renderer->framesRendered, frames, memory_order_relaxed);
		return got;
	}

	// The new audio fades in from its first frame, however late it arrives.
	size_t frame = 0;
	while(frame < got && renderer->fadeInPosition < renderer->fadeFrames) {
		const float in = fade_in_gain(renderer, renderer->fadeInPosition++);
		float *sample = out + frame++ * channels;
		for(uint32_t channel = 0; channel < channels; ++channel) {
			sample[channel] *= in;
		}
	}
	// The discarded audio fades out on its own clock, over any silence too.
	for(frame = 0; frame < frames && renderer->tailPosition < renderer->tailLength; ++frame) {
		const size_t position = renderer->tailPosition++;
		const float gain = fade_out_gain(renderer, position, renderer->tailLength);
		const float *old = renderer->tail + position * channels;
		float *sample = out + frame * channels;
		for(uint32_t channel = 0; channel < channels; ++channel) {
			sample[channel] += old[channel] * gain;
		}
	}

	cog_gain_apply(renderer->transport, out, frames, channels);
	cog_gain_apply(renderer->volume, out, frames, channels);

	float peak = atomic_load_explicit(&renderer->peak, memory_order_relaxed);
	const float before = peak;
	const size_t samples = frames * channels;
	for(size_t i = 0; i < samples; ++i) {
		const float magnitude = out[i] < 0.0f ? -out[i] : out[i];
		if(magnitude > peak) peak = magnitude;
	}
	if(peak > before) {
		atomic_store_explicit(&renderer->peak, peak, memory_order_relaxed);
	}

	atomic_fetch_add_explicit(&renderer->framesRendered, frames, memory_order_relaxed);
	return got;
}

void cog_convert_to_s32(int32_t *output, const float *input, size_t count, bool dop) {
	for(size_t i = 0; i < count; ++i) {
		int32_t value;
		if(dop) {
			// Exactly the carrier word, low byte clear.
			value = (int32_t)(((uint32_t)(int32_t)llrint((double)input[i] * 2147483648.0)) & 0xFFFFFF00U);
		} else if(input[i] >= 1.0f) {
			value = (int32_t)(((uint32_t)INT32_MAX) & 0xFFFFFF00U);
		} else if(input[i] <= -1.0f) {
			value = INT32_MIN;
		} else {
			value = (int32_t)(((uint32_t)(int32_t)llrint((double)input[i] * 2147483647.0)) & 0xFFFFFF00U);
		}
		output[i] = value;
	}
}

OSStatus cog_renderer_audio_unit_render(void *inRefCon,
                                        AudioUnitRenderActionFlags *ioActionFlags,
                                        const AudioTimeStamp *inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList *ioData) {
	(void)ioActionFlags;
	(void)inBusNumber;
	CogRenderer *renderer = (CogRenderer *)inRefCon;
	if(!renderer || !ioData || !ioData->mNumberBuffers || !ioData->mBuffers[0].mData) return noErr;

	if(inTimeStamp && (inTimeStamp->mFlags & kAudioTimeStampSampleTimeValid)) {
		if(atomic_load_explicit(&renderer->deviceTimeKnown, memory_order_relaxed) &&
		   inTimeStamp->mSampleTime != renderer->nextDeviceTime) {
			atomic_store_explicit(&renderer->lastDeviceJump, (int64_t)(inTimeStamp->mSampleTime - renderer->nextDeviceTime), memory_order_relaxed);
			atomic_fetch_add_explicit(&renderer->deviceDiscontinuities, 1, memory_order_relaxed);
		}
		renderer->nextDeviceTime = inTimeStamp->mSampleTime + inNumberFrames;
		atomic_store_explicit(&renderer->deviceTimeKnown, true, memory_order_relaxed);
	}

	const uint32_t channels = cog_ring_channels(renderer->ring);
	const UInt32 bytesPerFrame = (UInt32)(sizeof(float) * channels);
	const UInt32 capacity = ioData->mBuffers[0].mDataByteSize / bytesPerFrame;
	UInt32 frames = inNumberFrames < capacity ? inNumberFrames : capacity;
	if(!renderer->integerScratch) {
		cog_renderer_render(renderer, (float *)ioData->mBuffers[0].mData, frames);
	} else {
		if(frames > renderer->integerScratchFrames) frames = (UInt32)renderer->integerScratchFrames;
		float *scratch = renderer->integerScratch;
		cog_renderer_render(renderer, scratch, frames);
		cog_convert_to_s32((int32_t *)ioData->mBuffers[0].mData, scratch, (size_t)frames * channels, renderer->dopActive);
	}
	ioData->mBuffers[0].mDataByteSize = frames * bytesPerFrame;
	return noErr;
}

uint64_t cog_renderer_frames_rendered(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->framesRendered, memory_order_relaxed);
}

uint64_t cog_renderer_silent_frames(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->silentFrames, memory_order_relaxed);
}

float cog_renderer_take_peak(CogRenderer *renderer) {
	return atomic_exchange_explicit(&renderer->peak, 0.0f, memory_order_relaxed);
}

uint64_t cog_renderer_device_discontinuities(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->deviceDiscontinuities, memory_order_relaxed);
}

int64_t cog_renderer_last_device_jump(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->lastDeviceJump, memory_order_relaxed);
}

void cog_renderer_forget_device_time(CogRenderer *renderer) {
	atomic_store_explicit(&renderer->deviceTimeKnown, false, memory_order_relaxed);
}

uint64_t cog_renderer_underrun_events(const CogRenderer *renderer) {
	return atomic_load_explicit(&renderer->underrunEvents, memory_order_relaxed);
}
