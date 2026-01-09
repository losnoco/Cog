//
//  libvgmPropertiesReader.m
//  libvgmPlayer
//
//  Created by Christopher Snowhill on 01/02/22.
//  Copyright 2022-2026 __LoSnoCo__. All rights reserved.
//

#import "libvgmPropertiesReader.h"
#import "libvgmDecoder.h"

@implementation libvgmPropertiesReader

+ (NSArray *)fileTypes {
	return [libvgmDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [libvgmDecoder mimeTypes];
}

+ (float)priority {
	return [libvgmDecoder priority];
}

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source {
	libvgmDecoder *decoder = [libvgmDecoder new];

	NSDictionary *properties = @{};

	if([decoder open:source]) {
		properties = [decoder properties];

		[decoder close];
	}

	return properties;
}

@end
