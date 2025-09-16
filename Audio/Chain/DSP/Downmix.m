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

#import <Accelerate/Accelerate.h>

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

	vDSP_vclr(outBuffer, 1, count * 2);

	float tempBuffer[count * 2];

	for(uint32_t i = 0; i < channels; ++i) {
		float leftRatio = 0.0F;
		float rightRatio = 0.0F;
		switch(channelIndexes[i]) {
			case 0:
				leftRatio = FrontRatios[0];
				rightRatio = FrontRatios[1];
				break;

			case 1:
				leftRatio = FrontRatios[1];
				rightRatio = FrontRatios[0];
				break;

			case 2:
				leftRatio = FrontCenterRatio;
				rightRatio = FrontCenterRatio;
				break;

			case 3:
				leftRatio = LFERatio;
				rightRatio = LFERatio;
				break;

			case 4:
				leftRatio = BackRatios[0];
				rightRatio = BackRatios[1];
				break;

			case 5:
				leftRatio = BackRatios[1];
				rightRatio = BackRatios[0];
				break;

			case 6:
			case 7:
				break;

			case 8:
				leftRatio = BackCenterRatio;
				rightRatio = BackCenterRatio;
				break;

			case 9:
				leftRatio = SideRatios[0];
				rightRatio = SideRatios[1];
				break;

			case 10:
				leftRatio = SideRatios[1];
				rightRatio = SideRatios[0];
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
		vDSP_vsmul(inBuffer + i, channels, &leftRatio, tempBuffer, 1, count);
		vDSP_vsmul(inBuffer + i, channels, &rightRatio, tempBuffer + count, 1, count);
		vDSP_vadd(outBuffer, 2, tempBuffer, 1, outBuffer, 2, count);
		vDSP_vadd(outBuffer + 1, 2, tempBuffer + count, 1, outBuffer + 1, 2, count);
	}
}

static void downmix_to_mono(const float *inBuffer, int channels, uint32_t config, float *outBuffer, size_t count) {
	float tempBuffer[count * 2];
	if(channels > 2 || config != AudioConfigStereo) {
		downmix_to_stereo(inBuffer, channels, config, tempBuffer, count);
		inBuffer = tempBuffer;
		// channels = 2;
		// config = AudioConfigStereo;
	}
	cblas_scopy((int)count, inBuffer, 2, outBuffer, 1);
	vDSP_vadd(outBuffer, 1, inBuffer + 1, 2, outBuffer, 1, count);
	const float scale = 0.5f;
	vDSP_vsmul(outBuffer, 1, &scale, outBuffer, 1, count);
}

