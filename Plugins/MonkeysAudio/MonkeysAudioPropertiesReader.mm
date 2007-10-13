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

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	MonkeysAudioDecoder *decoder;
	
	decoder = [[MonkeysAudioDecoder alloc] init];
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
	return [MonkeysAudioDecoder fileTypes];
}

@end
