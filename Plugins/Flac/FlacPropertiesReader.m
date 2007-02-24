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

- (NSDictionary *)propertiesForURL:(NSURL *)url
{
	NSDictionary *properties;
	FlacDecoder *decoder;
	
	decoder = [[FlacDecoder alloc] init];
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
	return [FlacDecoder fileTypes];
}

@end
