//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "TagLibPlugin.h"
#import "TagLibMetadataReader.h"

@implementation TagLibPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogMetadataReader, [TagLibMetadataReader className],
		nil
	];
}

@end
