//
//  HeadphoneFilter.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 1/24/22.
//

#import "HeadphoneFilter.h"
#import "AudioChunk.h"
#import "AudioDecoder.h"
#import "AudioSource.h"

#import <stdlib.h>

#import <fstream>

#import "rsstate.h"

#import "HrtfData.h"

#import "Logging.h"

typedef struct speakerPosition {
	float elevation;
	float azimuth;
	float distance;
} speakerPosition;

#define DEGREES(x) ((x)*M_PI / 180.0)

static const speakerPosition speakerPositions[18] = {
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(-30.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(+30.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(-135.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(+135.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(-15.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(+15.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(-180.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(-90.0), .distance = 1.0 },
	{ .elevation = DEGREES(0.0), .azimuth = DEGREES(+90.0), .distance = 1.0 },
	{ .elevation = DEGREES(+90.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(+45.0), .azimuth = DEGREES(-30.0), .distance = 1.0 },
	{ .elevation = DEGREES(+45.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(+45.0), .azimuth = DEGREES(+30.0), .distance = 1.0 },
	{ .elevation = DEGREES(+45.0), .azimuth = DEGREES(-135.0), .distance = 1.0 },
	{ .elevation = DEGREES(+45.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(+45.0), .azimuth = DEGREES(+135.0), .distance = 1.0 }
};

void getImpulse(NSURL *url, float **outImpulse, int *outSampleCount, int channelCount, uint32_t channelConfig) {
	BOOL impulseFound = NO;
	const float *impulseData = NULL;
	double sampleRateOfSource = 0;
	int sampleCount = 0;

	NSString *filePath = [url path];

	try {
		std::ifstream file([filePath UTF8String], std::fstream::binary);

		if(!file.is_open()) {
			throw std::logic_error("Cannot open file.");
		}

		HrtfData data(file);

		file.close();

		sampleRateOfSource = data.get_sample_rate();

		uint32_t sampleCountExact = data.get_response_length();
		sampleCount = sampleCountExact + ((data.get_longest_delay() + 2) >> 2);
		sampleCount = (sampleCount + 15) & ~15;

		*outImpulse = (float *)calloc(sizeof(float), sampleCount * channelCount * 2);
		if(!*outImpulse) {
			throw std::bad_alloc();
		}
		float *hrtfData = *outImpulse;

		for(uint32_t i = 0; i < channelCount; ++i) {
			uint32_t channelFlag = [AudioChunk extractChannelFlag:i fromConfig:channelConfig];
			uint32_t channelNumber = [AudioChunk findChannelIndex:channelFlag];

			if(channelNumber < 18) {
				const speakerPosition &speaker = speakerPositions[channelNumber];
				DirectionData hrtfLeft;
				DirectionData hrtfRight;

				data.get_direction_data(speaker.elevation, speaker.azimuth, speaker.distance, hrtfLeft, hrtfRight);

				cblas_scopy(sampleCountExact, &hrtfLeft.impulse_response[0], 1, &hrtfData[((hrtfLeft.delay + 2) >> 2) + sampleCount * i * 2], 1);
				cblas_scopy(sampleCountExact, &hrtfRight.impulse_response[0], 1, &hrtfData[((hrtfLeft.delay + 2) >> 2) + sampleCount * (i * 2 + 1)], 1);
			}
		}
		
		*outSampleCount = sampleCount;
	} catch(std::exception &e) {
		ALog(@"Exception caught: %s", e.what());
	}
}

@implementation HeadphoneFilter

+ (BOOL)validateImpulseFile:(NSURL *)url {
	NSString *filePath = [url path];

	try {
		std::ifstream file([filePath UTF8String], std::fstream::binary);

		if(!file.is_open()) {
			throw std::logic_error("Cannot open file.");
		}

		HrtfData data(file);

		file.close();

		return YES;
	} catch(std::exception &e) {
		ALog(@"Exception thrown: %s", e.what());
		return NO;
	}
}

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(int)channels withConfig:(uint32_t)config {
	self = [super init];

	if(self) {
		float *impulseBuffer = NULL;
		int sampleCount = 0;
		getImpulse(url, &impulseBuffer, &sampleCount, channels, config);
		if(!impulseBuffer) {
			return nil;
		}

		channelCount = channels;

		mirroredImpulseResponses = (float **)calloc(sizeof(float *), channelCount * 2);
		if(!mirroredImpulseResponses) {
			free(impulseBuffer);
			return nil;
		}

		for(int i = 0; i < channelCount * 2; ++i) {
			mirroredImpulseResponses[i] = &impulseBuffer[sampleCount * i];
			vDSP_vrvrs(mirroredImpulseResponses[i], 1, sampleCount);
		}

		paddedBufferSize = sampleCount;

		paddedSignal[0] = (float *)calloc(sizeof(float), paddedBufferSize * 2);
		if(!paddedSignal[0]) {
			return nil;
		}
		paddedSignal[1] = paddedSignal[0] + paddedBufferSize;

		prevInputs = (float **)calloc(channels, sizeof(float *));
		if(!prevInputs)
			return nil;
		prevInputs[0] = (float *)calloc(sizeof(float), sampleCount * channelCount);
		if(!prevInputs[0])
			return nil;
		for(int i = 1; i < channels; ++i) {
			prevInputs[i] = prevInputs[i - 1] + sampleCount;
		}
	}

	return self;
}

- (void)dealloc {
	if(paddedSignal[0]) {
		free(paddedSignal[0]);
	}

	if(prevInputs) {
		if(prevInputs[0]) {
			free(prevInputs[0]);
		}
		free(prevInputs);
	}

	if(mirroredImpulseResponses) {
		if(mirroredImpulseResponses[0]) {
			free(mirroredImpulseResponses[0]);
		}
		free(mirroredImpulseResponses);
	}
}

- (void)process:(const float *)inBuffer sampleCount:(int)count toBuffer:(float *)outBuffer {
	int sampleCount = paddedBufferSize;
	while(count > 0) {
		float left = 0, right = 0;
		for(int i = 0; i < channelCount; ++i) {
			float thisleft, thisright;
			vDSP_vmul(prevInputs[i], 1, mirroredImpulseResponses[i * 2], 1, paddedSignal[0], 1, sampleCount);
			vDSP_vmul(prevInputs[i], 1, mirroredImpulseResponses[i * 2 + 1], 1, paddedSignal[1], 1, sampleCount);
			vDSP_sve(paddedSignal[0], 1, &thisleft, sampleCount);
			vDSP_sve(paddedSignal[1], 1, &thisright, sampleCount);
			left += thisleft;
			right += thisright;

			memmove(prevInputs[i], prevInputs[i] + 1, sizeof(float) * (sampleCount - 1));
			prevInputs[i][sampleCount - 1] = *inBuffer++;
		}
		
		outBuffer[0] = left;
		outBuffer[1] = right;
		outBuffer += 2;
		--count;
	}
}

- (void)reset {
	for(int i = 0; i < channelCount; ++i) {
		vDSP_vclr(prevInputs[i], 1, paddedBufferSize);
	}
}

@end
