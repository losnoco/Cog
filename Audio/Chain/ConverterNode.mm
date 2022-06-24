//
//  ConverterNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Accelerate/Accelerate.h>
#import <Foundation/Foundation.h>

#import "ConverterNode.h"

#import "BufferChain.h"
#import "OutputNode.h"

#import "Logging.h"

#import "hdcd_decode2.h"

#ifdef _DEBUG
#import "BadSampleCleaner.h"
#endif

#if !DSD_DECIMATE
#include "dsd2float.h"
#endif

void PrintStreamDesc(AudioStreamBasicDescription *inDesc) {
	if(!inDesc) {
		DLog(@"Can't print a NULL desc!\n");
		return;
	}
	DLog(@"- - - - - - - - - - - - - - - - - - - -\n");
	DLog(@"  Sample Rate:%f\n", inDesc->mSampleRate);
	DLog(@"  Format ID:%s\n", (char *)&inDesc->mFormatID);
	DLog(@"  Format Flags:%X\n", inDesc->mFormatFlags);
	DLog(@"  Bytes per Packet:%d\n", inDesc->mBytesPerPacket);
	DLog(@"  Frames per Packet:%d\n", inDesc->mFramesPerPacket);
	DLog(@"  Bytes per Frame:%d\n", inDesc->mBytesPerFrame);
	DLog(@"  Channels per Frame:%d\n", inDesc->mChannelsPerFrame);
	DLog(@"  Bits per Channel:%d\n", inDesc->mBitsPerChannel);
	DLog(@"- - - - - - - - - - - - - - - - - - - -\n");
}

@implementation ConverterNode

static void *kConverterNodeContext = &kConverterNodeContext;

@synthesize inputFormat;

- (id)initWithController:(id)c previous:(id)p {
	self = [super initWithController:c previous:p];
	if(self) {
		rgInfo = nil;

		inputBuffer = NULL;
		inputBufferSize = 0;
		floatBuffer = NULL;
		floatBufferSize = 0;

		stopping = NO;
		convertEntered = NO;
		paused = NO;

#if DSD_DECIMATE
		dsd2pcm = NULL;
		dsd2pcmCount = 0;
#endif

		hdcd_decoder = NULL;

		lastChunkIn = nil;

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling" options:0 context:kConverterNodeContext];
	}

	return self;
}

extern "C" void scale_by_volume(float *buffer, size_t count, float volume) {
	if(volume != 1.0) {
		size_t unaligned = (uintptr_t)buffer & 15;
		if(unaligned) {
			size_t count3 = unaligned >> 2;
			while(count3 > 0) {
				*buffer++ *= volume;
				count3--;
				count--;
			}
		}

		vDSP_vsmul(buffer, 1, &volume, buffer, 1, count);
	}
}

#if DSD_DECIMATE
/**
 * DSD 2 PCM: Stage 1:
 * Decimate by factor 8
 * (one byte (8 samples) -> one float sample)
 * The bits are processed from least signicifant to most signicicant.
 * @author Sebastian Gesemann
 */

#define dsd2pcm_FILTER_COEFFS_COUNT 64
static const float dsd2pcm_FILTER_COEFFS[64] = {
	0.09712411121659f, 0.09613438994044f, 0.09417884216316f, 0.09130441727307f,
	0.08757947648990f, 0.08309142055179f, 0.07794369263673f, 0.07225228745463f,
	0.06614191680338f, 0.05974199351302f, 0.05318259916599f, 0.04659059631228f,
	0.04008603356890f, 0.03377897290478f, 0.02776684382775f, 0.02213240062966f,
	0.01694232798846f, 0.01224650881275f, 0.00807793792573f, 0.00445323755944f,
	0.00137370697215f, -0.00117318019994f, -0.00321193033831f, -0.00477694265140f,
	-0.00591028841335f, -0.00665946056286f, -0.00707518873201f, -0.00720940203988f,
	-0.00711340642819f, -0.00683632603227f, -0.00642384017266f, -0.00591723006715f,
	-0.00535273320457f, -0.00476118922548f, -0.00416794965654f, -0.00359301524813f,
	-0.00305135909510f, -0.00255339111833f, -0.00210551956895f, -0.00171076760278f,
	-0.00136940723130f, -0.00107957856005f, -0.00083786862365f, -0.00063983084245f,
	-0.00048043272086f, -0.00035442550015f, -0.00025663481039f, -0.00018217573430f,
	-0.00012659899635f, -0.00008597726991f, -0.00005694188820f, -0.00003668060332f,
	-0.00002290670286f, -0.00001380895679f, -0.00000799057558f, -0.00000440385083f,
	-0.00000228567089f, -0.00000109760778f, -0.00000047286430f, -0.00000017129652f,
	-0.00000004282776f, 0.00000000119422f, 0.00000000949179f, 0.00000000747450f
};

