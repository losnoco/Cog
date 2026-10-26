//
//  AudioContainer.m
//  CogAudio
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioContainer.h"

#import "PluginController.h"

@implementation AudioContainer

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	@autoreleasepool {
		return [[PluginController sharedPluginController] urlsForContainerURL:url];
	}
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)info {
	// Held outside the pool: what comes back through `info` is autoreleased,
	// and would go with it.
	NSArray *urls;
	NSDictionary *trackInfo = nil;
	@autoreleasepool {
		urls = [[PluginController sharedPluginController] urlsForContainerURL:url trackInfo:info ? &trackInfo : NULL];
	}
	if(info) *info = trackInfo;
	return urls;
}

+ (NSArray *)dependencyUrlsForContainerURL:(NSURL *)url {
	@autoreleasepool {
		return [[PluginController sharedPluginController] dependencyUrlsForContainerURL:url];
	}
}

@end
