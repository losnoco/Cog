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

#import "r8bstate.h"

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
	{ .elevation = DEGREES(-90.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(-45.0), .azimuth = DEGREES(-30.0), .distance = 1.0 },
	{ .elevation = DEGREES(-45.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(-45.0), .azimuth = DEGREES(+30.0), .distance = 1.0 },
	{ .elevation = DEGREES(-45.0), .azimuth = DEGREES(-135.0), .distance = 1.0 },
	{ .elevation = DEGREES(-45.0), .azimuth = DEGREES(0.0), .distance = 1.0 },
	{ .elevation = DEGREES(-45.0), .azimuth = DEGREES(+135.0), .distance = 1.0 }
};

@interface impulseCacheObject : NSObject {
}
@property NSURL *URL;
@property int sampleCount;
@property int channelCount;
@property uint32_t channelConfig;
@property double sampleRate;
@property double targetSampleRate;
@property NSData *data;
@end

@implementation impulseCacheObject
@synthesize URL;
@synthesize sampleCount;
@synthesize channelCount;
@synthesize channelConfig;
@synthesize sampleRate;
@synthesize targetSampleRate;
@synthesize data;
@end

@interface impulseCache : NSObject {
}
@property NSMutableArray<impulseCacheObject *> *cacheObjects;
+ (impulseCache *)sharedController;
- (const float *)getImpulse:(NSURL *)url sampleCount:(int *)sampleCount channelCount:(int)channelCount channelConfig:(uint32_t)channelConfig sampleRate:(double)sampleRate;
@end

// Apparently _mm_malloc is Intel-only on newer macOS targets, so use supported posix_memalign
static void *_memalign_malloc(size_t size, size_t align) {
	void *ret = NULL;
	if(posix_memalign(&ret, align, size) != 0) {
		return NULL;
	}
	return ret;
}

@implementation impulseCache

static impulseCache *_sharedController = nil;

+ (impulseCache *)sharedController {
	@synchronized(self) {
		if(!_sharedController) {
			_sharedController = [[impulseCache alloc] init];
		}
	}
	return _sharedController;
}

- (id)init {
	self = [super init];
	if(self) {
		self.cacheObjects = [[NSMutableArray alloc] init];
	}
	return self;
}

- (impulseCacheObject *)addImpulse:(NSURL *)url sampleCount:(int)sampleCount channelCount:(int)channelCount channelConfig:(uint32_t)channelConfig originalSampleRate:(double)originalSampleRate targetSampleRate:(double)targetSampleRate impulseBuffer:(const float *)impulseBuffer {
	impulseCacheObject *obj = [[impulseCacheObject alloc] init];

	obj.URL = url;
	obj.sampleCount = sampleCount;
	obj.channelCount = channelCount;
	obj.sampleRate = originalSampleRate;
	obj.targetSampleRate = targetSampleRate;
	obj.data = [NSData dataWithBytes:impulseBuffer length:(sampleCount * channelCount * sizeof(float) * 2)];

	@synchronized(self.cacheObjects) {
		[self.cacheObjects addObject:obj];
	}

	return obj;
}

- (const float *)getImpulse:(NSURL *)url sampleCount:(int *)retSampleCount channelCount:(int)channelCount channelConfig:(uint32_t)channelConfig sampleRate:(double)sampleRate {
	BOOL impulseFound = NO;
	const float *impulseData = NULL;
	double sampleRateOfSource = 0;
	int sampleCount = 0;
	impulseCacheObject *cacheObject = nil;

	@synchronized(self.cacheObjects) {
		for(impulseCacheObject *obj in self.cacheObjects) {
			if([obj.URL isEqualTo:url] &&
			   obj.targetSampleRate == sampleRate &&
			   obj.channelCount == channelCount &&
			   obj.channelConfig == channelConfig) {
				*retSampleCount = obj.sampleCount;
				return (const float *)[obj.data bytes];
			}
		}
		for(impulseCacheObject *obj in self.cacheObjects) {
			if([obj.URL isEqualTo:url] &&
			   obj.sampleRate == obj.targetSampleRate &&
			   obj.channelCount == channelCount &&
			   obj.channelConfig == channelConfig) {
				impulseData = (const float *)[obj.data bytes];
				sampleCount = obj.sampleCount;
				sampleRateOfSource = obj.sampleRate;
				impulseFound = YES;
				break;
			}
		}
	}

	if(!impulseFound) {
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

			std::vector<float> hrtfData(sampleCount * channelCount * 2, 0.0);

			for(uint32_t i = 0; i < channelCount; ++i) {
				uint32_t channelFlag = [AudioChunk extractChannelFlag:i fromConfig:channelConfig];
				uint32_t channelNumber = [AudioChunk findChannelIndex:channelFlag];

				if(channelNumber < 18) {
					const speakerPosition &speaker = speakerPositions[channelNumber];
					DirectionData hrtfLeft;
					DirectionData hrtfRight;

					data.get_direction_data(speaker.elevation, speaker.azimuth, speaker.distance, hrtfLeft, hrtfRight);

					cblas_scopy(sampleCountExact, &hrtfLeft.impulse_response[0], 1, &hrtfData[((hrtfLeft.delay + 2) >> 2) * channelCount * 2 + i * 2], channelCount * 2);
					cblas_scopy(sampleCountExact, &hrtfRight.impulse_response[0], 1, &hrtfData[((hrtfLeft.delay + 2) >> 2) * channelCount * 2 + i * 2 + 1], channelCount * 2);
				}
			}

			cacheObject = [self addImpulse:url sampleCount:sampleCount channelCount:channelCount channelConfig:channelConfig originalSampleRate:sampleRateOfSource targetSampleRate:sampleRateOfSource impulseBuffer:&hrtfData[0]];

			impulseData = (const float *)[cacheObject.data bytes];
		} catch(std::exception &e) {
			ALog(@"Exception caught: %s", e.what());
			return nil;
		}
	}

	if(sampleRateOfSource != sampleRate) {
		double sampleRatio = sampleRate / sampleRateOfSource;
		int resampledCount = (int)ceil((double)sampleCount * sampleRatio);

		void *r8bstate = r8bstate_new(channelCount * 2, 1024, sampleRateOfSource, sampleRate);

		float *resampledImpulse = (float *)_memalign_malloc(resampledCount * sizeof(float) * channelCount * 2, 16);
		if(!resampledImpulse) {
			r8bstate_delete(r8bstate);
			return nil;
		}

		size_t inputDone = 0;
		size_t outputDone = 0;

		outputDone = r8bstate_resample(r8bstate, impulseData, sampleCount, &inputDone, resampledImpulse, resampledCount);

		while(outputDone < resampledCount) {
			outputDone += r8bstate_flush(r8bstate, resampledImpulse + outputDone * channelCount * 2, resampledCount - outputDone);
		}

		r8bstate_delete(r8bstate);

		sampleCount = (int)outputDone;

		// Normalize resampled impulse by sample ratio
		float fSampleRatio = (float)sampleRatio;
		vDSP_vsdiv(resampledImpulse, 1, &fSampleRatio, resampledImpulse, 1, sampleCount * channelCount * 2);

		cacheObject = [self addImpulse:url sampleCount:sampleCount channelCount:channelCount channelConfig:channelConfig originalSampleRate:sampleRateOfSource targetSampleRate:sampleRate impulseBuffer:resampledImpulse];

		free(resampledImpulse);

		impulseData = (const float *)[cacheObject.data bytes];
	}

	*retSampleCount = sampleCount;
	return impulseData;
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

- (id)initWithImpulseFile:(NSURL *)url forSampleRate:(double)sampleRate withInputChannels:(int)channels withConfig:(uint32_t)config {
	self = [super init];

	if(self) {
		int sampleCount = 0;
		const float *impulseBuffer = [[impulseCache sharedController] getImpulse:url sampleCount:&sampleCount channelCount:channels channelConfig:config sampleRate:sampleRate];
		if(!impulseBuffer) {
			return nil;
		}

		channelCount = channels;

		bufferSize = 512;
		fftSize = sampleCount + bufferSize;

		int pow = 1;
		while(fftSize > 2) {
			pow++;
			fftSize /= 2;
		}
		fftSize = 2 << pow;

		float *deinterleavedImpulseBuffer = (float *)_memalign_malloc(fftSize * sizeof(float) * channelCount * 2, 16);
		if(!deinterleavedImpulseBuffer) {
			return nil;
		}

		for(int i = 0; i < channelCount; ++i) {
			cblas_scopy(sampleCount, impulseBuffer + i * 2, (int)channelCount * 2, deinterleavedImpulseBuffer + i * fftSize * 2, 1);
			vDSP_vclr(deinterleavedImpulseBuffer + i * fftSize * 2 + sampleCount, 1, fftSize - sampleCount);
			cblas_scopy(sampleCount, impulseBuffer + i * 2 + 1, (int)channelCount * 2, deinterleavedImpulseBuffer + i * fftSize * 2 + fftSize, 1);
			vDSP_vclr(deinterleavedImpulseBuffer + i * fftSize * 2 + fftSize + sampleCount, 1, fftSize - sampleCount);
		}

		paddedBufferSize = fftSize;
		fftSizeOver2 = (fftSize + 1) / 2;
		const size_t fftSizeOver2Plus1 = fftSizeOver2 + 1; // DFT float overwrites plus one, double doesn't

		dftSetupF = vDSP_DFT_zrop_CreateSetup(nil, fftSize, vDSP_DFT_FORWARD);
		dftSetupB = vDSP_DFT_zrop_CreateSetup(nil, fftSize, vDSP_DFT_INVERSE);
		if(!dftSetupF || !dftSetupB) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		paddedSignal = (float *)_memalign_malloc(sizeof(float) * paddedBufferSize, 16);
		if(!paddedSignal) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		signal_fft.realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		signal_fft.imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		if(!signal_fft.realp || !signal_fft.imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_per_channel[0].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		input_filtered_signal_per_channel[0].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		if(!input_filtered_signal_per_channel[0].realp ||
		   !input_filtered_signal_per_channel[0].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_per_channel[1].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		input_filtered_signal_per_channel[1].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		if(!input_filtered_signal_per_channel[1].realp ||
		   !input_filtered_signal_per_channel[1].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_totals[0].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		input_filtered_signal_totals[0].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		if(!input_filtered_signal_totals[0].realp ||
		   !input_filtered_signal_totals[0].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		input_filtered_signal_totals[1].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		input_filtered_signal_totals[1].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
		if(!input_filtered_signal_totals[1].realp ||
		   !input_filtered_signal_totals[1].imagp) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		impulse_responses = (DSPSplitComplex *)calloc(sizeof(DSPSplitComplex), channels * 2);
		if(!impulse_responses) {
			free(deinterleavedImpulseBuffer);
			return nil;
		}

		for(int i = 0; i < channels; ++i) {
			impulse_responses[i * 2 + 0].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
			impulse_responses[i * 2 + 0].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
			impulse_responses[i * 2 + 1].realp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);
			impulse_responses[i * 2 + 1].imagp = (float *)_memalign_malloc(sizeof(float) * fftSizeOver2Plus1, 16);

			if(!impulse_responses[i * 2 + 0].realp || !impulse_responses[i * 2 + 0].imagp ||
			   !impulse_responses[i * 2 + 1].realp || !impulse_responses[i * 2 + 1].imagp) {
				free(deinterleavedImpulseBuffer);
				return nil;
			}

			vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + i * fftSize * 2), 2, &impulse_responses[i * 2 + 0], 1, fftSizeOver2);
			vDSP_ctoz((DSPComplex *)(deinterleavedImpulseBuffer + i * fftSize * 2 + fftSize), 2, &impulse_responses[i * 2 + 1], 1, fftSizeOver2);

			vDSP_DFT_Execute(dftSetupF, impulse_responses[i * 2 + 0].realp, impulse_responses[i * 2 + 0].imagp, impulse_responses[i * 2 + 0].realp, impulse_responses[i * 2 + 0].imagp);
			vDSP_DFT_Execute(dftSetupF, impulse_responses[i * 2 + 1].realp, impulse_responses[i * 2 + 1].imagp, impulse_responses[i * 2 + 1].realp, impulse_responses[i * 2 + 1].imagp);
		}

		free(deinterleavedImpulseBuffer);

		left_result = (float *)_memalign_malloc(sizeof(float) * fftSize, 16);
		right_result = (float *)_memalign_malloc(sizeof(float) * fftSize, 16);
		if(!left_result || !right_result)
			return nil;

		prevInputs = (float **)calloc(channels, sizeof(float *));
		if(!prevInputs)
			return nil;
		for(int i = 0; i < channels; ++i) {
			prevInputs[i] = (float *)_memalign_malloc(sizeof(float) * fftSize, 16);
			if(!prevInputs[i])
				return nil;
			vDSP_vclr(prevInputs[i], 1, fftSize);
		}
	}

	return self;
}

- (void)dealloc {
	if(dftSetupF) vDSP_DFT_DestroySetup(dftSetupF);
	if(dftSetupB) vDSP_DFT_DestroySetup(dftSetupB);

	free(paddedSignal);

	free(signal_fft.realp);
	free(signal_fft.imagp);

	free(input_filtered_signal_per_channel[0].realp);
	free(input_filtered_signal_per_channel[0].imagp);
	free(input_filtered_signal_per_channel[1].realp);
	free(input_filtered_signal_per_channel[1].imagp);

	free(input_filtered_signal_totals[0].realp);
	free(input_filtered_signal_totals[0].imagp);
	free(input_filtered_signal_totals[1].realp);
	free(input_filtered_signal_totals[1].imagp);

	if(impulse_responses) {
		for(int i = 0; i < channelCount * 2; ++i) {
			free(impulse_responses[i].realp);
			free(impulse_responses[i].imagp);
		}
		free(impulse_responses);
	}

	free(left_result);
	free(right_result);

	if(prevInputs) {
		for(int i = 0; i < channelCount; ++i) {
			free(prevInputs[i]);
		}
		free(prevInputs);
	}
}

- (void)process:(const float *)inBuffer sampleCount:(int)count toBuffer:(float *)outBuffer {
	const float scale = 1.0 / (4.0 * (float)fftSize);

	while(count > 0) {
		const int countToDo = (count > bufferSize) ? bufferSize : count;
		const int prevToDo = fftSize - countToDo;

		vDSP_vclr(input_filtered_signal_totals[0].realp, 1, fftSizeOver2);
		vDSP_vclr(input_filtered_signal_totals[0].imagp, 1, fftSizeOver2);
		vDSP_vclr(input_filtered_signal_totals[1].realp, 1, fftSizeOver2);
		vDSP_vclr(input_filtered_signal_totals[1].imagp, 1, fftSizeOver2);

		for(int i = 0; i < channelCount; ++i) {
			cblas_scopy((int)prevToDo, prevInputs[i] + countToDo, 1, paddedSignal, 1);
			cblas_scopy((int)countToDo, inBuffer + i, (int)channelCount, paddedSignal + prevToDo, 1);
			cblas_scopy((int)fftSize, paddedSignal, 1, prevInputs[i], 1);

			vDSP_ctoz((DSPComplex *)paddedSignal, 2, &signal_fft, 1, fftSizeOver2);

			vDSP_DFT_Execute(dftSetupF, signal_fft.realp, signal_fft.imagp, signal_fft.realp, signal_fft.imagp);

			// One channel forward, then multiply and back twice

			float preserveIRNyq = impulse_responses[i * 2 + 0].imagp[0];
			float preserveSigNyq = signal_fft.imagp[0];
			impulse_responses[i * 2 + 0].imagp[0] = 0;
			signal_fft.imagp[0] = 0;

			vDSP_zvmul(&signal_fft, 1, &impulse_responses[i * 2 + 0], 1, &input_filtered_signal_per_channel[0], 1, fftSizeOver2, 1);

			input_filtered_signal_per_channel[0].imagp[0] = preserveIRNyq * preserveSigNyq;
			impulse_responses[i * 2 + 0].imagp[0] = preserveIRNyq;

			preserveIRNyq = impulse_responses[i * 2 + 1].imagp[0];
			impulse_responses[i * 2 + 1].imagp[0] = 0;

			vDSP_zvmul(&signal_fft, 1, &impulse_responses[i * 2 + 1], 1, &input_filtered_signal_per_channel[1], 1, fftSizeOver2, 1);

			input_filtered_signal_per_channel[1].imagp[0] = preserveIRNyq * preserveSigNyq;
			impulse_responses[i * 2 + 1].imagp[0] = preserveIRNyq;

			vDSP_zvadd(&input_filtered_signal_totals[0], 1, &input_filtered_signal_per_channel[0], 1, &input_filtered_signal_totals[0], 1, fftSizeOver2);
			vDSP_zvadd(&input_filtered_signal_totals[1], 1, &input_filtered_signal_per_channel[1], 1, &input_filtered_signal_totals[1], 1, fftSizeOver2);
		}

		vDSP_DFT_Execute(dftSetupB, input_filtered_signal_totals[0].realp, input_filtered_signal_totals[0].imagp, input_filtered_signal_totals[0].realp, input_filtered_signal_totals[0].imagp);
		vDSP_DFT_Execute(dftSetupB, input_filtered_signal_totals[1].realp, input_filtered_signal_totals[1].imagp, input_filtered_signal_totals[1].realp, input_filtered_signal_totals[1].imagp);

		vDSP_ztoc(&input_filtered_signal_totals[0], 1, (DSPComplex *)left_result, 2, fftSizeOver2);
		vDSP_ztoc(&input_filtered_signal_totals[1], 1, (DSPComplex *)right_result, 2, fftSizeOver2);

		float *left_ptr = left_result + prevToDo;
		float *right_ptr = right_result + prevToDo;

		vDSP_vsmul(left_ptr, 1, &scale, left_ptr, 1, countToDo);
		vDSP_vsmul(right_ptr, 1, &scale, right_ptr, 1, countToDo);

		cblas_scopy((int)countToDo, left_ptr, 1, outBuffer + 0, 2);
		cblas_scopy((int)countToDo, right_ptr, 1, outBuffer + 1, 2);

		inBuffer += countToDo * channelCount;
		outBuffer += countToDo * 2;

		count -= countToDo;
	}
}

- (void)reset {
	for(int i = 0; i < channelCount; ++i) {
		vDSP_vclr(prevInputs[i], 1, fftSize);
	}
}

@end
