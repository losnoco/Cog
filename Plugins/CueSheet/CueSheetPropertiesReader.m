//
//  CueSheetPropertiesReader.m
//  CueSheet
//
//  Created by Vincent Spader on 10/08/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetPropertiesReader.h"
#import "CueSheetDecoder.h"

@implementation CueSheetPropertiesReader

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source
{
	NSDictionary *properties;
	CueSheetDecoder *decoder;
	
	decoder = [[CueSheetDecoder alloc] init];
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
	return [CueSheetDecoder fileTypes];
}

@end
