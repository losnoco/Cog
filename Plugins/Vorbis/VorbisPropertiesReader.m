//
//  MADPropertiesReader.m
//  MAD
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "VorbisPropertiesReader.h"
#import "VorbisDecoder.h"

@implementation VorbisPropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	VorbisDecoder *decoder;
	
	decoder = [[VorbisDecoder alloc] init];
	if (![decoder open:source])
	{
		return nil;
	}
	
	properties = [decoder properties];
	
	[decoder close];
	[decoder release];
	
	return properties;
}


+ (NSArray *)fileTypes
{
	return [VorbisDecoder fileTypes];
}

@end