struct dsd2pcm_state {
	/*
	 * This is the 2nd half of an even order symmetric FIR
	 * lowpass filter (to be used on a signal sampled at 44100*64 Hz)
	 * Passband is 0-24 kHz (ripples +/- 0.025 dB)
	 * Stopband starts at 176.4 kHz (rejection: 170 dB)
	 * The overall gain is 2.0
	 */

	/* These remain constant for the duration */
	int FILT_LOOKUP_PARTS;
	float *FILT_LOOKUP_TABLE;
	uint8_t *REVERSE_BITS;
	int FIFO_LENGTH;
	int FIFO_OFS_MASK;

	/* These are altered */
	int *fifo;
	int fpos;
};

static void dsd2pcm_free(void *);
static void dsd2pcm_reset(void *);

static void *dsd2pcm_alloc() {
	struct dsd2pcm_state *state = (struct dsd2pcm_state *)calloc(1, sizeof(struct dsd2pcm_state));

	float *FILT_LOOKUP_TABLE;
	double *temp;
	uint8_t *REVERSE_BITS;

	if(!state)
		return NULL;

	state->FILT_LOOKUP_PARTS = (dsd2pcm_FILTER_COEFFS_COUNT + 7) / 8;
	const int FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
	// The current 128 tap FIR leads to an 8 KB lookup table
	state->FILT_LOOKUP_TABLE = (float *)calloc(sizeof(float), FILT_LOOKUP_PARTS << 8);
	if(!state->FILT_LOOKUP_TABLE)
		goto fail;
	FILT_LOOKUP_TABLE = state->FILT_LOOKUP_TABLE;
	temp = (double *)calloc(sizeof(double), 0x100);
	if(!temp)
		goto fail;
	for(int part = 0, sofs = 0, dofs = 0; part < FILT_LOOKUP_PARTS;) {
		memset(temp, 0, 0x100 * sizeof(double));
		for(int bit = 0, bitmask = 0x80; bit < 8 && sofs + bit < dsd2pcm_FILTER_COEFFS_COUNT;) {
			double coeff = dsd2pcm_FILTER_COEFFS[sofs + bit];
			for(int bite = 0; bite < 0x100; bite++) {
				if((bite & bitmask) == 0) {
					temp[bite] -= coeff;
				} else {
					temp[bite] += coeff;
				}
			}
			bit++;
			bitmask >>= 1;
		}
		for(int s = 0; s < 0x100;) {
			FILT_LOOKUP_TABLE[dofs++] = (float)temp[s++];
		}
		part++;
		sofs += 8;
	}
	free(temp);
	{ // calculate FIFO stuff
		int k = 1;
		while(k < FILT_LOOKUP_PARTS * 2) k <<= 1;
		state->FIFO_LENGTH = k;
		state->FIFO_OFS_MASK = k - 1;
	}
	state->REVERSE_BITS = (uint8_t *)calloc(1, 0x100);
	if(!state->REVERSE_BITS)
		goto fail;
	REVERSE_BITS = state->REVERSE_BITS;
	for(int i = 0, j = 0; i < 0x100; i++) {
		REVERSE_BITS[i] = (uint8_t)j;
		// "reverse-increment" of j
		for(int bitmask = 0x80;;) {
			if(((j ^= bitmask) & bitmask) != 0) break;
			if(bitmask == 1) break;
			bitmask >>= 1;
		}
	}

	state->fifo = (int *)calloc(sizeof(int), state->FIFO_LENGTH);
	if(!state->fifo)
		goto fail;

	dsd2pcm_reset(state);

	return (void *)state;

fail:
	dsd2pcm_free(state);
	return NULL;
}

