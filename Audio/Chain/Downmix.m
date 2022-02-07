//
//  Downmix.m
//  Cog
//
//  Created by Christopher Snowhill on 2/05/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import "Downmix.h"

#import "Logging.h"

static const float STEREO_DOWNMIX[8 - 2][8][2] = {
	/*3.0*/
	{
	{ 0.5858F, 0.0F },
	{ 0.0F, 0.5858F },
	{ 0.4142F, 0.4142F } },
	/*quadrophonic*/
	{
	{ 0.4226F, 0.0F },
	{ 0.0F, 0.4226F },
	{ 0.366F, 0.2114F },
	{ 0.2114F, 0.336F } },
	/*5.0*/
	{
	{ 0.651F, 0.0F },
	{ 0.0F, 0.651F },
	{ 0.46F, 0.46F },
	{ 0.5636F, 0.3254F },
	{ 0.3254F, 0.5636F } },
	/*5.1*/
	{
	{ 0.529F, 0.0F },
	{ 0.0F, 0.529F },
	{ 0.3741F, 0.3741F },
	{ 0.3741F, 0.3741F },
	{ 0.4582F, 0.2645F },
	{ 0.2645F, 0.4582F } },
	/*6.1*/
	{
	{ 0.4553F, 0.0F },
	{ 0.0F, 0.4553F },
	{ 0.322F, 0.322F },
	{ 0.322F, 0.322F },
	{ 0.2788F, 0.2788F },
	{ 0.3943F, 0.2277F },
	{ 0.2277F, 0.3943F } },
	/*7.1*/
	{
	{ 0.3886F, 0.0F },
	{ 0.0F, 0.3886F },
	{ 0.2748F, 0.2748F },
	{ 0.2748F, 0.2748F },
	{ 0.3366F, 0.1943F },
	{ 0.1943F, 0.3366F },
	{ 0.3366F, 0.1943F },
	{ 0.1943F, 0.3366F } }
};

static void downmix_to_stereo(const float *inBuffer, int channels, float *outBuffer, size_t count) {
	if(channels >= 3 && channels <= 8)
		for(size_t i = 0; i < count; ++i) {
			float left = 0, right = 0;
			for(int j = 0; j < channels; ++j) {
				left += inBuffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][0];
				right += inBuffer[i * channels + j] * STEREO_DOWNMIX[channels - 3][j][1];
			}
			outBuffer[i * 2 + 0] = left;
			outBuffer[i * 2 + 1] = right;
		}
}

static void downmix_to_mono(const float *inBuffer, int channels, float *outBuffer, size_t count) {
	float tempBuffer[count * 2];
	if(channels >= 3 && channels <= 8) {
		downmix_to_stereo(inBuffer, channels, tempBuffer, count);
		inBuffer = tempBuffer;
		channels = 2;
	}
	float invchannels = 1.0 / (float)channels;
	for(size_t i = 0; i < count; ++i) {
		float sample = 0;
		for(int j = 0; j < channels; ++j) {
			sample += inBuffer[i * channels + j];
		}
		outBuffer[i] = sample * invchannels;
	}
}

