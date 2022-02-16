//
//  BadSampleCleaner.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/15/22.
//

#ifdef _DEBUG
#import "BadSampleCleaner.h"

#import "Logging.h"

@implementation BadSampleCleaner

+ (void)cleanSamples:(float *)buffer amount:(NSUInteger)amount location:(NSString *)location {
	BOOL hadNaN = NO;
	BOOL hadINF = NO;
	BOOL hadHUGE = NO;

	for(NSUInteger i = 0; i < amount; ++i) {
		float sample = buffer[i];
		BOOL isNaN = isnan(sample);
		BOOL isINF = isinf(sample);
		BOOL isHUGE = (fabs(sample) > 2.0);
		hadNaN = hadNaN || isNaN;
		hadINF = hadINF || isINF;
		hadHUGE = hadHUGE || isHUGE;
		if(isNaN || isINF || isHUGE) {
			memset(&buffer[i], 0, sizeof(buffer[i]));
			memset(&sample, 0, sizeof(sample));
		}
	}
	if(hadNaN || hadINF || hadHUGE) {
		DLog(@"Sample block at %@ had NaN: %@, INF: %@, HUGE: %@", location, hadNaN ? @"yes" : @"no", hadINF ? @"yes" : @"no", hadHUGE ? @"yes" : @"no");
	}
}

@end

#endif
