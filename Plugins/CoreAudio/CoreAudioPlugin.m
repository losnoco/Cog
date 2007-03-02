//
//  CoreAudio.m
//  CoreAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CoreAudioPlugin.h"
#import "CoreAudioDecoder.h"
#import "CoreAudioPropertiesReader.h"

@implementation CoreAudioPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[CoreAudioDecoder className],
		kCogPropertiesReader, 	[CoreAudioPropertiesReader className],
		nil
	];
}

@end
