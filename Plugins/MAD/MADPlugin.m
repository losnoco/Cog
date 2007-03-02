//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MADPlugin.h"
#import "MADDecoder.h"
#import "MADPropertiesReader.h"

@implementation MADPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, 			[MADDecoder className],
		kCogPropertiesReader, 	[MADPropertiesReader className],
		nil
	];
}

@end