static void upmix(const float *inBuffer, int inchannels, uint32_t inconfig, float *outBuffer, int outchannels, uint32_t outconfig, size_t count) {
	if(inconfig == AudioConfigMono && outconfig == AudioConfigStereo) {
		cblas_scopy((int)count, inBuffer, 1, outBuffer, 2);
		cblas_scopy((int)count, inBuffer, 1, outBuffer + 1, 2);
	} else if(inconfig == AudioConfigMono && outconfig == AudioConfig4Point0) {
		cblas_scopy((int)count, inBuffer, 1, outBuffer, 4);
		cblas_scopy((int)count, inBuffer, 1, outBuffer + 1, 4);
		vDSP_vclr(outBuffer + 2, 4, count);
		vDSP_vclr(outBuffer + 3, 4, count);
	} else if(inconfig == AudioConfigMono && (outconfig & AudioChannelFrontCenter)) {
		uint32_t cIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontCenter];
		cblas_scopy((int)count, inBuffer, 1, outBuffer + cIndex, outchannels);
		for(size_t i = 0; i < cIndex; ++i) {
			vDSP_vclr(outBuffer + i, outchannels, (int)count);
		}
		for(size_t i = cIndex + 1; i < outchannels; ++i) {
			vDSP_vclr(outBuffer + i, outchannels, (int)count);
		}
	} else if(inconfig == AudioConfig4Point0 && outchannels >= 5) {
		uint32_t flIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontLeft];
		uint32_t frIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontRight];
		uint32_t blIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackLeft];
		uint32_t brIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackRight];
		vDSP_vclr(outBuffer, 1, count * outchannels);
		if(flIndex != ~0)
			cblas_scopy((int)count, inBuffer + 0, 4, outBuffer + flIndex, outchannels);
		if(frIndex != ~0)
			cblas_scopy((int)count, inBuffer + 1, 4, outBuffer + frIndex, outchannels);
		if(blIndex != ~0)
			cblas_scopy((int)count, inBuffer + 2, 4, outBuffer + blIndex, outchannels);
		if(brIndex != ~0)
			cblas_scopy((int)count, inBuffer + 3, 4, outBuffer + brIndex, outchannels);
	} else if(inconfig == AudioConfig5Point0 && outchannels >= 6) {
		uint32_t flIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontLeft];
		uint32_t frIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontRight];
		uint32_t cIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelFrontCenter];
		uint32_t blIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackLeft];
		uint32_t brIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:AudioChannelBackRight];
		vDSP_vclr(outBuffer, 1, count * outchannels);
		if(flIndex != ~0)
			cblas_scopy((int)count, inBuffer + 0, 5, outBuffer + flIndex, outchannels);
		if(frIndex != ~0)
			cblas_scopy((int)count, inBuffer + 1, 5, outBuffer + frIndex, outchannels);
		if(cIndex != ~0)
			cblas_scopy((int)count, inBuffer + 2, 5, outBuffer + cIndex, outchannels);
		if(blIndex != ~0)
			cblas_scopy((int)count, inBuffer + 3, 5, outBuffer + blIndex, outchannels);
		if(brIndex != ~0)
			cblas_scopy((int)count, inBuffer + 4, 5, outBuffer + brIndex, outchannels);
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
		vDSP_vclr(outBuffer, 1, count * outchannels);
		if(flIndex != ~0)
			cblas_scopy((int)count, inBuffer + 0, 7, outBuffer + flIndex, outchannels);
		if(frIndex != ~0)
			cblas_scopy((int)count, inBuffer + 1, 7, outBuffer + frIndex, outchannels);
		if(cIndex != ~0)
			cblas_scopy((int)count, inBuffer + 2, 7, outBuffer + cIndex, outchannels);
		if(lfeIndex != ~0)
			cblas_scopy((int)count, inBuffer + 3, 7, outBuffer + lfeIndex, outchannels);
		if(slIndex != ~0)
			cblas_scopy((int)count, inBuffer + 4, 7, outBuffer + slIndex, outchannels);
		if(srIndex != ~0)
			cblas_scopy((int)count, inBuffer + 5, 7, outBuffer + srIndex, outchannels);
		if(bcIndex != ~0)
			cblas_scopy((int)count, inBuffer + 6, 7, outBuffer + bcIndex, outchannels);
		else {
			if(blIndex != ~0)
				cblas_scopy((int)count, inBuffer + 6, 7, outBuffer + blIndex, outchannels);
			if(brIndex != ~0)
				cblas_scopy((int)count, inBuffer + 6, 7, outBuffer + brIndex, outchannels);
		}
	} else {
		vDSP_vclr(outBuffer, 1, count * outchannels);
		for(int i = 0; i < inchannels; ++i) {
			uint32_t channelFlag = [AudioChunk extractChannelFlag:i fromConfig:inconfig];
			uint32_t outIndex = [AudioChunk channelIndexFromConfig:outconfig forFlag:channelFlag];
			if(outIndex != ~0)
				cblas_scopy((int)count, inBuffer + i, inchannels, outBuffer + outIndex, outchannels);
		}
	}
}

@implementation DownmixProcessor

static void *kDownmixProcessorContext = &kDownmixProcessorContext;

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
	}

	return self;
}

- (void)process:(const void *)inBuffer frameCount:(size_t)frames output:(void *)outBuffer {
	if(inputFormat.mChannelsPerFrame == 2 && outConfig == AudioConfigStereo &&
	   inConfig == (AudioChannelSideLeft | AudioChannelSideRight)) {
		// Workaround for HRTF output
		memcpy(outBuffer, inBuffer, frames * outputFormat.mBytesPerPacket);
	} else if(inputFormat.mChannelsPerFrame > 2 && outConfig == AudioConfigStereo) {
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
