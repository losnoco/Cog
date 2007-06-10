//
//  AudioDecoder.m
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioDecoder.h"

#import "PluginController.h"

@implementation AudioDecoder

+ (id<CogDecoder>) audioDecoderForURL:(NSURL *)url
{
	NSString *ext = [[url path] pathExtension];
	
	NSDictionary *decoders = [[PluginController sharedPluginController] decoders];
	
	Class decoder = NSClassFromString([decoders objectForKey:[ext lowercaseString]]);
	
	return [[[decoder alloc] init] autorelease];
}

@end
