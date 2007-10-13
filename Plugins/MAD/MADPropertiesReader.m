//
//  MADPropertiesReader.m
//  MAD
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MADPropertiesReader.h"
#import "MADDecoder.h"

@implementation MADPropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	MADDecoder *decoder;
	
	decoder = [[MADDecoder alloc] init];
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
	return [NSArray arrayWithObject:@"mp3"];
}

@end
