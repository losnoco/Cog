//
//  VisualizationController.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "VisualizationController.h"
#import <Accelerate/Accelerate.h>

#import "fft.h"

@implementation VisualizationController

static VisualizationController *_sharedController = nil;

+ (VisualizationController *)sharedController {
	@synchronized(self) {
		if(!_sharedController) {
			_sharedController = [[VisualizationController alloc] init];
		}
	}
	return _sharedController;
}

- (id)init {
	self = [super init];
	if(self) {
		vDSP_vclr(visAudio, 1, 4096);
	}
	return self;
}

- (void)dealloc {
	fft_free();
}

- (void)postSampleRate:(double)sampleRate {
	@synchronized(self) {
		self->sampleRate = sampleRate;
	}
}

- (void)postVisPCM:(const float *)inPCM {
	@synchronized(self) {
		cblas_scopy(4096 - 512, visAudio + 512, 1, visAudio, 1);
		cblas_scopy(512, inPCM, 1, visAudio + 4096 - 512, 1);
	}
}

- (double)readSampleRate {
	@synchronized(self) {
		return sampleRate;
	}
}

- (void)copyVisPCM:(float *)outPCM visFFT:(float *)outFFT {
	@synchronized(self) {
		cblas_scopy(4096, visAudio, 1, outPCM, 1);
		fft_calculate(visAudio, outFFT, 2048);
	}
}

@end
