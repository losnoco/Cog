//
//  HLSPlaylistParser.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Lightly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@class HLSPlaylist;

@interface HLSPlaylistParser : NSObject

+ (HLSPlaylist *)parsePlaylistFromSource:(id<CogSource>)source error:(NSError **)error;
+ (HLSPlaylist *)parsePlaylistString:(NSString *)playlistString baseURL:(NSURL *)baseURL error:(NSError **)error;

@end
