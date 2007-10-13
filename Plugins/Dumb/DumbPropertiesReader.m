//
//  DumbPropertiesReader.m
//  GME
//
//  Created by Vincent Spader on 10/10/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "DumbPropertiesReader.h"
#import "DumbDecoder.h"

@implementation DumbPropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	DumbDecoder *decoder;
	
	decoder = [[DumbDecoder alloc] init];
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
	return [DumbDecoder fileTypes];
}

@end
