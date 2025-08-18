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

#import "lpc.h"
#import "util.h"

#ifdef _DEBUG
#import "BadSampleCleaner.h"
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

		soxr = 0;
		inputBuffer = NULL;
		inputBufferSize = 0;
		floatBuffer = NULL;
		floatBufferSize = 0;

		stopping = NO;
		convertEntered = NO;
		paused = NO;

		skipResampler = YES;

		extrapolateBuffer = NULL;
		extrapolateBufferSize = 0;

#ifdef LOG_CHAINS
		[self initLogFiles];
#endif
	}

	return self;
}

- (void)addObservers {
	if(!observersAdded) {
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling" options:(NSKeyValueObservingOptionInitial|NSKeyValueObservingOptionNew) context:kConverterNodeContext];
		observersAdded = YES;
	}
}

- (void)removeObservers {
	if(observersAdded) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeScaling" context:kConverterNodeContext];
		observersAdded = NO;
	}
}

void scale_by_volume(float *buffer, size_t count, float volume) {
	if(volume != 1.0) {
		size_t unaligned = (uintptr_t)buffer & 15;
		if(unaligned) {
			size_t count_unaligned = (16 - unaligned) / sizeof(float);
			while(count > 0 && count_unaligned > 0) {
				*buffer++ *= volume;
				count_unaligned--;
				count--;
			}
		}

		if(count) {
			vDSP_vsmul(buffer, 1, &volume, buffer, 1, count);
		}
	}
}

- (BOOL)paused {
	return paused;
}

- (void)process {
	// Removed endOfStream check from here, since we want to be able to flush the converter
	// when the end of stream is reached. Convert function instead processes what it can,
	// and returns 0 samples when it has nothing more to process at the end of stream.
	while([self shouldContinue] == YES) {
		while(paused) {
			usleep(500);
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk || ![chunk frameCount]) {
				if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
					endOfStream = YES;
					break;
				}
				if(paused || !streamFormatChanged) {
					continue;
				}
				usleep(500);
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
			if(streamFormatChanged) {
				[self cleanUp];
				[self setupWithInputFormat:newInputFormat withInputConfig:newInputChannelConfig outputFormat:self->outputFormat isLossless:rememberedLossless];
			}
		}
	}
	endOfStream = YES;
}