static void upmix(const float *inBuffer, int inchannels, float *outBuffer, int outchannels, size_t count) {
	for(ssize_t i = 0; i < count; ++i) {
		if(inchannels == 1 && outchannels == 2) {
			// upmix mono to stereo
			float sample = inBuffer[i];
			outBuffer[i * 2 + 0] = sample;
			outBuffer[i * 2 + 1] = sample;
		} else if(inchannels == 1 && outchannels == 4) {
			// upmix mono to quad
			float sample = inBuffer[i];
			outBuffer[i * 4 + 0] = sample;
			outBuffer[i * 4 + 1] = sample;
			outBuffer[i * 4 + 2] = 0;
			outBuffer[i * 4 + 3] = 0;
		} else if(inchannels == 1 && (outchannels == 3 || outchannels >= 5)) {
			// upmix mono to center channel
			float sample = inBuffer[i];
			outBuffer[i * outchannels + 2] = sample;
			for(int j = 0; j < 2; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
			for(int j = 3; j < outchannels; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
		} else if(inchannels == 4 && outchannels >= 5) {
			float fl = inBuffer[i * 4 + 0];
			float fr = inBuffer[i * 4 + 1];
			float bl = inBuffer[i * 4 + 2];
			float br = inBuffer[i * 4 + 3];
			const int skipclfe = (outchannels == 5) ? 1 : 2;
			outBuffer[i * outchannels + 0] = fl;
			outBuffer[i * outchannels + 1] = fr;
			outBuffer[i * outchannels + skipclfe + 2] = bl;
			outBuffer[i * outchannels + skipclfe + 3] = br;
			for(int j = 0; j < skipclfe; ++j) {
				outBuffer[i * outchannels + 2 + j] = 0;
			}
			for(int j = 4 + skipclfe; j < outchannels; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
		} else if(inchannels == 5 && outchannels >= 6) {
			float fl = inBuffer[i * 5 + 0];
			float fr = inBuffer[i * 5 + 1];
			float c = inBuffer[i * 5 + 2];
			float bl = inBuffer[i * 5 + 3];
			float br = inBuffer[i * 5 + 4];
			outBuffer[i * outchannels + 0] = fl;
			outBuffer[i * outchannels + 1] = fr;
			outBuffer[i * outchannels + 2] = c;
			outBuffer[i * outchannels + 3] = 0;
			outBuffer[i * outchannels + 4] = bl;
			outBuffer[i * outchannels + 5] = br;
			for(int j = 6; j < outchannels; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
		} else if(inchannels == 7 && outchannels == 8) {
			float fl = inBuffer[i * 7 + 0];
			float fr = inBuffer[i * 7 + 1];
			float c = inBuffer[i * 7 + 2];
			float lfe = inBuffer[i * 7 + 3];
			float sl = inBuffer[i * 7 + 4];
			float sr = inBuffer[i * 7 + 5];
			float bc = inBuffer[i * 7 + 6];
			outBuffer[i * 8 + 0] = fl;
			outBuffer[i * 8 + 1] = fr;
			outBuffer[i * 8 + 2] = c;
			outBuffer[i * 8 + 3] = lfe;
			outBuffer[i * 8 + 4] = bc;
			outBuffer[i * 8 + 5] = bc;
			outBuffer[i * 8 + 6] = sl;
			outBuffer[i * 8 + 7] = sr;
		} else {
			// upmix N channels to N channels plus silence the empty channels
			float samples[inchannels];
			for(int j = 0; j < inchannels; ++j) {
				samples[j] = inBuffer[i * inchannels + j];
			}
			for(int j = 0; j < inchannels; ++j) {
				outBuffer[i * outchannels + j] = samples[j];
			}
			for(int j = inchannels; j < outchannels; ++j) {
				outBuffer[i * outchannels + j] = 0;
			}
		}
	}
}

@implementation DownmixProcessor

- (id)initWithInputFormat:(AudioStreamBasicDescription)inf andOutputFormat:(AudioStreamBasicDescription)outf {
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
	   inputFormat.mChannelsPerFrame >= 1 &&
	   inputFormat.mChannelsPerFrame <= 8) {
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
				hFilter = [[HeadphoneFilter alloc] initWithImpulseFile:presetUrl forSampleRate:outputFormat.mSampleRate withInputChannels:inputFormat.mChannelsPerFrame];
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

	if(inputFormat.mChannelsPerFrame > 2 && outputFormat.mChannelsPerFrame == 2) {
		downmix_to_stereo((const float *)inBuffer, inputFormat.mChannelsPerFrame, (float *)outBuffer, frames);
	} else if(inputFormat.mChannelsPerFrame > 1 && outputFormat.mChannelsPerFrame == 1) {
		downmix_to_mono((const float *)inBuffer, inputFormat.mChannelsPerFrame, (float *)outBuffer, frames);
	} else if(inputFormat.mChannelsPerFrame < outputFormat.mChannelsPerFrame) {
		upmix((const float *)inBuffer, inputFormat.mChannelsPerFrame, (float *)outBuffer, outputFormat.mChannelsPerFrame, frames);
	} else if(inputFormat.mChannelsPerFrame == outputFormat.mChannelsPerFrame) {
		memcpy(outBuffer, inBuffer, frames * outputFormat.mBytesPerPacket);
	}
}

@end
