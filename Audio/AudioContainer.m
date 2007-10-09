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

+ (NSArray *) urlsForContainerURL:(NSURL *)url
{
	return [[PluginController sharedPluginController] urlsForContainerURL:url];
}

@end
