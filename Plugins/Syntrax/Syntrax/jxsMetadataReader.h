//
//  jxsMetadataReader.h
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Syntrax_c/jaytrax.h>

#import "Plugin.h"

@interface jxsMetadataReader : NSObject <CogMetadataReader> {
}

// The tags of the track `subsong` is.
+ (NSDictionary *)metadataForSubsong:(JT1Subsong *)subsong;

@end