- (AudioChunk *)convert {
	UInt32 ioNumberPackets;

	if(stopping)
		return 0;

	convertEntered = YES;

	BOOL resetProcessed = NO;

	if(stopping || [self shouldContinue] == NO) {
		convertEntered = NO;
		return nil;
	}

	if(inpOffset == inpSize) {
		streamTimestamp = 0.0;
		streamTimeRatio = 1.0;
		if(![self peekTimestamp:&streamTimestamp timeRatio:&streamTimeRatio]) {
			convertEntered = NO;
			return nil;
		}
	}

	while(inpOffset == inpSize) {
		// Approximately the most we want on input
		ioNumberPackets = 4096;

		size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
		if(!inputBuffer || inputBufferSize < newSize)
			inputBuffer = realloc(inputBuffer, inputBufferSize = newSize);

		ssize_t amountToWrite = ioNumberPackets * floatFormat.mBytesPerPacket;

		ssize_t bytesReadFromInput = 0;

		while(bytesReadFromInput < amountToWrite && !stopping && !paused && !streamFormatChanged && [self shouldContinue] == YES && !([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES)) {
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

			AudioChunk *chunk = [self readChunkAsFloat32:((amountToWrite - bytesReadFromInput) / floatFormat.mBytesPerPacket)];
			inf = [chunk format];
			size_t frameCount = [chunk frameCount];
			config = [chunk channelConfig];
			size_t bytesRead = frameCount * inf.mBytesPerPacket;
			if(frameCount) {
				if(chunk.resetForward) {
					bytesReadFromInput = 0;
					resetProcessed = YES;
				}
				NSData *samples = [chunk removeSamples:frameCount];
				memcpy(((uint8_t *)inputBuffer) + bytesReadFromInput, [samples bytes], bytesRead);
				if([chunk isHDCD]) {
					[controller sustainHDCD];
				}
			}
			bytesReadFromInput += bytesRead;
			if(!frameCount) {
				usleep(500);
			}
		}

		if(!bytesReadFromInput) {
			convertEntered = NO;
			return nil;
		}

		if(stopping || paused || streamFormatChanged || [self shouldContinue] == NO || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES)) {
			if(!skipResampler) {
				if(!is_postextrapolated_) {
					is_postextrapolated_ = 1;
				}
			} else {
				is_postextrapolated_ = 3;
			}
		}

		// Extrapolate start
		if(!skipResampler && !is_preextrapolated_) {
			size_t inputSamples = bytesReadFromInput / floatFormat.mBytesPerPacket;
			size_t prime = MIN(inputSamples, PRIME_LEN_);
			size_t _N_samples_to_add_ = N_samples_to_add_;
			size_t newSize = _N_samples_to_add_ * floatFormat.mBytesPerPacket;
			newSize += bytesReadFromInput;

			if(newSize > inputBufferSize) {
				inputBuffer = realloc(inputBuffer, inputBufferSize = newSize * 3);
			}

			memmove(inputBuffer + _N_samples_to_add_ * floatFormat.mBytesPerPacket, inputBuffer, bytesReadFromInput);

			lpc_extrapolate_bkwd(inputBuffer + _N_samples_to_add_ * floatFormat.mBytesPerPacket, inputSamples, prime, floatFormat.mChannelsPerFrame, LPC_ORDER, _N_samples_to_add_, &extrapolateBuffer, &extrapolateBufferSize);

			bytesReadFromInput += _N_samples_to_add_ * floatFormat.mBytesPerPacket;
			latencyEaten = N_samples_to_drop_;
			is_preextrapolated_ = YES;
		}

		if(is_postextrapolated_ == 1) {
			size_t inputSamples = bytesReadFromInput / floatFormat.mBytesPerPacket;
			size_t prime = MIN(inputSamples, PRIME_LEN_);
			size_t _N_samples_to_add_ = N_samples_to_add_;

			size_t newSize = bytesReadFromInput;
			newSize += _N_samples_to_add_ * floatFormat.mBytesPerPacket;
			if(newSize > inputBufferSize) {
				inputBuffer = realloc(inputBuffer, inputBufferSize = newSize * 3);
			}

			lpc_extrapolate_fwd(inputBuffer, inputSamples, prime, floatFormat.mChannelsPerFrame, LPC_ORDER, _N_samples_to_add_, &extrapolateBuffer, &extrapolateBufferSize);

			bytesReadFromInput += _N_samples_to_add_ * floatFormat.mBytesPerPacket;
			latencyEatenPost = N_samples_to_drop_;
			is_postextrapolated_ = 2;
		} else if(is_postextrapolated_ == 3) {
			latencyEatenPost = 0;
		}

		// Input now contains bytesReadFromInput worth of floats, in the input sample rate
		inpSize = bytesReadFromInput;
		inpOffset = 0;
	}

	ioNumberPackets = (UInt32)(inpSize - inpOffset);

	ioNumberPackets -= ioNumberPackets % floatFormat.mBytesPerPacket;

	if(ioNumberPackets) {
		size_t inputSamples = ioNumberPackets / floatFormat.mBytesPerPacket;
		ioNumberPackets = (UInt32)inputSamples;
		ioNumberPackets = (UInt32)ceil((float)ioNumberPackets * sampleRatio);
		ioNumberPackets += soxr_delay(soxr);
		ioNumberPackets = (ioNumberPackets + 255) & ~255;

		size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
		if(!floatBuffer || floatBufferSize < newSize) {
			floatBuffer = realloc(floatBuffer, floatBufferSize = newSize * 3);
		}

		if(stopping) {
			convertEntered = NO;
			return nil;
		}

		size_t inputDone = 0;
		size_t outputDone = 0;

		if(!skipResampler) {
			soxr_process(soxr, (float *)(((uint8_t *)inputBuffer) + inpOffset), inputSamples, &inputDone, floatBuffer, ioNumberPackets, &outputDone);

			if(latencyEatenPost) {
				// Post file or format change flush
				size_t idone = 0, odone = 0;

				do {
					soxr_process(soxr, NULL, 0, &idone, floatBuffer + outputDone * floatFormat.mBytesPerPacket, ioNumberPackets - outputDone, &odone);
					outputDone += odone;
				} while(odone > 0);
			}
		} else {
			memcpy(floatBuffer, (((uint8_t *)inputBuffer) + inpOffset), inputSamples * floatFormat.mBytesPerPacket);
			inputDone = inputSamples;
			outputDone = inputSamples;
		}

		inpOffset += inputDone * floatFormat.mBytesPerPacket;

		if(latencyEaten) {
			if(outputDone > latencyEaten) {
				outputDone -= latencyEaten;
				memmove(floatBuffer, floatBuffer + latencyEaten * floatFormat.mBytesPerPacket, outputDone * floatFormat.mBytesPerPacket);
				latencyEaten = 0;
			} else {
				latencyEaten -= outputDone;
				outputDone = 0;
			}
		}

		if(latencyEatenPost) {
			if(outputDone > latencyEatenPost) {
				outputDone -= latencyEatenPost;
			} else {
				outputDone = 0;
			}
			latencyEatenPost = 0;
		}

		ioNumberPackets = (UInt32)outputDone * floatFormat.mBytesPerPacket;
	}

	if(ioNumberPackets) {
		AudioChunk *chunk = [[AudioChunk alloc] init];
		[chunk setFormat:nodeFormat];
		if(nodeChannelConfig) {
			[chunk setChannelConfig:nodeChannelConfig];
		}
		[self addObservers];
		scale_by_volume(floatBuffer, ioNumberPackets / sizeof(float), volumeScale);
		[chunk setStreamTimestamp:streamTimestamp];
		[chunk setStreamTimeRatio:streamTimeRatio];
		[chunk assignSamples:floatBuffer frameCount:ioNumberPackets / floatFormat.mBytesPerPacket];
		if(resetProcessed) chunk.resetForward = YES;
		streamTimestamp += [chunk durationRatioed];
		convertEntered = NO;
		return chunk;
	}

	convertEntered = NO;
	return nil;
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

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf withInputConfig:(uint32_t)inputConfig outputFormat:(AudioStreamBasicDescription)outf isLossless:(BOOL)lossless {
	// Make the converter
	inputFormat = inf;
	outputFormat = outf;

	inputChannelConfig = inputConfig;

	rememberedLossless = lossless;

	// These are the only sample formats we support translating
	BOOL isFloat = !!(inputFormat.mFormatFlags & kAudioFormatFlagIsFloat);
	if((!isFloat && !(inputFormat.mBitsPerChannel >= 1 && inputFormat.mBitsPerChannel <= 32)) || (isFloat && !(inputFormat.mBitsPerChannel == 32 || inputFormat.mBitsPerChannel == 64)))
		return NO;

	floatFormat = inputFormat;
	floatFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
	floatFormat.mBitsPerChannel = 32;
	floatFormat.mBytesPerFrame = (32 / 8) * floatFormat.mChannelsPerFrame;
	floatFormat.mBytesPerPacket = floatFormat.mBytesPerFrame * floatFormat.mFramesPerPacket;

#if DSD_DECIMATE
	if(inputFormat.mBitsPerChannel == 1) {
		// Decimate this for speed
		floatFormat.mSampleRate *= 1.0 / 8.0;
	}
#endif

	inpOffset = 0;
	inpSize = 0;

	// This is a post resampler format

	nodeFormat = floatFormat;
	nodeFormat.mSampleRate = outputFormat.mSampleRate;
	nodeChannelConfig = inputChannelConfig;

	sampleRatio = (double)outputFormat.mSampleRate / (double)floatFormat.mSampleRate;
	skipResampler = fabs(sampleRatio - 1.0) < 1e-7;
	if(!skipResampler) {
		soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_HQ, 0);
		soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
		soxr_runtime_spec_t runtime_spec = soxr_runtime_spec(0);
		soxr_error_t error;

		soxr = soxr_create(floatFormat.mSampleRate, outputFormat.mSampleRate, floatFormat.mChannelsPerFrame, &error, &io_spec, &q_spec, &runtime_spec);
		if(error)
			return NO;

		PRIME_LEN_ = MAX(floatFormat.mSampleRate / 20, 1024u);
		PRIME_LEN_ = MIN(PRIME_LEN_, 16384u);
		PRIME_LEN_ = MAX(PRIME_LEN_, (unsigned int)(2 * LPC_ORDER + 1));

		N_samples_to_add_ = floatFormat.mSampleRate;
		N_samples_to_drop_ = outputFormat.mSampleRate;

		samples_len(&N_samples_to_add_, &N_samples_to_drop_, 20, 8192u);

		is_preextrapolated_ = NO;
		is_postextrapolated_ = 0;
	}

	latencyEaten = 0;
	latencyEatenPost = 0;

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
	DLog(@"Converter dealloc");

	[self removeObservers];

	paused = NO;
	[self cleanUp];
	[super cleanUp];
}

- (void)setOutputFormat:(AudioStreamBasicDescription)format {
	DLog(@"SETTING OUTPUT FORMAT!");
	outputFormat = format;
}

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format inputConfig:(uint32_t)inputConfig {
	DLog(@"FORMAT CHANGED");
	paused = YES;
	while(convertEntered) {
		usleep(500);
	}
	[self cleanUp];
	[self setupWithInputFormat:format withInputConfig:inputConfig outputFormat:self->outputFormat isLossless:rememberedLossless];
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
	if(soxr) {
		soxr_delete(soxr);
		soxr = NULL;
	}
	if(extrapolateBuffer) {
		free(extrapolateBuffer);
		extrapolateBuffer = NULL;
		extrapolateBufferSize = 0;
	}
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
	inpOffset = 0;
	inpSize = 0;
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

@end
