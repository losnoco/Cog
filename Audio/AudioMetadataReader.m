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

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	NSString *ext = [[url path] pathExtension];
	
	NSDictionary *metadataReaders = [[PluginController sharedPluginController] metadataReaders];
	
	Class metadataReader = NSClassFromString([metadataReaders objectForKey:[ext lowercaseString]]);
	
	return [[[[metadataReader alloc] init] autorelease] metadataForURL:url];

}

@end
