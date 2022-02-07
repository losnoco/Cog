//
//  Downmix.m
//  Cog
//
//  Created by Christopher Snowhill on 2/05/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import "Downmix.h"

#import "Logging.h"

#import "AudioChunk.h"

static void downmix_to_stereo(const float *inBuffer, int channels, uint32_t config, float *outBuffer, size_t count) {
	float FrontRatios[2] = { 0.0F, 0.0F };
	float FrontCenterRatio = 0.0F;
	float LFERatio = 0.0F;
	float BackRatios[2] = { 0.0F, 0.0F };
	float BackCenterRatio = 0.0F;
	float SideRatios[2] = { 0.0F, 0.0F };
	if(config & (AudioChannelFrontLeft | AudioChannelFrontRight)) {
		FrontRatios[0] = 1.0F;
	}
	if(config & AudioChannelFrontCenter) {
		FrontRatios[0] = 0.5858F;
		FrontCenterRatio = 0.4142F;
	}
	if(config & (AudioChannelBackLeft | AudioChannelBackRight)) {
		if(config & AudioChannelFrontCenter) {
			FrontRatios[0] = 0.651F;
			FrontCenterRatio = 0.46F;
			BackRatios[0] = 0.5636F;
			BackRatios[1] = 0.3254F;
		} else {
			FrontRatios[0] = 0.4226F;
			BackRatios[0] = 0.366F;
			BackRatios[1] = 0.2114F;
		}
	}
	if(config & AudioChannelLFE) {
		FrontRatios[0] *= 0.8F;
		FrontCenterRatio *= 0.8F;
		LFERatio = FrontCenterRatio;
		BackRatios[0] *= 0.8F;
		BackRatios[1] *= 0.8F;
	}
	if(config & AudioChannelBackCenter) {
		FrontRatios[0] *= 0.86F;
		FrontCenterRatio *= 0.86F;
		LFERatio *= 0.86F;
		BackRatios[0] *= 0.86F;
		BackRatios[1] *= 0.86F;
		BackCenterRatio = FrontCenterRatio * 0.86F;
	}
	if(config & (AudioChannelSideLeft | AudioChannelSideRight)) {
		float ratio = 0.73F;
		if(config & AudioChannelBackCenter) ratio = 0.85F;
		FrontRatios[0] *= ratio;
		FrontCenterRatio *= ratio;
		LFERatio *= ratio;
		BackRatios[0] *= ratio;
		BackRatios[1] *= ratio;
		BackCenterRatio *= ratio;
		SideRatios[0] = 0.463882352941176 * ratio;
		SideRatios[1] = 0.267882352941176 * ratio;
	}

	int32_t channelIndexes[channels];
	for(int i = 0; i < channels; ++i) {
		channelIndexes[i] = [AudioChunk findChannelIndex:[AudioChunk extractChannelFlag:i fromConfig:config]];
	}

	for(size_t i = 0; i < count; ++i) {
		float left = 0.0F, right = 0.0F;
		for(uint32_t j = 0; j < channels; ++j) {
			float inSample = inBuffer[i * channels + j];
			switch(channelIndexes[j]) {
				case 0:
					left += inSample * FrontRatios[0];
					right += inSample * FrontRatios[1];
					break;

				case 1:
					left += inSample * FrontRatios[1];
					right += inSample * FrontRatios[0];
					break;

				case 2:
					left += inSample * FrontCenterRatio;
					right += inSample * FrontCenterRatio;
					break;

				case 3:
					left += inSample * LFERatio;
					right += inSample * LFERatio;
					break;

				case 4:
					left += inSample * BackRatios[0];
					right += inSample * BackRatios[1];
					break;

				case 5:
					left += inSample * BackRatios[1];
					right += inSample * BackRatios[0];
					break;

				case 6:
				case 7:
					break;

				case 8:
					left += inSample * BackCenterRatio;
					right += inSample * BackCenterRatio;
					break;

				case 9:
					left += inSample * SideRatios[0];
					right += inSample * SideRatios[1];
					break;

				case 10:
					left += inSample * SideRatios[1];
					right += inSample * SideRatios[0];
					break;

				case 11:
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				default:
					break;
			}

			outBuffer[i * 2 + 0] = left;
			outBuffer[i * 2 + 1] = right;
		}
	}
}

