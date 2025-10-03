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

#import <soxr.h>

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

static simd_float4x4 matX(float theta) {
	simd_float4x4 mat = {
		simd_make_float4(1.0f, 0.0f, 0.0f, 0.0f),
		simd_make_float4(0.0f, cosf(theta), -sinf(theta), 0.0f),
		simd_make_float4(0.0f, sinf(theta), cosf(theta), 0.0f),
		simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f)
	};
	return mat;
};

static simd_float4x4 matY(float theta) {
	simd_float4x4 mat = {
		simd_make_float4(cosf(theta), 0.0f, sinf(theta), 0.0f),
		simd_make_float4(0.0f, 1.0f, 0.0f, 0.0f),
		simd_make_float4(-sinf(theta), 0.0f, cosf(theta), 0.0f),
		simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f)
	};
	return mat;
}

#if 0
static simd_float4x4 matZ(float theta) {
	simd_float4x4 mat = {
		simd_make_float4(cosf(theta), -sinf(theta), 0.0f, 0.0f),
		simd_make_float4(sinf(theta), cosf(theta), 0.0f, 0.0f),
		simd_make_float4(0.0f, 0.0f, 1.0f, 0.0f),
		simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f)
	};
	return mat;
};
#endif

static void transformPosition(float &elevation, float &azimuth, const simd_float4x4 &matrix) {
	simd_float4x4 mat_x = matX(azimuth);
	simd_float4x4 mat_y = matY(elevation);
	//simd_float4x4 mat_z = matrix_identity_float4x4;
	simd_float4x4 offsetMatrix = simd_mul(mat_x, mat_y);
	//offsetMatrix = simd_mul(offsetMatrix, mat_z);
	offsetMatrix = simd_mul(offsetMatrix, matrix);

	double sy = sqrt(offsetMatrix.columns[0].x * offsetMatrix.columns[0].x + offsetMatrix.columns[1].x * offsetMatrix.columns[1].x);

	bool singular = sy < 1e-6; // If

	float x, y/*, z*/;
	if(!singular) {
		x = atan2(offsetMatrix.columns[2].y, offsetMatrix.columns[2].z);
		y = atan2(-offsetMatrix.columns[2].x, sy);
		//z = atan2(offsetMatrix.columns[1].x, offsetMatrix.columns[0].x);
	} else {
		x = atan2(-offsetMatrix.columns[1].z, offsetMatrix.columns[1].y);
		y = atan2(-offsetMatrix.columns[2].x, sy);
		//z = 0;
	}

	elevation = y;
	azimuth = x;

	if(elevation < (M_PI * (-0.5))) {
		elevation = (M_PI * (-0.5));
	} else if(elevation > M_PI * 0.5) {
		elevation = M_PI * 0.5;
	}
	while(azimuth < (M_PI * (-2.0))) {
		azimuth += M_PI * 2.0;
	}
	while(azimuth > M_PI * 2.0) {
		azimuth -= M_PI * 2.0;
	}
}

@interface impulseSetCache : NSObject {
	NSURL *URL;
	HrtfData *data;
}
+ (impulseSetCache *)sharedController;
- (void)getImpulse:(NSURL *)url outImpulse:(float **)outImpulse outSampleCount:(int *)outSampleCount sampleRate:(double)sampleRate channelCount:(int)channelCount channelConfig:(uint32_t)channelConfig withMatrix:(simd_float4x4)matrix;
@end

@implementation impulseSetCache
static impulseSetCache *_sharedController = nil;

+ (impulseSetCache *)sharedController {
	@synchronized(self) {
		if(!_sharedController) {
			_sharedController = [impulseSetCache new];
		}
	}
	return _sharedController;
}

- (id)init {
	self = [super init];
	if(self) {
		data = NULL;
	}
	return self;
}

- (void)dealloc {
	delete data;
}

