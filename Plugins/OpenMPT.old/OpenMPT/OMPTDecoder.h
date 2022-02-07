//
//  OMPTDecoder.h
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libOpenMPTOld/libopenmpt.hpp>

#include <vector>

#import "Plugin.h"

@interface OMPTOldDecoder : NSObject <CogDecoder> {
	openmpt::module *mod;
	std::vector<float> left;
	std::vector<float> right;

	id<CogSource> source;
	long length;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end
