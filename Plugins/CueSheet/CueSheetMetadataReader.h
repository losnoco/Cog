//
//  CueSheetMetadataReader.h
//  CueSheet
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

#import "CueSheetTrack.h"

@interface CueSheetMetadataReader : NSObject <CogMetadataReader> {
}

+ (NSDictionary *)processDataForTrack:(CueSheetTrack *)track;

@end
