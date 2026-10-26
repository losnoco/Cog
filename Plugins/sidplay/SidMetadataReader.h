//
//  SidMetadataReader.h
//  sidplay
//
//  Created by Christopher Snowhill on 12/8/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <sidplayfp/SidTuneInfo.h>

#import "Plugin.h"

@interface SidMetadataReader : NSObject <CogMetadataReader> {
}

// The tags of every song of a tune, from its info.
+ (NSDictionary *)metadataForTuneInfo:(const SidTuneInfo *)info;

@end