- (void)getImpulse:(NSURL *)url outImpulse:(float **)outImpulse outSampleCount:(int *)outSampleCount sampleRate:(double)sampleRate channelCount:(int)channelCount channelConfig:(uint32_t)channelConfig withMatrix:(simd_float4x4)matrix {
	double sampleRateOfSource = 0;
	int sampleCount = 0;

	if(!data || ![url isEqualTo:URL]) {
		delete data;
		data = NULL;
		URL = url;
		NSString *filePath = [url path];
		try {
			std::ifstream file([filePath UTF8String], std::fstream::binary);
			if(!file.is_open()) {
				throw std::logic_error("Cannot open file.");
			}
			data = new HrtfData(file);
			file.close();
		} catch(std::exception &e) {
			ALog(@"Exception caught: %s", e.what());
		}
	}

	try {
		soxr_quality_spec_t q_spec = soxr_quality_spec(SOXR_HQ, 0);
		soxr_io_spec_t io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
		soxr_runtime_spec_t runtime_spec = soxr_runtime_spec(0);

		bool resampling;

		sampleRateOfSource = data->get_sample_rate();
		resampling = !!(fabs(sampleRateOfSource - sampleRate) > 1e-6);

		uint32_t sampleCountResampled;
		uint32_t sampleCountExact = data->get_response_length();
		sampleCount = sampleCountExact + ((data->get_longest_delay() + 2) >> 2);

		uint32_t actualSampleCount = sampleCount;
		if(resampling) {
			sampleCountResampled = (uint32_t)(((double)sampleCountExact) * sampleRate / sampleRateOfSource);
			actualSampleCount = (uint32_t)(((double)actualSampleCount) * sampleRate / sampleRateOfSource);
			io_spec.scale = sampleRateOfSource / sampleRate;
		}
		actualSampleCount = (actualSampleCount + 15) & ~15;

		*outImpulse = (float *)calloc(sizeof(float), actualSampleCount * channelCount * 2);
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

				float azimuth = speaker.azimuth;
				float elevation = speaker.elevation;

				transformPosition(elevation, azimuth, matrix);

				data->get_direction_data(elevation, azimuth, speaker.distance, hrtfLeft, hrtfRight);

				if(resampling) {
					ssize_t leftDelay = (ssize_t)((double)(hrtfLeft.delay) * 0.25 * sampleRate / sampleRateOfSource);
					ssize_t rightDelay = (ssize_t)((double)(hrtfRight.delay) * 0.25 * sampleRate / sampleRateOfSource);
					soxr_oneshot(sampleRateOfSource, sampleRate, 1, &hrtfLeft.impulse_response[0], sampleCountExact, NULL, &hrtfData[leftDelay + actualSampleCount * i * 2], sampleCountResampled, NULL, &io_spec, &q_spec, &runtime_spec);
					soxr_oneshot(sampleRateOfSource, sampleRate, 1, &hrtfRight.impulse_response[0], sampleCountExact, NULL, &hrtfData[rightDelay + actualSampleCount * (i * 2 + 1)], sampleCountResampled, NULL, &io_spec, &q_spec, &runtime_spec);
				} else {
					cblas_scopy(sampleCountExact, &hrtfLeft.impulse_response[0], 1, &hrtfData[((hrtfLeft.delay + 2) >> 2) + actualSampleCount * i * 2], 1);
					cblas_scopy(sampleCountExact, &hrtfRight.impulse_response[0], 1, &hrtfData[((hrtfRight.delay + 2) >> 2) + actualSampleCount * (i * 2 + 1)], 1);
				}
			}
		}

		*outSampleCount = actualSampleCount;
	} catch(std::exception &e) {
		ALog(@"Exception caught: %s", e.what());
	}
}
@end

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

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(int)channels withConfig:(uint32_t)config withMatrix:(simd_float4x4)matrix {
	self = [super init];

	if(self) {
		URL = url;
		self->sampleRate = sampleRate;
		channelCount = channels;
		self->config = config;

		float *impulseBuffer = NULL;
		int sampleCount = 0;
		[[impulseSetCache sharedController] getImpulse:url outImpulse:&impulseBuffer outSampleCount:&sampleCount sampleRate:sampleRate channelCount:channels channelConfig:config withMatrix:matrix];
		if(!impulseBuffer) {
			return nil;
		}

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

- (void)reloadWithMatrix:(simd_float4x4)matrix {
	@synchronized (self) {
		if(!mirroredImpulseResponses[0]) {
			return;
		}

		free(mirroredImpulseResponses[0]);

		float *impulseBuffer = NULL;
		int sampleCount = 0;
		[[impulseSetCache sharedController] getImpulse:URL outImpulse:&impulseBuffer outSampleCount:&sampleCount sampleRate:sampleRate channelCount:channelCount channelConfig:config withMatrix:matrix];

		for(int i = 0; i < channelCount * 2; ++i) {
			mirroredImpulseResponses[i] = &impulseBuffer[sampleCount * i];
			vDSP_vrvrs(mirroredImpulseResponses[i], 1, sampleCount);
		}
	}
}

- (void)process:(const float *)inBuffer sampleCount:(int)count toBuffer:(float *)outBuffer {
	@synchronized (self) {
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
}

- (void)reset {
	for(int i = 0; i < channelCount; ++i) {
		vDSP_vclr(prevInputs[i], 1, paddedBufferSize);
	}
}

- (size_t)needPrefill {
	return paddedBufferSize;
}

@end
