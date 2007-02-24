//
//  MADPropertiesReader.m
//  MAD
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MonkeysAudioPropertiesReader.h"
#import "MonkeysAudioDecoder.h"

@implementation MonkeysAudioPropertiesReader

- (NSDictionary *)propertiesForURL:(NSURL *)url
{
	NSDictionary *properties;
	MonkeysAudioDecoder *decoder;
	
	decoder = [[MonkeysAudioDecoder alloc] init];
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
	return [MonkeysAudioDecoder fileTypes];
}

@end
