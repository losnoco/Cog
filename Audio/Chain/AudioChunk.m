//
//  AudioChunk.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import "AudioChunk.h"

#import "CoreAudioUtils.h"

@implementation AudioChunk

- (id)init {
	self = [super init];

	if(self) {
		chunkData = [NSMutableData new];
		formatAssigned = NO;
		lossless = NO;
		hdcd = NO;
		resetForward = NO;
		streamTimestamp = 0.0;
		streamTimeRatio = 1.0;
	}

	return self;
}

- (id)initWithProperties:(NSDictionary *)properties {
	self = [super init];

	if(self) {
		chunkData = [NSMutableData new];
		[self setFormat:propertiesToASBD(properties)];
		lossless = [[properties objectForKey:@"encoding"] isEqualToString:@"lossless"];
		hdcd = NO;
		resetForward = NO;
		streamTimestamp = 0.0;
		streamTimeRatio = 1.0;
	}

	return self;
}

- (AudioChunk *)copy {
	AudioChunk *outputChunk = [AudioChunk new];
	[outputChunk setFormat:format];
	[outputChunk setChannelConfig:channelConfig];
	if(hdcd) [outputChunk setHDCD];
	if(resetForward) outputChunk.resetForward = YES;
	[outputChunk setStreamTimestamp:streamTimestamp];
	[outputChunk setStreamTimeRatio:streamTimeRatio];
	[outputChunk assignData:chunkData];
	return outputChunk;
}

static const uint32_t AudioChannelConfigTable[] = {
	0,
	AudioConfigMono,
	AudioConfigStereo,
	AudioConfig3Point0,
	AudioConfig4Point0,
	AudioConfig5Point0,
	AudioConfig5Point1,
	AudioConfig6Point1,
	AudioConfig7Point1,
	0,
	AudioConfig7Point1 | AudioChannelFrontCenterLeft | AudioChannelFrontCenterRight
};

+ (uint32_t)guessChannelConfig:(uint32_t)channelCount {
	if(channelCount == 0) return 0;
	if(channelCount > 32) return 0;
	int ret = 0;
	if(channelCount < (sizeof(AudioChannelConfigTable) / sizeof(AudioChannelConfigTable[0])))
		ret = AudioChannelConfigTable[channelCount];
	if(!ret) {
		ret = (1 << channelCount) - 1;
	}
	assert([AudioChunk countChannels:ret] == channelCount);
	return ret;
}

+ (uint32_t)channelIndexFromConfig:(uint32_t)channelConfig forFlag:(uint32_t)flag {
	uint32_t index = 0;
	for(uint32_t walk = 0; walk < 32; ++walk) {
		uint32_t query = 1 << walk;
		if(flag & query) return index;
		if(channelConfig & query) ++index;
	}
	return ~0;
}

+ (uint32_t)extractChannelFlag:(uint32_t)index fromConfig:(uint32_t)channelConfig {
	uint32_t toskip = index;
	uint32_t flag = 1;
	while(flag) {
		if(channelConfig & flag) {
			if(toskip == 0) break;
			toskip--;
		}
		flag <<= 1;
	}
	return flag;
}

+ (uint32_t)countChannels:(uint32_t)channelConfig {
	return __builtin_popcount(channelConfig);
}

+ (uint32_t)findChannelIndex:(uint32_t)flag {
	uint32_t rv = 0;
	if((flag & 0xFFFF) == 0) {
		rv += 16;
		flag >>= 16;
	}
	if((flag & 0xFF) == 0) {
		rv += 8;
		flag >>= 8;
	}
	if((flag & 0xF) == 0) {
		rv += 4;
		flag >>= 4;
	}
	if((flag & 0x3) == 0) {
		rv += 2;
		flag >>= 2;
	}
	if((flag & 0x1) == 0) {
		rv += 1;
		flag >>= 1;
	}
	assert(flag & 1);
	return rv;
}

@synthesize lossless;
@synthesize resetForward;
@synthesize streamTimestamp;
@synthesize streamTimeRatio;

- (AudioStreamBasicDescription)format {
	return format;
}

- (void)setFormat:(AudioStreamBasicDescription)informat {
	formatAssigned = YES;
	format = informat;
	channelConfig = [AudioChunk guessChannelConfig:format.mChannelsPerFrame];
}

- (uint32_t)channelConfig {
	return channelConfig;
}

- (void)setChannelConfig:(uint32_t)config {
	if(formatAssigned) {
		if(config == 0) {
			config = [AudioChunk guessChannelConfig:format.mChannelsPerFrame];
		}
	}
	channelConfig = config;
}

- (void)assignSamples:(const void *_Nonnull)data frameCount:(size_t)count {
	if(formatAssigned) {
		const size_t bytesPerPacket = format.mBytesPerPacket;
		[chunkData appendBytes:data length:bytesPerPacket * count];
	}
}

- (void)assignData:(NSData *)data {
	[chunkData appendData:data];
}

- (NSData *)removeSamples:(size_t)frameCount {
	if(formatAssigned) {
		@autoreleasepool {
			const double secondsDuration = (double)(frameCount) / format.mSampleRate;
			const double DSDrate = (format.mBitsPerChannel == 1) ? 8.0 : 1.0;
			const size_t bytesPerPacket = format.mBytesPerPacket;
			const size_t byteCount = bytesPerPacket * frameCount;
			NSData *ret = [chunkData subdataWithRange:NSMakeRange(0, byteCount)];
			[chunkData replaceBytesInRange:NSMakeRange(0, byteCount) withBytes:NULL length:0];
			streamTimestamp += secondsDuration * streamTimeRatio * DSDrate;
			return ret;
		}
	}
	return [NSData data];
}

- (BOOL)isEmpty {
	return [chunkData length] == 0;
}

- (size_t)frameCount {
	if(formatAssigned) {
		const size_t bytesPerPacket = format.mBytesPerPacket;
		return [chunkData length] / bytesPerPacket;
	}
	return 0;
}

- (void)setFrameCount:(size_t)count {
	if(formatAssigned) {
		count *= format.mBytesPerPacket;
		size_t currentLength = [chunkData length];
		if(count < currentLength) {
			[chunkData replaceBytesInRange:NSMakeRange(count, currentLength - count) withBytes:NULL length:0];
		}
	}
}

- (double)duration {
	if(formatAssigned && [chunkData length]) {
		const size_t bytesPerPacket = format.mBytesPerPacket;
		const double sampleRate = format.mSampleRate;
		const double DSDrate = (format.mBitsPerChannel == 1) ? 8.0 : 1.0;
		return ((double)([chunkData length] / bytesPerPacket) / sampleRate) * DSDrate;
	}
	return 0.0;
}

- (double)durationRatioed {
	return [self duration] * streamTimeRatio;
}

- (BOOL)isHDCD {
	return hdcd;
}

- (void)setHDCD {
	hdcd = YES;
}

@end
