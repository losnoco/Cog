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

+ (NSDictionary *)propertiesForURL:(NSURL *)url
{
	return [[PluginController sharedPluginController] propertiesForURL:url];
}

@end
