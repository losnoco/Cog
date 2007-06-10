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
	NSString *ext = [[url path] pathExtension];
	
	id<CogSource> source = [AudioSource audioSourceForURL:url];
	if (![source open:url])
		return nil;
	
	NSDictionary *propertiesReaders = [[PluginController sharedPluginController] propertiesReaders];
	
	Class propertiesReader = NSClassFromString([propertiesReaders objectForKey:[ext lowercaseString]]);
	
	NSDictionary *properties =  [propertiesReader propertiesForSource:source];
	
	[source close];
	
	return properties;

}

@end