static void downmix_to_mono(const float *inBuffer, int channels, uint32_t config, float *outBuffer, size_t count) {
	float tempBuffer[count * 2];
	downmix_to_stereo(inBuffer, channels, config, tempBuffer, count);
	inBuffer = tempBuffer;
	channels = 2;
	config = AudioConfigStereo;
	float invchannels = 1.0 / (float)channels;
	for(size_t i = 0; i < count; ++i) {
		float sample = 0;
		for(int j = 0; j < channels; ++j) {
			sample += inBuffer[i * channels + j];
		}
		outBuffer[i] = sample * invchannels;
	}
}

static void upmix(const float *inBuffer, int inchannels, uint32_t inconfig, float *outBuffer, int outchannels, uint32_t outconfig, size_t count) {
	if(inconfig == AudioConfigMono && outconfig == AudioConfigStereo) {
		for(size_t i = 0; i < count; ++i) {
			// upmix mono to stereo
			float sample = inBuffer[i];
			outBuffer[i * 2 + 0] = sample;
			outBuffer[i * 2 + 1] = sample;
		}
	} else if(inconfig == AudioConfigMono && outconfig == AudioConfig4Point0) {
		for(size_t i = 0; i < count; ++i) {
			// upmix mono to quad
			float sample = inBuffer[i];
			outBuffer[i * 4 + 0] = sample;
			outBuffer[i * 4 + 1] = sample;
			outBuffer[i * 4 + 2] = 0;
			outBuffer[i * 4 + 3] = 0;
		}
	} else if(inconfig == AudioConfigMono && (outconfig & AudioChannelFrontCenter)) {
		uint32_t cIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontCenter];
		for(size_t i = 0; i < count; ++i) {
			// upmix mono to center channel
			float sample = inBuffer[i];
			outBuffer[i * outchannels + cIndex] = sample;
			for(int j = 0; j < cIndex; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
			for(int j = cIndex + 1; j < outchannels; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
		}
	} else if(inconfig == AudioConfig4Point0 && outchannels >= 5) {
		uint32_t flIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontLeft];
		uint32_t frIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontRight];
		uint32_t blIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackLeft];
		uint32_t brIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackRight];
		for(size_t i = 0; i < count; ++i) {
			float fl = inBuffer[i * 4 + 0];
			float fr = inBuffer[i * 4 + 1];
			float bl = inBuffer[i * 4 + 2];
			float br = inBuffer[i * 4 + 3];
			memset(outBuffer + i * outchannels, 0, sizeof(float) * outchannels);
			if(flIndex != ~0) {
				outBuffer[i * outchannels + flIndex] = fl;
			}
			if(frIndex != ~0) {
				outBuffer[i * outchannels + frIndex] = fr;
			}
			if(blIndex != ~0) {
				outBuffer[i * outchannels + blIndex] = bl;
			}
			if(brIndex != ~0) {
				outBuffer[i * outchannels + brIndex] = br;
			}
		}
	} else if(inconfig == AudioConfig5Point0 && outchannels >= 6) {
		uint32_t flIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontLeft];
		uint32_t frIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontRight];
		uint32_t cIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontCenter];
		uint32_t blIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackLeft];
		uint32_t brIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackRight];
		for(size_t i = 0; i < count; ++i) {
			float fl = inBuffer[i * 5 + 0];
			float fr = inBuffer[i * 5 + 1];
			float c = inBuffer[i * 5 + 2];
			float bl = inBuffer[i * 5 + 3];
			float br = inBuffer[i * 5 + 4];
			memset(outBuffer + i * outchannels, 0, sizeof(float) * outchannels);
			if(flIndex != ~0) {
				outBuffer[i * outchannels + flIndex] = fl;
			}
			if(frIndex != ~0) {
				outBuffer[i * outchannels + frIndex] = fr;
			}
			if(cIndex != ~0) {
				outBuffer[i * outchannels + cIndex] = c;
			}
			if(blIndex != ~0) {
				outBuffer[i * outchannels + blIndex] = bl;
			}
			if(brIndex != ~0) {
				outBuffer[i * outchannels + brIndex] = br;
			}
		}
	} else if(inconfig == AudioConfig6Point1 && outchannels >= 8) {
		uint32_t flIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontLeft];
		uint32_t frIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontRight];
		uint32_t cIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontCenter];
		uint32_t lfeIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelLFE];
		uint32_t blIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackLeft];
		uint32_t brIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackRight];
		uint32_t bcIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackCenter];
		uint32_t slIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelSideLeft];
		uint32_t srIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelSideRight];
		for(size_t i = 0; i < count; ++i) {
			float fl = inBuffer[i * 7 + 0];
			float fr = inBuffer[i * 7 + 1];
			float c = inBuffer[i * 7 + 2];
			float lfe = inBuffer[i * 7 + 3];
			float sl = inBuffer[i * 7 + 4];
			float sr = inBuffer[i * 7 + 5];
			float bc = inBuffer[i * 7 + 6];
			memset(outBuffer + i * outchannels, 0, sizeof(float) * outchannels);
			if(flIndex != ~0) {
				outBuffer[i * outchannels + flIndex] = fl;
			}
			if(frIndex != ~0) {
				outBuffer[i * outchannels + frIndex] = fr;
			}
			if(cIndex != ~0) {
				outBuffer[i * outchannels + cIndex] = c;
			}
			if(lfeIndex != ~0) {
				outBuffer[i * outchannels + lfeIndex] = lfe;
			}
			if(slIndex != ~0) {
				outBuffer[i * outchannels + slIndex] = sl;
			}
			if(srIndex != ~0) {
				outBuffer[i * outchannels + srIndex] = sr;
			}
			if(bcIndex != ~0) {
				outBuffer[i * outchannels + bcIndex] = bc;
			} else {
				if(blIndex != ~0) {
					outBuffer[i * outchannels + blIndex] = bc;
				}
				if(brIndex != ~0) {
					outBuffer[i * outchannels + brIndex] = bc;
				}
			}
		}
	} else {
		uint32_t outIndexes[inchannels];
		for(int i = 0; i < inchannels; ++i) {
			uint32_t channelFlag = [AudioChunk extractChannelFlag:i fromConfig:inconfig];
			outIndexes[i] = [AudioChunk channelIndexFromConfig:outconfig forFlag:channelFlag];
		}
		for(size_t i = 0; i < count; ++i) {
			// upmix N channels to N channels plus silence the empty channels
			for(int j = 0; j < inchannels; ++j) {
				if(outIndexes[j] != ~0) {
					outBuffer[i * outchannels + outIndexes[j]] = inBuffer[i * inchannels + j];
				}
			}
		}
	}
}

