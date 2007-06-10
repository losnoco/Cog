//
//  QuicktimePropertiesReader.m
//  Quicktime
//
//  Created by Vincent Spader on 6/10/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "QuicktimePropertiesReader.h"
#import "QuicktimeDecoder.h"

@implementation QuicktimePropertiesReader


+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	QuicktimeDecoder *decoder;
	
	decoder = [[QuicktimeDecoder alloc] init];
	if (![decoder open:source])
	{
		return nil;
	}
	
	properties = [decoder properties];
	
	[decoder close];
	
	return properties;
}


+ (NSArray *)fileTypes
{
	return [QuicktimeDecoder fileTypes];
}


@end
