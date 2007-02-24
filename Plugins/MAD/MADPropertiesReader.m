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

- (NSDictionary *)propertiesForURL:(NSURL *)url
{
	NSDictionary *properties;
	MADDecoder *decoder;
	
	decoder = [[MADDecoder alloc] init];
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
	return [NSArray arrayWithObject:@"mp3"];
}

@end
