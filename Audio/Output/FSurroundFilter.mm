//
//  FSurroundFilter.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 7/9/22.
//

#import "FSurroundFilter.h"

#import "freesurround_decoder.h"

#import "AudioChunk.h"

#import <Accelerate/Accelerate.h>

#import <map>
#import <vector>

struct freesurround_params {
	// the user-configurable parameters
	float center_image, shift, depth, circular_wrap, focus, front_sep, rear_sep, bass_lo, bass_hi;
	bool use_lfe;
	channel_setup channels_fs; // FreeSurround channel setup
	std::vector<unsigned> chanmap; // FreeSurround -> WFX channel index translation (derived data for faster lookup)

	// construct with defaults
	freesurround_params()
	: center_image(0.7), shift(0), depth(1), circular_wrap(90), focus(0), front_sep(1), rear_sep(1),
	  bass_lo(40), bass_hi(90), use_lfe(false) {
		set_channels_fs(cs_5point1);
	}

	// compute the WFX version of the channel setup code
	unsigned channel_count() {
		return (unsigned)chanmap.size();
	}
	unsigned channels_wfx() {
		unsigned res = 0;
		for(unsigned i = 0; i < chanmap.size(); res |= chanmap[i++]) {};
		return res;
	}

	// assign a channel setup & recompute derived data
	void set_channels_fs(channel_setup setup) {
		channels_fs = setup;
		chanmap.clear();
		// Note: Because WFX does not define a few of the more exotic channels (side front left&right, side rear left&right, back center left&right),
		// the side front/back channel pairs (both left and right sides, resp.) are mapped here onto foobar's top front/back channel pairs and the
		// back (off-)center left/right channels are mapped onto foobar's top front center and top back center, respectively.
		// Therefore, these speakers should be connected to those outputs instead.
		std::map<channel_id, uint32_t> fs2wfx;
		fs2wfx[ci_front_left] = AudioChannelFrontLeft;
		fs2wfx[ci_front_center_left] = AudioChannelFrontCenterLeft;
		fs2wfx[ci_front_center] = AudioChannelFrontCenter;
		fs2wfx[ci_front_center_right] = AudioChannelFrontCenterRight;
		fs2wfx[ci_front_right] = AudioChannelFrontRight;
		fs2wfx[ci_side_front_left] = AudioChannelFrontLeft;
		fs2wfx[ci_side_front_right] = AudioChannelTopFrontRight;
		fs2wfx[ci_side_center_left] = AudioChannelSideLeft;
		fs2wfx[ci_side_center_right] = AudioChannelSideRight;
		fs2wfx[ci_side_back_left] = AudioChannelTopBackLeft;
		fs2wfx[ci_side_back_right] = AudioChannelTopBackRight;
		fs2wfx[ci_back_left] = AudioChannelBackLeft;
		fs2wfx[ci_back_center_left] = AudioChannelTopFrontCenter;
		fs2wfx[ci_back_center] = AudioChannelBackCenter;
		fs2wfx[ci_back_center_right] = AudioChannelTopBackCenter;
		fs2wfx[ci_back_right] = AudioChannelBackRight;
		fs2wfx[ci_lfe] = AudioChannelLFE;
		for(unsigned i = 0; i < freesurround_decoder::num_channels(channels_fs); i++)
			chanmap.push_back(fs2wfx[freesurround_decoder::channel_at(channels_fs, i)]);
	}
};

@implementation FSurroundFilter

- (id)initWithSampleRate:(double)srate {
	self = [super init];
	if(!self) return nil;

	self->srate = srate;

	freesurround_params *_params = new freesurround_params;
	params = (void *)_params;

	freesurround_decoder *_decoder = new freesurround_decoder(cs_5point1, 4096);
	decoder = (void *)_decoder;

	_decoder->circular_wrap(_params->circular_wrap);
	_decoder->shift(_params->shift);
	_decoder->depth(_params->depth);
	_decoder->focus(_params->focus);
	_decoder->center_image(_params->center_image);
	_decoder->front_separation(_params->front_sep);
	_decoder->rear_separation(_params->rear_sep);
	_decoder->bass_redirection(_params->use_lfe);
	_decoder->low_cutoff(_params->bass_lo / (srate / 2.0));
	_decoder->high_cutoff(_params->bass_hi / (srate / 2.0));

	channelCount = _params->channel_count();
	channelConfig = _params->channels_wfx();

	return self;
}

- (void)dealloc {
	if(decoder) {
		freesurround_decoder *_decoder = (freesurround_decoder *)decoder;
		delete _decoder;
	}
	if(params) {
		freesurround_params *_params = (freesurround_params *)params;
		delete _params;
	}
}

- (uint32_t)channelCount {
	return channelCount;
}

- (uint32_t)channelConfig {
	return channelConfig;
}

- (double)srate {
	return srate;
}

- (void)process:(const float *)samplesIn output:(float *)samplesOut count:(uint32_t)count {
	freesurround_params *_params = (freesurround_params *)params;
	freesurround_decoder *_decoder = (freesurround_decoder *)decoder;

	float tempInput[4096 * 2];
	uint32_t zeroCount = 0;

	if(count > 4096) {
		zeroCount = count - 4096;
		count = 4096;
	}

	if(count < 4096) {
		cblas_scopy(count * 2, samplesIn, 1, &tempInput[0], 1);
		vDSP_vclr(&tempInput[count * 2], 1, (4096 - count) * 2);
		samplesIn = &tempInput[0];
	}

	float *src = _decoder->decode(samplesIn);

	for(unsigned c = 0, num = channelCount; c < num; c++) {
		unsigned idx = [AudioChunk channelIndexFromConfig:channelConfig forFlag:_params->chanmap[c]];
		cblas_scopy(count, src + c, num, samplesOut + idx, num);
		if(zeroCount) {
			vDSP_vclr(samplesOut + idx + count, num, zeroCount);
		}
	}
}

@end
