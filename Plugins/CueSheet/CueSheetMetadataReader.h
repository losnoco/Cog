//
//  CueSheetMetadataReader.h
//  CueSheet
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

#import "CueSheetTrack.h"

@interface CueSheetMetadataReader : NSObject <CogMetadataReader> {
}

+ (NSDictionary *)processDataForTrack:(CueSheetTrack *)track;
// The tags of `track`, the cue sheet's over `fileMetadata`, those of the
// audio file it plays.
+ (NSDictionary *)metadataForTrack:(CueSheetTrack *)track fileMetadata:(NSDictionary *)fileMetadata;

@end
