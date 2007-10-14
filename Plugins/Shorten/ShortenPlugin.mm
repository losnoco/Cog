//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ShortenPlugin.h"
#import "ShortenDecoder.h"

@implementation ShortenPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[ShortenDecoder className],
		nil
	];
}

@end
