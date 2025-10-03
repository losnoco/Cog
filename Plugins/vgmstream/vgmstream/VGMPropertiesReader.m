//
//  VGMPropertiesReader.m
//  VGMStream
//
//  Created by Christopher Snowhill on 10/18/19.
//  Copyright 2019-2025 __LoSnoCo__. All rights reserved.
//

#import "VGMPropertiesReader.h"
#import "VGMDecoder.h"
#import "VGMInterface.h"

@implementation VGMPropertiesReader

+ (NSArray *)fileTypes {
	return [VGMDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [VGMDecoder mimeTypes];
}

+ (float)priority {
	return [VGMDecoder priority];
}

+ (NSDictionary *)propertiesForSource:(id<CogSource>)source {
	VGMInfoCache *sharedMyCache = [VGMInfoCache sharedCache];

	NSURL *url = [source url];

	NSDictionary *properties = [sharedMyCache getPropertiesForURL:url];

	if(!properties) {
		int track_num = [[url fragment] intValue];

		NSString *path = [url absoluteString];
		NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
		if(fragmentRange.location != NSNotFound) {
			path = [path substringToIndex:fragmentRange.location];
		}

		libvgmstream_config_t vcfg = { 0 };

		vcfg.allow_play_forever = 1;
		vcfg.play_forever = 0;
		vcfg.loop_count = 2;
		vcfg.fade_time = 10;
		vcfg.fade_delay = 0;
		vcfg.ignore_loop = 0;

		libstreamfile_t* sf = open_vfs([[path stringByRemovingPercentEncoding] UTF8String]);
		if(!sf)
			return nil;
		libvgmstream_t* stream = libvgmstream_create(sf, track_num, &vcfg);
		libstreamfile_close(sf);
		if(!stream)
			return nil;

		[sharedMyCache stuffURL:url stream:stream];

		libvgmstream_free(stream);

		properties = [sharedMyCache getPropertiesForURL:url];
	}

	return properties;
}

@end
