//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "WavPackPlugin.h"
#import "WavPackDecoder.h"
#import "WavPackPropertiesReader.h"

@implementation WavPackPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[WavPackDecoder className],
		kCogPropertiesReader, 	[WavPackPropertiesReader className],
		nil
	];
}

@end