static void *dsd2pcm_dup(void *_state) {
	struct dsd2pcm_state *state = (struct dsd2pcm_state *)_state;
	if(state) {
		struct dsd2pcm_state *newstate = (struct dsd2pcm_state *)calloc(1, sizeof(struct dsd2pcm_state));
		if(newstate) {
			newstate->FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
			newstate->FIFO_LENGTH = state->FIFO_LENGTH;
			newstate->FIFO_OFS_MASK = state->FIFO_OFS_MASK;
			newstate->fpos = state->fpos;

			newstate->FILT_LOOKUP_TABLE = (float *)calloc(sizeof(float), state->FILT_LOOKUP_PARTS << 8);
			if(!newstate->FILT_LOOKUP_TABLE)
				goto fail;

			memcpy(newstate->FILT_LOOKUP_TABLE, state->FILT_LOOKUP_TABLE, sizeof(float) * (state->FILT_LOOKUP_PARTS << 8));

			newstate->REVERSE_BITS = (uint8_t *)calloc(1, 0x100);
			if(!newstate->REVERSE_BITS)
				goto fail;

			memcpy(newstate->REVERSE_BITS, state->REVERSE_BITS, 0x100);

			newstate->fifo = (int *)calloc(sizeof(int), state->FIFO_LENGTH);
			if(!newstate->fifo)
				goto fail;

			memcpy(newstate->fifo, state->fifo, sizeof(int) * state->FIFO_LENGTH);

			return (void *)newstate;
		}

	fail:
		dsd2pcm_free(newstate);
		return NULL;
	}

	return NULL;
}

static void dsd2pcm_free(void *_state) {
	struct dsd2pcm_state *state = (struct dsd2pcm_state *)_state;
	if(state) {
		free(state->fifo);
		free(state->REVERSE_BITS);
		free(state->FILT_LOOKUP_TABLE);
		free(state);
	}
}

static void dsd2pcm_reset(void *_state) {
	struct dsd2pcm_state *state = (struct dsd2pcm_state *)_state;
	const int FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
	int *fifo = state->fifo;
	for(int i = 0; i < FILT_LOOKUP_PARTS; i++) {
		fifo[i] = 0x55;
		fifo[i + FILT_LOOKUP_PARTS] = 0xAA;
	}
	state->fpos = FILT_LOOKUP_PARTS;
}

static int dsd2pcm_latency(void *_state) {
	struct dsd2pcm_state *state = (struct dsd2pcm_state *)_state;
	if(state)
		return state->FIFO_LENGTH;
	else
		return 0;
}

static void dsd2pcm_process(void *_state, const uint8_t *src, size_t sofs, size_t sinc, float *dest, size_t dofs, size_t dinc, size_t len) {
	struct dsd2pcm_state *state = (struct dsd2pcm_state *)_state;
	int bite1, bite2, temp;
	float sample;
	int *fifo = state->fifo;
	const uint8_t *REVERSE_BITS = state->REVERSE_BITS;
	const float *FILT_LOOKUP_TABLE = state->FILT_LOOKUP_TABLE;
	const int FILT_LOOKUP_PARTS = state->FILT_LOOKUP_PARTS;
	const int FIFO_OFS_MASK = state->FIFO_OFS_MASK;
	int fpos = state->fpos;
	while(len > 0) {
		fifo[fpos] = REVERSE_BITS[fifo[fpos]] & 0xFF;
		fifo[(fpos + FILT_LOOKUP_PARTS) & FIFO_OFS_MASK] = src[sofs] & 0xFF;
		sofs += sinc;
		temp = (fpos + 1) & FIFO_OFS_MASK;
		sample = 0;
		for(int k = 0, lofs = 0; k < FILT_LOOKUP_PARTS;) {
			bite1 = fifo[(fpos - k) & FIFO_OFS_MASK];
			bite2 = fifo[(temp + k) & FIFO_OFS_MASK];
			sample += FILT_LOOKUP_TABLE[lofs + bite1] + FILT_LOOKUP_TABLE[lofs + bite2];
			k++;
			lofs += 0x100;
		}
		fpos = temp;
		dest[dofs] = sample;
		dofs += dinc;
		len--;
	}
	state->fpos = fpos;
}

static void convert_dsd_to_f32(float *output, const uint8_t *input, size_t count, size_t channels, void **dsd2pcm) {
	for(size_t channel = 0; channel < channels; ++channel) {
		dsd2pcm_process(dsd2pcm[channel], input, channel, channels, output, channel, channels, count);
	}
}
#else
static void convert_dsd_to_f32(float *output, const uint8_t *input, size_t count, size_t channels) {
	const uint8_t *iptr = input;
	float *optr = output;
	for(size_t index = 0; index < count; ++index) {
		for(size_t channel = 0; channel < channels; ++channel) {
			uint8_t sample = *iptr++;
			cblas_scopy(8, &dsd2float[sample][0], 1, optr++, (int)channels);
		}
		optr += channels * 7;
	}
}
#endif

