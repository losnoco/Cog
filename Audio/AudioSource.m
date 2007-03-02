//
//  AudioDecoder.m
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioSource.h"


@implementation AudioSource

+ audioSourceForURL:(NSURL *)url
{
	NSString *scheme = [url scheme];
	
	NSDictionary *sources = [[PluginController sharedPluginController] sources];
	
	Class source = NSClassFromString([sources objectForKey:scheme]);
	
	return [[[source alloc] init] autorelease];
}

@end
