//
//  MADPropertiesReader.m
//  MAD
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "WavPackPropertiesReader.h"
#import "WavPackDecoder.h"

@implementation WavPackPropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	WavPackDecoder *decoder;
	
	decoder = [[WavPackDecoder alloc] init];
	if (![decoder open:source])
	{
		[decoder release];
		return nil;
	}
	
	properties = [decoder properties];
	
	[decoder close];
	[decoder release];
	
	return properties;
}


+ (NSArray *)fileTypes
{
	return [WavPackDecoder fileTypes];
}

@end