static void convert_u8_to_s16(int16_t *output, const uint8_t *input, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		uint16_t sample = (input[i] << 8) | input[i];
		sample ^= 0x8080;
		output[i] = (int16_t)(sample);
	}
}

static void convert_s8_to_s16(int16_t *output, const uint8_t *input, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		uint16_t sample = (input[i] << 8) | input[i];
		output[i] = (int16_t)(sample);
	}
}

static void convert_u16_to_s16(int16_t *buffer, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		buffer[i] ^= 0x8000;
	}
}

static void convert_s16_to_hdcd_input(int32_t *output, const int16_t *input, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		output[i] = input[i];
	}
}

static void convert_s24_to_s32(int32_t *output, const uint8_t *input, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		int32_t sample = (input[i * 3] << 8) | (input[i * 3 + 1] << 16) | (input[i * 3 + 2] << 24);
		output[i] = sample;
	}
}

static void convert_u24_to_s32(int32_t *output, const uint8_t *input, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		int32_t sample = (input[i * 3] << 8) | (input[i * 3 + 1] << 16) | (input[i * 3 + 2] << 24);
		output[i] = sample ^ 0x80000000;
	}
}

static void convert_u32_to_s32(int32_t *buffer, size_t count) {
	for(size_t i = 0; i < count; ++i) {
		buffer[i] ^= 0x80000000;
	}
}

static void convert_f64_to_f32(float *output, const double *input, size_t count) {
	vDSP_vdpsp(input, 1, output, 1, count);
}

static void convert_be_to_le(uint8_t *buffer, size_t bitsPerSample, size_t bytes) {
	size_t i;
	bitsPerSample = (bitsPerSample + 7) / 8;
	switch(bitsPerSample) {
		case 2:
			for(i = 0; i < bytes; i += 2) {
				*(int16_t *)buffer = __builtin_bswap16(*(int16_t *)buffer);
				buffer += 2;
			}
			break;

		case 3: {
			union {
				vDSP_int24 int24;
				uint32_t int32;
			} intval;
			intval.int32 = 0;
			for(i = 0; i < bytes; i += 3) {
				intval.int24 = *(vDSP_int24 *)buffer;
				intval.int32 = __builtin_bswap32(intval.int32 << 8);
				*(vDSP_int24 *)buffer = intval.int24;
				buffer += 3;
			}
		} break;

		case 4:
			for(i = 0; i < bytes; i += 4) {
				*(uint32_t *)buffer = __builtin_bswap32(*(uint32_t *)buffer);
				buffer += 4;
			}
			break;

		case 8:
			for(i = 0; i < bytes; i += 8) {
				*(uint64_t *)buffer = __builtin_bswap64(*(uint64_t *)buffer);
				buffer += 8;
			}
			break;
	}
}

- (void)process {
	char writeBuf[CHUNK_SIZE];

	// Removed endOfStream check from here, since we want to be able to flush the converter
	// when the end of stream is reached. Convert function instead processes what it can,
	// and returns 0 samples when it has nothing more to process at the end of stream.
	while([self shouldContinue] == YES) {
		int amountConverted;
		while(paused) {
			usleep(500);
		}
		@autoreleasepool {
			amountConverted = [self convert:writeBuf amount:CHUNK_SIZE];
		}
		if(!amountConverted) {
			if(paused) {
				continue;
			} else if(streamFormatChanged) {
				[self cleanUp];
				[self setupWithInputFormat:newInputFormat withInputConfig:newInputChannelConfig isLossless:rememberedLossless];
				continue;
			} else
				break;
		}
		[self writeData:writeBuf amount:amountConverted];
	}
}

