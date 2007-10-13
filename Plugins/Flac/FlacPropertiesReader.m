//
//  MADPropertiesReader.m
//  MAD
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FlacPropertiesReader.h"
#import "FlacDecoder.h"

@implementation FlacPropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	FlacDecoder *decoder;
	
	decoder = [[FlacDecoder alloc] init];
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
	return [FlacDecoder fileTypes];
}

@end
