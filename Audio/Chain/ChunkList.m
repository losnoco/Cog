//
//  ChunkList.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import <Accelerate/Accelerate.h>

#import "ChunkList.h"

#import "hdcd_decode2.h"

#if !DSD_DECIMATE
#import "dsd2float.h"
#endif

#ifdef _DEBUG
#import "BadSampleCleaner.h"
#endif

static void *kChunkListContext = &kChunkListContext;

#if DSD_DECIMATE
/**
 * DSD 2 PCM: Stage 1:
 * Decimate by factor 8
 * (one byte (8 samples) -> one float sample)
 * The bits are processed from least signicifant to most signicicant.
 * @author Sebastian Gesemann
 */

/**
 * This is the 2nd half of an even order symmetric FIR
 * lowpass filter (to be used on a signal sampled at 44100*64 Hz)
 * Passband is 0-24 kHz (ripples +/- 0.025 dB)
 * Stopband starts at 176.4 kHz (rejection: 170 dB)
 * The overall gain is 2.0
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

static void *dsd2pcm_alloc(void) {
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
		return state->FILT_LOOKUP_PARTS * 8;
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

@implementation ChunkList

@synthesize listDuration;
@synthesize listDurationRatioed;
@synthesize maxDuration;

- (id)initWithMaximumDuration:(double)duration {
	self = [super init];

	if(self) {
		chunkList = [NSMutableArray new];
		listDuration = 0.0;
		listDurationRatioed = 0.0;
		maxDuration = duration;

		inAdder = NO;
		inRemover = NO;
		inPeeker = NO;
		inMerger = NO;
		inConverter = NO;
		stopping = NO;
		
		formatRead = NO;

		inputBuffer = NULL;
		inputBufferSize = 0;

#if DSD_DECIMATE
		dsd2pcm = NULL;
		dsd2pcmCount = 0;
		dsd2pcmLatency = 0;
#endif

		observersRegistered = NO;
	}

	return self;
}

- (void)addObservers {
	if(!observersRegistered) {
		halveDSDVolume = NO;
		enableHDCD = NO;

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.halveDSDVolume" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kChunkListContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableHDCD" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kChunkListContext];

		observersRegistered = YES;
	}
}

- (void)removeObservers {
	if(observersRegistered) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.halveDSDVolume" context:kChunkListContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableHDCD" context:kChunkListContext];

		observersRegistered = NO;
	}
}

- (void)dealloc {
	stopping = YES;
	while(inAdder || inRemover || inPeeker || inMerger || inConverter) {
		usleep(500);
	}
	[self removeObservers];
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
	if(tempData) {
		free(tempData);
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kChunkListContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
	
	if([keyPath isEqualToString:@"values.halveDSDVolume"]) {
		halveDSDVolume = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"halveDSDVolume"];
	} else if([keyPath isEqualToString:@"values.enableHDCD"]) {
		enableHDCD = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"enableHDCD"];
	}
}

- (void)reset {
	@synchronized(chunkList) {
		[chunkList removeAllObjects];
		listDuration = 0.0;
		listDurationRatioed = 0.0;
	}
}

- (BOOL)isEmpty {
	@synchronized(chunkList) {
		return [chunkList count] == 0;
	}
}

- (BOOL)isFull {
	@synchronized (chunkList) {
		return (maxDuration - listDuration) < 0.001;
	}
}

- (void)addChunk:(AudioChunk *)chunk {
	if(stopping) return;

	inAdder = YES;

	const double chunkDuration = [chunk duration];
	const double chunkDurationRatioed = [chunk durationRatioed];

	@synchronized(chunkList) {
		[chunkList addObject:chunk];
		listDuration += chunkDuration;
		listDurationRatioed += chunkDurationRatioed;
	}

	inAdder = NO;
}

- (AudioChunk *)removeSamples:(size_t)maxFrameCount {
	if(stopping) {
		return [AudioChunk new];
	}

	@synchronized(chunkList) {
		inRemover = YES;
		if(![chunkList count]) {
			inRemover = NO;
			return [AudioChunk new];
		}
		AudioChunk *chunk = [chunkList objectAtIndex:0];
		if([chunk frameCount] <= maxFrameCount) {
			[chunkList removeObjectAtIndex:0];
			listDuration -= [chunk duration];
			listDurationRatioed -= [chunk durationRatioed];
			inRemover = NO;
			return chunk;
		}
		double streamTimestamp = [chunk streamTimestamp];
		NSData *removedData = [chunk removeSamples:maxFrameCount];
		AudioChunk *ret = [AudioChunk new];
		[ret setFormat:[chunk format]];
		[ret setChannelConfig:[chunk channelConfig]];
		[ret setLossless:[chunk lossless]];
		[ret setStreamTimestamp:streamTimestamp];
		[ret setStreamTimeRatio:[chunk streamTimeRatio]];
		[ret assignData:removedData];
		if(chunk.resetForward) {
			ret.resetForward = YES;
			chunk.resetForward = NO;
		}
		listDuration -= [ret duration];
		listDurationRatioed -= [ret durationRatioed];
		inRemover = NO;
		return ret;
	}
}

- (AudioChunk *)removeSamplesAsFloat32:(size_t)maxFrameCount {
	if(stopping) {
		return [AudioChunk new];
	}

	@synchronized (chunkList) {
		inRemover = YES;
		if(![chunkList count]) {
			inRemover = NO;
			return [AudioChunk new];
		}
		AudioChunk *chunk = [chunkList objectAtIndex:0];
#if !DSD_DECIMATE
		AudioStreamBasicDescription asbd = [chunk format];
		if(asbd.mBitsPerChannel == 1) {
			maxFrameCount /= 8;
		}
#endif
		if([chunk frameCount] <= maxFrameCount) {
			[chunkList removeObjectAtIndex:0];
			listDuration -= [chunk duration];
			listDurationRatioed -= [chunk durationRatioed];
			inRemover = NO;
			return [self convertChunk:chunk];
		}
		double streamTimestamp = [chunk streamTimestamp];
		NSData *removedData = [chunk removeSamples:maxFrameCount];
		AudioChunk *ret = [AudioChunk new];
		[ret setFormat:[chunk format]];
		[ret setChannelConfig:[chunk channelConfig]];
		[ret setLossless:[chunk lossless]];
		[ret setStreamTimestamp:streamTimestamp];
		[ret setStreamTimeRatio:[chunk streamTimeRatio]];
		[ret assignData:removedData];
		if(chunk.resetForward) {
			ret.resetForward = YES;
			chunk.resetForward = NO;
		}
		listDuration -= [ret duration];
		listDurationRatioed -= [ret durationRatioed];
		inRemover = NO;
		return [self convertChunk:ret];
	}
}

- (AudioChunk *)removeAndMergeSamples:(size_t)maxFrameCount callBlock:(BOOL(NS_NOESCAPE ^ _Nonnull)(void))block {
	if(stopping) {
		return [AudioChunk new];
	}

	inMerger = YES;

	BOOL formatSet = NO;
	AudioStreamBasicDescription currentFormat;
	uint32_t currentChannelConfig = 0;

	double streamTimestamp = 0.0;
	double streamTimeRatio = 1.0;
	BOOL blocked = NO;
	while(![self peekTimestamp:&streamTimestamp timeRatio:&streamTimeRatio]) {
		if((blocked = block())) {
			break;
		}
	}

	if(blocked) {
		inMerger = NO;
		return [AudioChunk new];
	}

	AudioChunk *chunk;
	size_t totalFrameCount = 0;
	AudioChunk *outputChunk = [AudioChunk new];

	[outputChunk setStreamTimestamp:streamTimestamp];
	[outputChunk setStreamTimeRatio:streamTimeRatio];

	while(!stopping && totalFrameCount < maxFrameCount) {
		AudioStreamBasicDescription newFormat;
		uint32_t newChannelConfig;
		if(![self peekFormat:&newFormat channelConfig:&newChannelConfig]) {
			if(block()) {
				break;
			}
			continue;
		}
		if(formatSet &&
		   (memcmp(&newFormat, &currentFormat, sizeof(newFormat)) != 0 ||
			newChannelConfig != currentChannelConfig)) {
			break;
		} else if(!formatSet) {
			[outputChunk setFormat:newFormat];
			[outputChunk setChannelConfig:newChannelConfig];
			currentFormat = newFormat;
			currentChannelConfig = newChannelConfig;
			formatSet = YES;
		}

		chunk = [self removeSamples:maxFrameCount - totalFrameCount];
		if(!chunk || ![chunk frameCount]) {
			if(block()) {
				break;
			}
			continue;
		}

		if([chunk isHDCD]) {
			[outputChunk setHDCD];
		}

		if(chunk.resetForward) {
			outputChunk.resetForward = YES;
		}

		size_t frameCount = [chunk frameCount];
		NSData *sampleData = [chunk removeSamples:frameCount];

		[outputChunk assignData:sampleData];

		totalFrameCount += frameCount;
	}

	if(!totalFrameCount) {
		inMerger = NO;
		return [AudioChunk new];
	}

	inMerger = NO;
	return outputChunk;
}

- (AudioChunk *)removeAndMergeSamplesAsFloat32:(size_t)maxFrameCount callBlock:(BOOL(NS_NOESCAPE ^ _Nonnull)(void))block {
	AudioChunk *ret = [self removeAndMergeSamples:maxFrameCount callBlock:block];
	return [self convertChunk:ret];
}

- (AudioChunk *)convertChunk:(AudioChunk *)inChunk {
	if(stopping) return [AudioChunk new];

	inConverter = YES;

	AudioStreamBasicDescription chunkFormat = [inChunk format];
	if(![inChunk frameCount] ||
	   (chunkFormat.mFormatFlags == kAudioFormatFlagsNativeFloatPacked &&
		chunkFormat.mBitsPerChannel == 32)) {
		inConverter = NO;
		return inChunk;
	}

	uint32_t chunkConfig = [inChunk channelConfig];
	BOOL chunkLossless = [inChunk lossless];
	if(!formatRead || memcmp(&chunkFormat, &inputFormat, sizeof(chunkFormat)) != 0 ||
	   chunkConfig != inputChannelConfig || chunkLossless != inputLossless) {
		formatRead = YES;
		inputFormat = chunkFormat;
		inputChannelConfig = chunkConfig;
		inputLossless = chunkLossless;

		BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
		if((!isFloat && !(inputFormat.mBitsPerChannel >= 1 && inputFormat.mBitsPerChannel <= 32)) || (isFloat && !(inputFormat.mBitsPerChannel == 32 || inputFormat.mBitsPerChannel == 64))) {
			inConverter = NO;
			return [AudioChunk new];
		}

		// These are really placeholders, as we're doing everything internally now
		if(inputLossless &&
		   inputFormat.mBitsPerChannel == 16 &&
		   inputFormat.mChannelsPerFrame == 2 &&
		   inputFormat.mSampleRate == 44100) {
			// possibly HDCD, run through decoder
			[self addObservers];
			if(hdcd_decoder) {
				free(hdcd_decoder);
				hdcd_decoder = NULL;
			}
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
			if(dsd2pcm && dsd2pcmCount) {
				for(size_t i = 0; i < dsd2pcmCount; ++i) {
					dsd2pcm_free(dsd2pcm[i]);
					dsd2pcm[i] = NULL;
				}
				free(dsd2pcm);
				dsd2pcm = NULL;
			}
			dsd2pcmCount = floatFormat.mChannelsPerFrame;
			dsd2pcm = (void **)calloc(dsd2pcmCount, sizeof(void *));
			dsd2pcm[0] = dsd2pcm_alloc();
			dsd2pcmLatency = dsd2pcm_latency(dsd2pcm[0]);
			for(size_t i = 1; i < dsd2pcmCount; ++i) {
				dsd2pcm[i] = dsd2pcm_dup(dsd2pcm[0]);
			}
		}
#endif
	}
	
	NSUInteger samplesRead = [inChunk frameCount];
	
	if(!samplesRead) {
		inConverter = NO;
		return [AudioChunk new];
	}
	
	BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
	BOOL isUnsigned = !isFloat && !(inputFormat.mFormatFlags & kAudioFormatFlagIsSignedInteger);
	size_t bitsPerSample = inputFormat.mBitsPerChannel;
	BOOL isBigEndian = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsBigEndian);

	double streamTimestamp = [inChunk streamTimestamp];

	NSData *inputData = [inChunk removeSamples:samplesRead];

#if DSD_DECIMATE
	const size_t sizeFactor = 3;
#else
	const size_t sizeFactor = (bitsPerSample == 1) ? 9 : 3;
#endif
	size_t newSize = samplesRead * floatFormat.mBytesPerPacket * sizeFactor + 64;
	if(!tempData || tempDataSize < newSize)
		tempData = realloc(tempData, tempDataSize = newSize); // Either two buffers plus padding, and/or double precision in case of endian flip

	// double buffer system, with alignment
	const size_t buffer_adder_base = (samplesRead * floatFormat.mBytesPerPacket + 31) & ~31;

	NSUInteger bytesReadFromInput = samplesRead * inputFormat.mBytesPerPacket;

	uint8_t *inputBuffer = (uint8_t *)[inputData bytes];
	BOOL inputChanged = NO;

	BOOL hdcdSustained = NO;

	if(bytesReadFromInput && isBigEndian) {
		// Time for endian swap!
		memcpy(&tempData[0], [inputData bytes], bytesReadFromInput);
		convert_be_to_le((uint8_t *)(&tempData[0]), inputFormat.mBitsPerChannel, bytesReadFromInput);
		inputBuffer = &tempData[0];
		inputChanged = YES;
	}

	if(bytesReadFromInput && isFloat && bitsPerSample == 64) {
		// Time for precision loss from weird inputs
		const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base * 2 : 0;
		samplesRead = bytesReadFromInput / sizeof(double);
		convert_f64_to_f32((float *)(&tempData[buffer_adder]), (const double *)inputBuffer, samplesRead);
		bytesReadFromInput = samplesRead * sizeof(float);
		inputBuffer = &tempData[buffer_adder];
		inputChanged = YES;
		bitsPerSample = 32;
	}

	if(bytesReadFromInput && !isFloat) {
		float gain = 1.0;
		if(bitsPerSample == 1) {
			const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			samplesRead = bytesReadFromInput / inputFormat.mBytesPerPacket;
			convert_dsd_to_f32((float *)(&tempData[buffer_adder]), (const uint8_t *)inputBuffer, samplesRead, inputFormat.mChannelsPerFrame
#if DSD_DECIMATE
							   ,
							   dsd2pcm
#endif
			);
#if !DSD_DECIMATE
			samplesRead *= 8;
#endif
			bitsPerSample = 32;
			bytesReadFromInput = samplesRead * floatFormat.mBytesPerPacket;
			isFloat = YES;
			inputBuffer = &tempData[buffer_adder];
			inputChanged = YES;
			[self addObservers];
#if DSD_DECIMATE
			if(halveDSDVolume) {
				float scaleFactor = 2.0f;
				vDSP_vsdiv((float *)inputBuffer, 1, &scaleFactor, (float *)inputBuffer, 1, bytesReadFromInput / sizeof(float));
			}
#else
			if(!halveDSDVolume) {
				float scaleFactor = 2.0f;
				vDSP_vsmul((float *)inputBuffer, 1, &scaleFactor, (float *)inputBuffer, 1, bytesReadFromInput / sizeof(float));
			}
#endif
		} else if(bitsPerSample <= 8) {
			samplesRead = bytesReadFromInput;
			const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			if(!isUnsigned)
				convert_s8_to_s16((int16_t *)(&tempData[buffer_adder]), (const uint8_t *)inputBuffer, samplesRead);
			else
				convert_u8_to_s16((int16_t *)(&tempData[buffer_adder]), (const uint8_t *)inputBuffer, samplesRead);
			bitsPerSample = 16;
			bytesReadFromInput = samplesRead * 2;
			isUnsigned = NO;
			inputBuffer = &tempData[buffer_adder];
			inputChanged = YES;
		}
		if(hdcd_decoder) { // implied bits per sample is 16, produces 32 bit int scale
			samplesRead = bytesReadFromInput / 2;
			const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			if(isUnsigned) {
				if(!inputChanged) {
					memcpy(&tempData[buffer_adder], inputBuffer, samplesRead * 2);
					inputBuffer = &tempData[buffer_adder];
					inputChanged = YES;
				}
				convert_u16_to_s16((int16_t *)inputBuffer, samplesRead);
				isUnsigned = NO;
			}
			const size_t buffer_adder2 = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			convert_s16_to_hdcd_input((int32_t *)(&tempData[buffer_adder2]), (int16_t *)inputBuffer, samplesRead);
			hdcd_process_stereo((hdcd_state_stereo_t *)hdcd_decoder, (int32_t *)(&tempData[buffer_adder2]), (int)(samplesRead / 2));
			if(((hdcd_state_stereo_t *)hdcd_decoder)->channel[0].sustain &&
			   ((hdcd_state_stereo_t *)hdcd_decoder)->channel[1].sustain) {
				hdcdSustained = YES;
			}
			if(enableHDCD) {
				gain = 2.0;
				bitsPerSample = 32;
				bytesReadFromInput = samplesRead * 4;
				isUnsigned = NO;
				inputBuffer = &tempData[buffer_adder2];
				inputChanged = YES;
			} else {
				// Discard the output of the decoder and process again
				goto process16bit;
			}
		} else if(bitsPerSample <= 16) {
		process16bit:
			samplesRead = bytesReadFromInput / 2;
			const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			if(isUnsigned) {
				if(!inputChanged) {
					memcpy(&tempData[buffer_adder], inputBuffer, samplesRead * 2);
					inputBuffer = &tempData[buffer_adder];
					//inputChanged = YES;
				}
				convert_u16_to_s16((int16_t *)inputBuffer, samplesRead);
			}
			const size_t buffer_adder2 = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			vDSP_vflt16((const short *)inputBuffer, 1, (float *)(&tempData[buffer_adder2]), 1, samplesRead);
			float scale = 1ULL << 15;
			vDSP_vsdiv((const float *)(&tempData[buffer_adder2]), 1, &scale, (float *)(&tempData[buffer_adder2]), 1, samplesRead);
			bitsPerSample = 32;
			bytesReadFromInput = samplesRead * sizeof(float);
			isUnsigned = NO;
			isFloat = YES;
			inputBuffer = &tempData[buffer_adder2];
			inputChanged = YES;
		} else if(bitsPerSample <= 24) {
			const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0;
			samplesRead = bytesReadFromInput / 3;
			if(isUnsigned)
				convert_u24_to_s32((int32_t *)(&tempData[buffer_adder]), (uint8_t *)inputBuffer, samplesRead);
			else
				convert_s24_to_s32((int32_t *)(&tempData[buffer_adder]), (uint8_t *)inputBuffer, samplesRead);
			bitsPerSample = 32;
			bytesReadFromInput = samplesRead * 4;
			isUnsigned = NO;
			inputBuffer = &tempData[buffer_adder];
			inputChanged = YES;
		}
		if(!isFloat && bitsPerSample <= 32) {
			samplesRead = bytesReadFromInput / 4;
			if(isUnsigned) {
				if(!inputChanged) {
					memcpy(&tempData[0], inputBuffer, bytesReadFromInput);
					inputBuffer = &tempData[0];
				}
				convert_u32_to_s32((int32_t *)inputBuffer, samplesRead);
			}
			const size_t buffer_adder = (inputBuffer == &tempData[0]) ? buffer_adder_base : 0; // vDSP functions expect aligned to four elements
			vDSP_vflt32((const int *)inputBuffer, 1, (float *)(&tempData[buffer_adder]), 1, samplesRead);
			float scale = (1ULL << 31) / gain;
			vDSP_vsdiv((const float *)(&tempData[buffer_adder]), 1, &scale, (float *)(&tempData[buffer_adder]), 1, samplesRead);
			//bitsPerSample = 32;
			bytesReadFromInput = samplesRead * sizeof(float);
			//isUnsigned = NO;
			//isFloat = YES;
			inputBuffer = &tempData[buffer_adder];
		}

#ifdef _DEBUG
		[BadSampleCleaner cleanSamples:(float *)inputBuffer
								amount:bytesReadFromInput / sizeof(float)
							  location:@"post int to float conversion"];
#endif
	}

	AudioChunk *outChunk = [AudioChunk new];
	[outChunk setFormat:floatFormat];
	[outChunk setChannelConfig:inputChannelConfig];
	[outChunk setLossless:inputLossless];
	[outChunk setStreamTimestamp:streamTimestamp];
	[outChunk setStreamTimeRatio:[inChunk streamTimeRatio]];
	if(hdcdSustained) [outChunk setHDCD];
	if(inChunk.resetForward) {
		outChunk.resetForward = YES;
	}
	
	[outChunk assignSamples:inputBuffer frameCount:bytesReadFromInput / floatFormat.mBytesPerPacket];

	inConverter = NO;
	return outChunk;
}

- (BOOL)peekFormat:(AudioStreamBasicDescription *)format channelConfig:(uint32_t *)config {
	if(stopping) return NO;
	inPeeker = YES;
	@synchronized(chunkList) {
		if([chunkList count]) {
			AudioChunk *chunk = [chunkList objectAtIndex:0];
			*format = [chunk format];
			*config = [chunk channelConfig];
			inPeeker = NO;
			return YES;
		}
	}
	inPeeker = NO;
	return NO;
}

- (BOOL)peekTimestamp:(double *)timestamp timeRatio:(double *)timeRatio {
	if(stopping) return NO;
	inPeeker = YES;
	@synchronized (chunkList) {
		if([chunkList count]) {
			AudioChunk *chunk = [chunkList objectAtIndex:0];
			*timestamp = [chunk streamTimestamp];
			*timeRatio = [chunk streamTimeRatio];
			inPeeker = NO;
			return YES;
		}
	}
	*timestamp = 0.0;
	*timeRatio = 1.0;
	inPeeker = NO;
	return NO;
}

@end