@implementation DownmixProcessor

- (id)initWithInputFormat:(AudioStreamBasicDescription)inf inputConfig:(uint32_t)iConfig andOutputFormat:(AudioStreamBasicDescription)outf outputConfig:(uint32_t)oConfig {
	self = [super init];

	if(self) {
		if(inf.mFormatID != kAudioFormatLinearPCM ||
		   (inf.mFormatFlags & kAudioFormatFlagsNativeFloatPacked) != kAudioFormatFlagsNativeFloatPacked ||
		   inf.mBitsPerChannel != 32 ||
		   inf.mBytesPerFrame != (4 * inf.mChannelsPerFrame) ||
		   inf.mBytesPerPacket != inf.mFramesPerPacket * inf.mBytesPerFrame)
			return nil;

		if(outf.mFormatID != kAudioFormatLinearPCM ||
		   (outf.mFormatFlags & kAudioFormatFlagsNativeFloatPacked) != kAudioFormatFlagsNativeFloatPacked ||
		   outf.mBitsPerChannel != 32 ||
		   outf.mBytesPerFrame != (4 * outf.mChannelsPerFrame) ||
		   outf.mBytesPerPacket != outf.mFramesPerPacket * outf.mBytesPerFrame)
			return nil;

		inputFormat = inf;
		outputFormat = outf;

		inConfig = iConfig;
		outConfig = oConfig;

		[self setupVirt];

		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.headphoneVirtualization" options:0 context:nil];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hrirPath" options:0 context:nil];
	}

	return self;
}

