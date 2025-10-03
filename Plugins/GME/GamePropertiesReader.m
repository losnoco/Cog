//
//  VGMPropertiesReader.m
//  VGMStream
//
//  Created by Christopher Snowhill on 10/18/19.
//  Copyright 2019-2025 __LoSnoCo__. All rights reserved.
//

#import "GamePropertiesReader.h"
#import "GameDecoder.h"

@implementation GamePropertiesReader

+ (NSArray *)fileTypes {
	return [GameDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [GameDecoder mimeTypes];
}

+ (float)priority {
	return [GameDecoder priority];
}

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source {
	GameDecoder *decoder = [GameDecoder new];

	NSDictionary *properties = @{};

	if([decoder open:source]) {
		properties = [decoder properties];

		[decoder close];
	}

	return properties;
}

@end
