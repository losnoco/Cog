//
//  OMPTMetadataReader.h
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

#import <libOpenMPT/libopenmpt.hpp>

@interface OMPTMetadataReader : NSObject <CogMetadataReader> {
}

// The tags of every subsong of `mod`. Throws as openmpt::module does.
+ (NSDictionary *)metadataForModule:(openmpt::module *)mod;

@end
