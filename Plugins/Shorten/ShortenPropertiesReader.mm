//
//  MADPropertiesReader.m
//  MAD
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ShortenPropertiesReader.h"
#import "ShortenDecoder.h"

@implementation ShortenPropertiesReader

- (NSDictionary *)propertiesForURL:(NSURL *)url
{
	NSDictionary *properties;
	ShortenDecoder *decoder;
	
	decoder = [[ShortenDecoder alloc] init];
	if (![decoder open:url])
	{
		return nil;
	}
	
	properties = [decoder properties];
	
	[decoder close];
	
	return properties;
}


+ (NSArray *)fileTypes
{
	return [ShortenDecoder fileTypes];
}

@end
