//
//  DumbPlugin.m
//  GME
//
//  Created by Vincent Spader on 10/11/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "DumbPlugin.h"

#import "DumbDecoder.h"
#import "DumbPropertiesReader.h"
#import "DumbMetadataReader.h"

@implementation DumbPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogDecoder, [DumbDecoder className],
		kCogPropertiesReader, [DumbPropertiesReader className],
		kCogMetadataReader, [DumbMetadataReader className],
		nil];
}

@end
