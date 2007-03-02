//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FlacPlugin.h"
#import "FlacDecoder.h"
#import "FlacPropertiesReader.h"

@implementation FlacPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[FlacDecoder className],
		kCogPropertiesReader, 	[FlacPropertiesReader className],
		nil
	];
}


@end
