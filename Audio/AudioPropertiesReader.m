//
//  AudioMetadataReader.m
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioPropertiesReader.h"
#import "PluginController.h"

@implementation AudioPropertiesReader

+ (NSDictionary *)propertiesForURL:(NSURL *)url
{
	NSString *ext = [[url path] pathExtension];
	
	NSDictionary *propertiesReaders = [[PluginController sharedPluginController] propertiesReaders];
	
	Class propertiesReader = NSClassFromString([propertiesReaders objectForKey:ext]);
	
	return [[[[propertiesReader alloc] init] autorelease] propertiesForURL:url];

}

@end
