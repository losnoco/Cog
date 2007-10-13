//
//  GamePropertiesReader.m
//  GME
//
//  Created by Vincent Spader on 10/10/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "GamePropertiesReader.h"
#import "GameDecoder.h"

@implementation GamePropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	GameDecoder *decoder;
	
	decoder = [[GameDecoder alloc] init];
	if (![decoder open:source])
	{
		NSLog(@"Could not open");
		[decoder release];
		return nil;
	}
	
	properties = [decoder properties];

	NSLog(@"Properties! %@", properties);
		
	[decoder close];
	[decoder release];
	
	return properties;
}


+ (NSArray *)fileTypes
{
	return [GameDecoder fileTypes];
}

@end