- (void)dealloc {
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.headphoneVirtualization"];
	[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.hrirPath"];
}

- (void)setupVirt {
	@synchronized(hFilter) {
		hFilter = nil;
	}

	BOOL hVirt = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"headphoneVirtualization"];

	if(hVirt &&
	   outputFormat.mChannelsPerFrame == 2 &&
	   outConfig == AudioConfigStereo &&
	   inputFormat.mChannelsPerFrame >= 1 &&
	   (inConfig & (AudioConfig7Point1 | AudioChannelBackCenter)) != 0) {
		NSString *userPreset = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] stringForKey:@"hrirPath"];

		NSURL *presetUrl = nil;

		if(userPreset && ![userPreset isEqualToString:@""]) {
			presetUrl = [NSURL fileURLWithPath:userPreset];
			if(![HeadphoneFilter validateImpulseFile:presetUrl])
				presetUrl = nil;
		}

		if(!presetUrl) {
			presetUrl = [[NSBundle mainBundle] URLForResource:@"gsx" withExtension:@"wv"];
			if(![HeadphoneFilter validateImpulseFile:presetUrl])
				presetUrl = nil;
		}

		if(presetUrl) {
			@synchronized(hFilter) {
				hFilter = [[HeadphoneFilter alloc] initWithImpulseFile:presetUrl forSampleRate:outputFormat.mSampleRate withInputChannels:inputFormat.mChannelsPerFrame withConfig:inConfig];
			}
		}
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	DLog(@"SOMETHING CHANGED!");
	if([keyPath isEqualToString:@"values.headphoneVirtualization"] ||
	   [keyPath isEqualToString:@"values.hrirPath"]) {
		// Reset the converter, without rebuffering
		[self setupVirt];
	}
}

- (void)process:(const void *)inBuffer frameCount:(size_t)frames output:(void *)outBuffer {
	@synchronized(hFilter) {
		if(hFilter) {
			[hFilter process:(const float *)inBuffer sampleCount:frames toBuffer:(float *)outBuffer];
			return;
		}
	}

	if(inputFormat.mChannelsPerFrame > 2 && outConfig == AudioConfigStereo) {
		downmix_to_stereo((const float *)inBuffer, inputFormat.mChannelsPerFrame, inConfig, (float *)outBuffer, frames);
	} else if(inputFormat.mChannelsPerFrame > 1 && outConfig == AudioConfigMono) {
		downmix_to_mono((const float *)inBuffer, inputFormat.mChannelsPerFrame, inConfig, (float *)outBuffer, frames);
	} else if(inputFormat.mChannelsPerFrame < outputFormat.mChannelsPerFrame) {
		upmix((const float *)inBuffer, inputFormat.mChannelsPerFrame, inConfig, (float *)outBuffer, outputFormat.mChannelsPerFrame, outConfig, frames);
	} else if(inConfig == outConfig) {
		memcpy(outBuffer, inBuffer, frames * outputFormat.mBytesPerPacket);
	}
}

@end
