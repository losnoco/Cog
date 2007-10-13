//
//  GamePlugin.m
//  GME
//
//  Created by Vincent Spader on 10/11/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "GamePlugin.h"

#import "GameContainer.h"
#import "GameDecoder.h"
#import "GamePropertiesReader.h"
#import "GameMetadataReader.h"

@implementation GamePlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogContainer, [GameContainer className],
		kCogDecoder, [GameDecoder className],
		kCogPropertiesReader, [GamePropertiesReader className],
		kCogMetadataReader, [GameMetadataReader className],
		nil];
}

@end
