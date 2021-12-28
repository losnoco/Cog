//
//  OMPTDecoder.h
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <libOpenMPT/libopenmpt.hpp>

#include <vector>

#import "Plugin.h"

@interface OMPTDecoder : NSObject <CogDecoder> {
    openmpt::module *mod;
    
	id<CogSource> source;
    long length;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;
- (void)cleanUp;
@end
