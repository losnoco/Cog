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
		vDSP_vclr(visAudio, 1, 512);
	}
	return self;
}

- (void)dealloc {
	fft_free();
}

- (void)postVisPCM:(const float *)inPCM {
	@synchronized(self) {
		cblas_scopy(512, inPCM, 1, visAudio, 1);
	}
}

- (void)copyVisPCM:(float *)outPCM visFFT:(float *)outFFT {
	@synchronized(self) {
		cblas_scopy(512, visAudio, 1, outPCM, 1);
		fft_calculate(visAudio, outFFT, 256);
	}
}

@end
