//
//  VisualizationController.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "VisualizationController.h"
#import <Accelerate/Accelerate.h>

#import "cqt.h"

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
	cqt_free();
}

- (void)postSampleRate:(double)sampleRate {
	@synchronized(self) {
		self->sampleRate = sampleRate;
	}
}

- (void)postVisPCM:(const float *)inPCM amount:(int)amount {
	int skipAmount = 0;
	if(amount > 4096) {
		skipAmount = amount - 4096;
		amount = 4096;
	}
	@synchronized(self) {
		cblas_scopy(4096 - amount, visAudio + amount, 1, visAudio, 1);
		cblas_scopy(amount, inPCM + skipAmount, 1, visAudio + 4096 - amount, 1);
	}
}

- (double)readSampleRate {
	@synchronized(self) {
		return sampleRate;
	}
}

- (void)copyVisPCM:(float *)outPCM visCQT:(float *)outCQT {
	@synchronized(self) {
		cblas_scopy(4096, visAudio, 1, outPCM, 1);
		cqt_calculate(visAudio, sampleRate, outCQT, 4096);
	}
}

@end