- (int)convert:(void *)dest amount:(int)amount {
	UInt32 ioNumberPackets;
	int amountReadFromFC;
	int amountRead = 0;

	if(stopping)
		return 0;

	convertEntered = YES;

tryagain:
	if(stopping || [self shouldContinue] == NO) {
		convertEntered = NO;
		return amountRead;
	}

	amountReadFromFC = 0;

	if(floatOffset == floatSize) // skip this step if there's still float buffered
		while(inpOffset == inpSize) {
			size_t samplesRead = 0;

			BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
			BOOL isUnsigned = !isFloat && !(inputFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger);
			size_t bitsPerSample = inputFormat.mBitsPerChannel;

			// Approximately the most we want on input
			ioNumberPackets = CHUNK_SIZE;

#if DSD_DECIMATE
			const size_t sizeScale = 3;
#else
			const size_t sizeScale = (bitsPerSample == 1) ? 10 : 3;
#endif

			size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
			if(!inputBuffer || inputBufferSize < newSize)
				inputBuffer = realloc(inputBuffer, inputBufferSize = newSize * sizeScale);

			ssize_t amountToWrite = ioNumberPackets * inputFormat.mBytesPerPacket;

			ssize_t bytesReadFromInput = 0;

			while(bytesReadFromInput < amountToWrite && !stopping && !paused && !streamFormatChanged && [self shouldContinue] == YES && [self endOfStream] == NO) {
				AudioStreamBasicDescription inf;
				uint32_t config;
				if([self peekFormat:&inf channelConfig:&config]) {
					if(config != inputChannelConfig || memcmp(&inf, &inputFormat, sizeof(inf)) != 0) {
						if(inputChannelConfig == 0 && memcmp(&inf, &inputFormat, sizeof(inf)) == 0) {
							inputChannelConfig = config;
							continue;
						} else {
							newInputFormat = inf;
							newInputChannelConfig = config;
							streamFormatChanged = YES;
							break;
						}
					}
				}

				AudioChunk *chunk = [self readChunk:((amountToWrite - bytesReadFromInput) / inputFormat.mBytesPerPacket)];
				inf = [chunk format];
				size_t frameCount = [chunk frameCount];
				config = [chunk channelConfig];
				size_t bytesRead = frameCount * inf.mBytesPerPacket;
				if(frameCount) {
					NSData *samples = [chunk removeSamples:frameCount];
					memcpy(((uint8_t *)inputBuffer) + bytesReadFromInput, [samples bytes], bytesRead);
					lastChunkIn = [[AudioChunk alloc] init];
					[lastChunkIn setFormat:inf];
					[lastChunkIn setChannelConfig:config];
					[lastChunkIn setLossless:[chunk lossless]];
					[lastChunkIn assignSamples:[samples bytes] frameCount:frameCount];
				}
				bytesReadFromInput += bytesRead;
				if(!frameCount) {
					usleep(500);
				}
			}

			BOOL isBigEndian = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsBigEndian);

			if(!bytesReadFromInput) {
				convertEntered = NO;
				return amountRead;
			}

			if(bytesReadFromInput && isBigEndian) {
				// Time for endian swap!
				convert_be_to_le((uint8_t *)inputBuffer, inputFormat.mBitsPerChannel, bytesReadFromInput);
			}

			if(bytesReadFromInput && isFloat && inputFormat.mBitsPerChannel == 64) {
				// Time for precision loss from weird inputs
				samplesRead = bytesReadFromInput / sizeof(double);
				convert_f64_to_f32((float *)(((uint8_t *)inputBuffer) + bytesReadFromInput), (const double *)inputBuffer, samplesRead);
				memmove(inputBuffer, ((uint8_t *)inputBuffer) + bytesReadFromInput, samplesRead * sizeof(float));
				bytesReadFromInput = samplesRead * sizeof(float);
			}

			if(bytesReadFromInput && !isFloat) {
				float gain = 1.0;
				if(bitsPerSample == 1) {
					samplesRead = bytesReadFromInput / inputFormat.mBytesPerPacket;
					size_t buffer_adder = (bytesReadFromInput + 15) & ~15;
					convert_dsd_to_f32((float *)(((uint8_t *)inputBuffer) + buffer_adder), (const uint8_t *)inputBuffer, samplesRead, inputFormat.mChannelsPerFrame
#if DSD_DECIMATE
					                   ,
					                   dsd2pcm
#endif
					);
#if !DSD_DECIMATE
					samplesRead *= 8;
#endif
					memmove(inputBuffer, ((const uint8_t *)inputBuffer) + buffer_adder, samplesRead * inputFormat.mChannelsPerFrame * sizeof(float));
					bitsPerSample = 32;
					bytesReadFromInput = samplesRead * inputFormat.mChannelsPerFrame * sizeof(float);
					isFloat = YES;
				} else if(bitsPerSample <= 8) {
					samplesRead = bytesReadFromInput;
					size_t buffer_adder = (bytesReadFromInput + 1) & ~1;
					if(!isUnsigned)
						convert_s8_to_s16((int16_t *)(((uint8_t *)inputBuffer) + buffer_adder), (const uint8_t *)inputBuffer, samplesRead);
					else
						convert_u8_to_s16((int16_t *)(((uint8_t *)inputBuffer) + buffer_adder), (const uint8_t *)inputBuffer, samplesRead);
					memmove(inputBuffer, ((uint8_t *)inputBuffer) + buffer_adder, samplesRead * 2);
					bitsPerSample = 16;
					bytesReadFromInput = samplesRead * 2;
					isUnsigned = NO;
				}
				if(hdcd_decoder) { // implied bits per sample is 16, produces 32 bit int scale
					samplesRead = bytesReadFromInput / 2;
					if(isUnsigned)
						convert_u16_to_s16((int16_t *)inputBuffer, samplesRead);
					size_t buffer_adder = (bytesReadFromInput + 3) & ~3;
					convert_s16_to_hdcd_input((int32_t *)(((uint8_t *)inputBuffer) + buffer_adder), (int16_t *)inputBuffer, samplesRead);
					memmove(inputBuffer, ((uint8_t *)inputBuffer) + buffer_adder, samplesRead * 4);
					hdcd_process_stereo((hdcd_state_stereo_t *)hdcd_decoder, (int32_t *)inputBuffer, (int)(samplesRead / 2));
					if(((hdcd_state_stereo_t *)hdcd_decoder)->channel[0].sustain &&
					   ((hdcd_state_stereo_t *)hdcd_decoder)->channel[1].sustain) {
						[controller sustainHDCD];
					}
					gain = 2.0;
					bitsPerSample = 32;
					bytesReadFromInput = samplesRead * 4;
					isUnsigned = NO;
				} else if(bitsPerSample <= 16) {
					samplesRead = bytesReadFromInput / 2;
					if(isUnsigned)
						convert_u16_to_s16((int16_t *)inputBuffer, samplesRead);
					size_t buffer_adder = (bytesReadFromInput + 15) & ~15; // vDSP functions expect aligned to four elements
					vDSP_vflt16((const short *)inputBuffer, 1, (float *)(((uint8_t *)inputBuffer) + buffer_adder), 1, samplesRead);
					float scale = 1ULL << 15;
					vDSP_vsdiv((const float *)(((uint8_t *)inputBuffer) + buffer_adder), 1, &scale, (float *)(((uint8_t *)inputBuffer) + buffer_adder), 1, samplesRead);
					memmove(inputBuffer, ((uint8_t *)inputBuffer) + buffer_adder, samplesRead * sizeof(float));
					bitsPerSample = 32;
					bytesReadFromInput = samplesRead * sizeof(float);
					isUnsigned = NO;
					isFloat = YES;
				} else if(bitsPerSample <= 24) {
					samplesRead = bytesReadFromInput / 3;
					size_t buffer_adder = (bytesReadFromInput + 3) & ~3;
					if(isUnsigned)
						convert_u24_to_s32((int32_t *)(((uint8_t *)inputBuffer) + buffer_adder), (uint8_t *)inputBuffer, samplesRead);
					else
						convert_s24_to_s32((int32_t *)(((uint8_t *)inputBuffer) + buffer_adder), (uint8_t *)inputBuffer, samplesRead);
					memmove(inputBuffer, ((uint8_t *)inputBuffer) + buffer_adder, samplesRead * 4);
					bitsPerSample = 32;
					bytesReadFromInput = samplesRead * 4;
					isUnsigned = NO;
				}
				if(!isFloat && bitsPerSample <= 32) {
					samplesRead = bytesReadFromInput / 4;
					if(isUnsigned)
						convert_u32_to_s32((int32_t *)inputBuffer, samplesRead);
					size_t buffer_adder = (bytesReadFromInput + 31) & ~31; // vDSP functions expect aligned to four elements
					vDSP_vflt32((const int *)inputBuffer, 1, (float *)(((uint8_t *)inputBuffer) + buffer_adder), 1, samplesRead);
					float scale = (1ULL << 31) / gain;
					vDSP_vsdiv((const float *)(((uint8_t *)inputBuffer) + buffer_adder), 1, &scale, (float *)(((uint8_t *)inputBuffer) + buffer_adder), 1, samplesRead);
					memmove(inputBuffer, ((uint8_t *)inputBuffer) + buffer_adder, samplesRead * sizeof(float));
					bitsPerSample = 32;
					bytesReadFromInput = samplesRead * sizeof(float);
					isUnsigned = NO;
					isFloat = YES;
				}

#ifdef _DEBUG
				[BadSampleCleaner cleanSamples:(float *)inputBuffer
				                        amount:bytesReadFromInput / sizeof(float)
				                      location:@"post int to float conversion"];
#endif
			}

			// Input now contains bytesReadFromInput worth of floats, in the input sample rate
			inpSize = bytesReadFromInput;
			inpOffset = 0;
		}

	if(inpOffset != inpSize && floatOffset == floatSize) {
#if DSD_DECIMATE
		const float scaleModifier = (inputFormat.mBitsPerChannel == 1) ? 0.5f : 1.0f;
#endif

		size_t inputSamples = (inpSize - inpOffset) / floatFormat.mBytesPerPacket;

		ioNumberPackets = (UInt32)inputSamples;

		ioNumberPackets = (ioNumberPackets + 255) & ~255;

		size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
		if(newSize < (ioNumberPackets * dmFloatFormat.mBytesPerPacket))
			newSize = ioNumberPackets * dmFloatFormat.mBytesPerPacket;
		if(!floatBuffer || floatBufferSize < newSize)
			floatBuffer = realloc(floatBuffer, floatBufferSize = newSize * 3);

		if(stopping) {
			convertEntered = NO;
			return 0;
		}

		size_t inputDone = 0;
		size_t outputDone = 0;

		memcpy(floatBuffer, (((uint8_t *)inputBuffer) + inpOffset), inputSamples * floatFormat.mBytesPerPacket);
		inputDone = inputSamples;
		outputDone = inputSamples;

		inpOffset += inputDone * floatFormat.mBytesPerPacket;

		amountReadFromFC = (int)(outputDone * floatFormat.mBytesPerPacket);

		scale_by_volume((float *)floatBuffer, amountReadFromFC / sizeof(float), volumeScale
#if DSD_DECIMATE
		                                                                        * scaleModifier
#endif
		);

		floatSize = amountReadFromFC;
		floatOffset = 0;
	}

	if(floatOffset == floatSize)
		goto tryagain;

	ioNumberPackets = (amount - amountRead);
	if(ioNumberPackets > (floatSize - floatOffset))
		ioNumberPackets = (UInt32)(floatSize - floatOffset);

	ioNumberPackets -= ioNumberPackets % dmFloatFormat.mBytesPerPacket;

	memcpy(((uint8_t *)dest) + amountRead, ((uint8_t *)floatBuffer) + floatOffset, ioNumberPackets);

	floatOffset += ioNumberPackets;
	amountRead += ioNumberPackets;

	convertEntered = NO;
	return amountRead;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	if(context == kConverterNodeContext) {
		DLog(@"SOMETHING CHANGED!");
		if([keyPath isEqualToString:@"values.volumeScaling"]) {
			// User reset the volume scaling option
			[self refreshVolumeScaling];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

static float db_to_scale(float db) {
	return pow(10.0, db / 20);
}

- (void)refreshVolumeScaling {
	if(rgInfo == nil) {
		volumeScale = 1.0;
		return;
	}

	NSString *scaling = [[NSUserDefaults standardUserDefaults] stringForKey:@"volumeScaling"];
	BOOL useAlbum = [scaling hasPrefix:@"albumGain"];
	BOOL useTrack = useAlbum || [scaling hasPrefix:@"trackGain"];
	BOOL useVolume = useAlbum || useTrack || [scaling isEqualToString:@"volumeScale"];
	BOOL usePeak = [scaling hasSuffix:@"WithPeak"];
	float scale = 1.0;
	float peak = 0.0;
	if(useVolume) {
		id pVolumeScale = [rgInfo objectForKey:@"volume"];
		if(pVolumeScale != nil)
			scale = [pVolumeScale floatValue];
	}
	if(useTrack) {
		id trackGain = [rgInfo objectForKey:@"replayGainTrackGain"];
		id trackPeak = [rgInfo objectForKey:@"replayGainTrackPeak"];
		if(trackGain != nil)
			scale = db_to_scale([trackGain floatValue]);
		if(trackPeak != nil)
			peak = [trackPeak floatValue];
	}
	if(useAlbum) {
		id albumGain = [rgInfo objectForKey:@"replayGainAlbumGain"];
		id albumPeak = [rgInfo objectForKey:@"replayGainAlbumPeak"];
		if(albumGain != nil)
			scale = db_to_scale([albumGain floatValue]);
		if(albumPeak != nil)
			peak = [albumPeak floatValue];
	}
	if(usePeak) {
		if(scale * peak > 1.0)
			scale = 1.0 / peak;
	}
	volumeScale = scale;
}

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf withInputConfig:(uint32_t)inputConfig isLossless:(BOOL)lossless {
	// Make the converter
	inputFormat = inf;

	inputChannelConfig = inputConfig;

	rememberedLossless = lossless;

	// These are the only sample formats we support translating
	BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
	if((!isFloat && !(inputFormat.mBitsPerChannel >= 1 && inputFormat.mBitsPerChannel <= 32)) || (isFloat && !(inputFormat.mBitsPerChannel == 32 || inputFormat.mBitsPerChannel == 64)))
		return NO;

	// These are really placeholders, as we're doing everything internally now
	if(lossless &&
	   inputFormat.mBitsPerChannel == 16 &&
	   inputFormat.mChannelsPerFrame == 2 &&
	   inputFormat.mSampleRate == 44100) {
		// possibly HDCD, run through decoder
		hdcd_decoder = calloc(1, sizeof(hdcd_state_stereo_t));
		hdcd_reset_stereo((hdcd_state_stereo_t *)hdcd_decoder, 44100);
	}

	floatFormat = inputFormat;
	floatFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
	floatFormat.mBitsPerChannel = 32;
	floatFormat.mBytesPerFrame = (32 / 8) * floatFormat.mChannelsPerFrame;
	floatFormat.mBytesPerPacket = floatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;

#if DSD_DECIMATE
	if(inputFormat.mBitsPerChannel == 1) {
		// Decimate this for speed
		floatFormat.mSampleRate *= 1.0 / 8.0;
		dsd2pcmCount = floatFormat.mChannelsPerFrame;
		dsd2pcm = (void **)calloc(dsd2pcmCount, sizeof(void *));
		dsd2pcm[0] = dsd2pcm_alloc();
		dsd2pcmLatency = dsd2pcm_latency(dsd2pcm[0]);
		for(size_t i = 1; i < dsd2pcmCount; ++i) {
			dsd2pcm[i] = dsd2pcm_dup(dsd2pcm[0]);
		}
	}
#endif

	inpOffset = 0;
	inpSize = 0;

	floatOffset = 0;
	floatSize = 0;

	// This is a post resampler, post-down/upmix format

	dmFloatFormat = floatFormat;

	nodeFormat = dmFloatFormat;
	nodeChannelConfig = inputChannelConfig;

	PrintStreamDesc(&inf);
	PrintStreamDesc(&nodeFormat);

	[self refreshVolumeScaling];

	// Move this here so process call isn't running the resampler until it's allocated
	stopping = NO;
	convertEntered = NO;
	streamFormatChanged = NO;
	paused = NO;

	return YES;
}

- (void)dealloc {
	DLog(@"Decoder dealloc");

	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeScaling" context:kConverterNodeContext];

	paused = NO;
	[self cleanUp];
}

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format inputConfig:(uint32_t)inputConfig {
	DLog(@"FORMAT CHANGED");
	paused = YES;
	while(convertEntered) {
		usleep(500);
	}
	[self cleanUp];
	[self setupWithInputFormat:format withInputConfig:inputConfig isLossless:rememberedLossless];
}

- (void)setRGInfo:(NSDictionary *)rgi {
	DLog(@"Setting ReplayGain info");
	rgInfo = rgi;
	[self refreshVolumeScaling];
}

- (void)cleanUp {
	stopping = YES;
	while(convertEntered) {
		usleep(500);
	}
	if(hdcd_decoder) {
		free(hdcd_decoder);
		hdcd_decoder = NULL;
	}
#if DSD_DECIMATE
	if(dsd2pcm && dsd2pcmCount) {
		for(size_t i = 0; i < dsd2pcmCount; ++i) {
			dsd2pcm_free(dsd2pcm[i]);
			dsd2pcm[i] = NULL;
		}
		free(dsd2pcm);
		dsd2pcm = NULL;
	}
#endif
	if(floatBuffer) {
		free(floatBuffer);
		floatBuffer = NULL;
		floatBufferSize = 0;
	}
	if(inputBuffer) {
		free(inputBuffer);
		inputBuffer = NULL;
		inputBufferSize = 0;
	}
	floatOffset = 0;
	floatSize = 0;
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

@end
