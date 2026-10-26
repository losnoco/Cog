//
//  GameMetadataReader.h
//  GME
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

#import <GME/gme.h>

@interface GameMetadataReader : NSObject <CogMetadataReader> {
}

// The tags of track `track` of the file at `url`, from its info.
+ (NSDictionary *)metadataForTrackInfo:(gme_info_t *)info url:(NSURL *)url track:(int)track;

@end
