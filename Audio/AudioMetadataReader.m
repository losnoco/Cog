//
//  AudioMetadataReader.m
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioMetadataReader.h"
#import "PluginController.h"

@implementation AudioMetadataReader

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	@autoreleasepool {
		return [[PluginController sharedPluginController] metadataForURL:url skipCue:NO];
	}
}

+ (NSDictionary *)metadataForURL:(NSURL *)url skipCue:(BOOL)skip {
	@autoreleasepool {
		return [[PluginController sharedPluginController] metadataForURL:url skipCue:skip];
	}
}

@end
