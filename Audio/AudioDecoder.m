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

+ (id<CogDecoder>) audioDecoderForSource:(id <CogSource>)source
{
	return [[PluginController sharedPluginController] audioDecoderForSource:source];
}

@end
