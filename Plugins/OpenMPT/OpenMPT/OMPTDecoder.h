//
//  OMPTDecoder.h
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <libOpenMPT/libopenmpt.hpp>

#include <vector>

#import "Plugin.h"

@interface OMPTDecoder : NSObject <CogDecoder> {
	openmpt::module *mod;

	id<CogSource> source;
	long length;

	double sampleRate;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;

// The rate modules play at, from the user's settings.
+ (double)sampleRate;
// A module loaded from `data` as the decoder loads it. Throws as
// openmpt::module does.
+ (openmpt::module *)newModuleWithData:(const std::vector<char> &)data;
// Selects `subsong` of `mod` and gives its length in frames at `sampleRate`.
+ (long)selectSubsong:(int)subsong ofModule:(openmpt::module *)mod sampleRate:(double)sampleRate;
// The properties of a subsong `length` frames long.
+ (NSDictionary *)propertiesWithSampleRate:(double)sampleRate length:(long)length;
@end
