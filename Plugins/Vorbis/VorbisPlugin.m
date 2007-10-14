//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "VorbisPlugin.h"
#import "VorbisDecoder.h"

@implementation VorbisPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[VorbisDecoder	className],
		nil
	];
}

@end
