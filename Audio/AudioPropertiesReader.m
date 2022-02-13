//
//  AudioMetadataReader.m
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioPropertiesReader.h"
#import "AudioSource.h"
#import "Plugin.h"
#import "PluginController.h"

@implementation AudioPropertiesReader

+ (NSDictionary *)propertiesForURL:(NSURL *)url {
	@autoreleasepool {
		return [[PluginController sharedPluginController] propertiesForURL:url skipCue:NO];
	}
}

+ (NSDictionary *)propertiesForURL:(NSURL *)url skipCue:(BOOL)skip {
	@autoreleasepool {
		return [[PluginController sharedPluginController] propertiesForURL:url skipCue:skip];
	}
}

@end
