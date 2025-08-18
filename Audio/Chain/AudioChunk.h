//
//  AudioChunk.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#ifndef AudioChunk_h
#define AudioChunk_h

#import <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

enum {
	AudioChannelFrontLeft = 1 << 0,
	AudioChannelFrontRight = 1 << 1,
	AudioChannelFrontCenter = 1 << 2,
	AudioChannelLFE = 1 << 3,
	AudioChannelBackLeft = 1 << 4,
	AudioChannelBackRight = 1 << 5,
	AudioChannelFrontCenterLeft = 1 << 6,
	AudioChannelFrontCenterRight = 1 << 7,
	AudioChannelBackCenter = 1 << 8,
	AudioChannelSideLeft = 1 << 9,
	AudioChannelSideRight = 1 << 10,
	AudioChannelTopCenter = 1 << 11,
	AudioChannelTopFrontLeft = 1 << 12,
	AudioChannelTopFrontCenter = 1 << 13,
	AudioChannelTopFrontRight = 1 << 14,
	AudioChannelTopBackLeft = 1 << 15,
	AudioChannelTopBackCenter = 1 << 16,
	AudioChannelTopBackRight = 1 << 17,

	AudioConfigMono = AudioChannelFrontCenter,
	AudioConfigStereo = AudioChannelFrontLeft | AudioChannelFrontRight,
	AudioConfig3Point0 = AudioChannelFrontLeft | AudioChannelFrontRight |
	                     AudioChannelFrontCenter,
	AudioConfig4Point0 = AudioChannelFrontLeft | AudioChannelFrontRight |
	                     AudioChannelBackLeft | AudioChannelBackRight,
	AudioConfig5Point0 = AudioChannelFrontLeft | AudioChannelFrontRight |
	                     AudioChannelFrontCenter | AudioChannelBackLeft |
	                     AudioChannelBackRight,
	AudioConfig5Point1 = AudioChannelFrontLeft | AudioChannelFrontRight |
	                     AudioChannelFrontCenter | AudioChannelLFE |
	                     AudioChannelBackLeft | AudioChannelBackRight,
	AudioConfig5Point1Side = AudioChannelFrontLeft | AudioChannelFrontRight |
	                         AudioChannelFrontCenter | AudioChannelLFE |
	                         AudioChannelSideLeft | AudioChannelSideRight,
	AudioConfig6Point1 = AudioChannelFrontLeft | AudioChannelFrontRight |
	                     AudioChannelFrontCenter | AudioChannelLFE |
	                     AudioChannelBackCenter | AudioChannelSideLeft |
	                     AudioChannelSideRight,
	AudioConfig7Point1 = AudioChannelFrontLeft | AudioChannelFrontRight |
	                     AudioChannelFrontCenter | AudioChannelLFE |
	                     AudioChannelBackLeft | AudioChannelBackRight |
	                     AudioChannelSideLeft | AudioChannelSideRight,

	AudioChannelsBackLeftRight = AudioChannelBackLeft | AudioChannelBackRight,
	AudioChannelsSideLeftRight = AudioChannelSideLeft | AudioChannelSideRight,
};

@interface AudioChunk : NSObject {
	AudioStreamBasicDescription format;
	NSMutableData *chunkData;
	uint32_t channelConfig;
	double streamTimestamp;
	double streamTimeRatio;
	BOOL formatAssigned;
	BOOL lossless;
	BOOL hdcd;
	BOOL resetForward;
}

@property AudioStreamBasicDescription format;
@property uint32_t channelConfig;
@property double streamTimestamp;
@property double streamTimeRatio;
@property BOOL lossless;
@property BOOL resetForward;

+ (uint32_t)guessChannelConfig:(uint32_t)channelCount;
+ (uint32_t)channelIndexFromConfig:(uint32_t)channelConfig forFlag:(uint32_t)flag;
+ (uint32_t)extractChannelFlag:(uint32_t)index fromConfig:(uint32_t)channelConfig;
+ (uint32_t)countChannels:(uint32_t)channelConfig;
+ (uint32_t)findChannelIndex:(uint32_t)flag;

- (id)init;
- (id)initWithProperties:(NSDictionary *)properties;

- (void)assignSamples:(const void *_Nonnull)data frameCount:(size_t)count;
- (void)assignData:(NSData *)data;

- (NSData *)removeSamples:(size_t)frameCount;

- (BOOL)isEmpty;

- (size_t)frameCount;
- (void)setFrameCount:(size_t)count; // For truncation only

- (double)duration;
- (double)durationRatioed;

- (BOOL)isHDCD;
- (void)setHDCD;

- (AudioChunk *)copy;

@end

NS_ASSUME_NONNULL_END

#endif /* AudioChunk_h */
