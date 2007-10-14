//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MusepackPlugin.h"
#import "MusepackDecoder.h"

@implementation MusepackPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[MusepackDecoder className],
		nil
	];
}

@end
