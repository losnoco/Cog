//
//  AudioDecoder.m
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AudioSource.h"


@implementation AudioSource

+ (id<CogSource>) audioSourceForURL:(NSURL *)url
{
	return [[PluginController sharedPluginController] audioSourceForURL:url];
}

@end
