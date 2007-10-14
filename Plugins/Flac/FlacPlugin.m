//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FlacPlugin.h"
#import "FlacDecoder.h"

@implementation FlacPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[FlacDecoder className],
		nil
	];
}


@end
