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

		inputBuffer = NULL;
		inputBufferSize = 0;

		stopping = NO;
		convertEntered = NO;
		paused = NO;

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling" options:0 context:kConverterNodeContext];
	}

	return self;
}

void scale_by_volume(float *buffer, size_t count, float volume) {
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

- (void)process {
	// Removed endOfStream check from here, since we want to be able to flush the converter
	// when the end of stream is reached. Convert function instead processes what it can,
	// and returns 0 samples when it has nothing more to process at the end of stream.
	while([self shouldContinue] == YES) {
		AudioChunk *chunk = nil;
		while(paused) {
			usleep(500);
		}
		@autoreleasepool {
			chunk = [self convert];
		}
		if(!chunk) {
			if(paused) {
				continue;
			} else if(!streamFormatChanged) {
				break;
			}
		} else {
			@autoreleasepool {
				[self writeChunk:chunk];
				chunk = nil;
			}
		}
		if(streamFormatChanged) {
			[self cleanUp];
			[self setupWithInputFormat:newInputFormat withInputConfig:newInputChannelConfig isLossless:rememberedLossless];
		}
	}
}

- (AudioChunk *)convert {
	UInt32 ioNumberPackets;

	if(stopping)
		return 0;

	convertEntered = YES;

	if(stopping || [self shouldContinue] == NO) {
		convertEntered = NO;
		return nil;
	}

	while(inpOffset == inpSize) {
		// Approximately the most we want on input
		ioNumberPackets = 4096;

		size_t newSize = ioNumberPackets * floatFormat.mBytesPerPacket;
		if(!inputBuffer || inputBufferSize < newSize)
			inputBuffer = realloc(inputBuffer, inputBufferSize = newSize);

		ssize_t amountToWrite = ioNumberPackets * floatFormat.mBytesPerPacket;

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

			AudioChunk *chunk = [self readChunkAsFloat32:((amountToWrite - bytesReadFromInput) / floatFormat.mBytesPerPacket)];
			inf = [chunk format];
			size_t frameCount = [chunk frameCount];
			config = [chunk channelConfig];
			size_t bytesRead = frameCount * inf.mBytesPerPacket;
			if(frameCount) {
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

		// Input now contains bytesReadFromInput worth of floats, in the input sample rate
		inpSize = bytesReadFromInput;
		inpOffset = 0;
	}

	ioNumberPackets = (UInt32)(inpSize - inpOffset);

	ioNumberPackets -= ioNumberPackets % floatFormat.mBytesPerPacket;

	if(ioNumberPackets) {
		AudioChunk *chunk = [[AudioChunk alloc] init];
		[chunk setFormat:nodeFormat];
		if(nodeChannelConfig) {
			[chunk setChannelConfig:nodeChannelConfig];
		}
		scale_by_volume((float *)(((uint8_t *)inputBuffer) + inpOffset), ioNumberPackets / sizeof(float), volumeScale);
		[chunk assignSamples:(((uint8_t *)inputBuffer) + inpOffset) frameCount:ioNumberPackets / floatFormat.mBytesPerPacket];

		inpOffset += ioNumberPackets;
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

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inf withInputConfig:(uint32_t)inputConfig isLossless:(BOOL)lossless {
	// Make the converter
	inputFormat = inf;

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

	// This is a post resampler, post-down/upmix format

	nodeFormat = floatFormat;
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
