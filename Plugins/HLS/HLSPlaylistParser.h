//
//  HLSPlaylistParser.h
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "Plugin.h"

@class HLSPlaylist;

extern NSString *const HLSParserErrorDomain;

typedef NS_ENUM(NSInteger, HLSParserErrorCode) {
	HLSParserErrorReadFailed = 1,
	HLSParserErrorEncoding = 2,
	HLSParserErrorMissingHeader = 3,
	HLSParserErrorEmpty = 4,
};

@interface HLSPlaylistParser : NSObject

// Reads the entire source and parses it as an m3u8 playlist. The base URL for
// resolving relative URIs is taken from `[source url]`.
+ (HLSPlaylist *)parsePlaylistFromSource:(id<CogSource>)source
                                   error:(NSError **)error;

// Parses an m3u8 playlist already in memory. baseURL is used to resolve
// relative segment / variant URIs.
+ (HLSPlaylist *)parsePlaylistString:(NSString *)playlistString
                             baseURL:(NSURL *)baseURL
                               error:(NSError **)error;

@end
