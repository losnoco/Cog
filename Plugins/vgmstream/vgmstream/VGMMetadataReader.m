//
//  VGMMetadataReader.m
//  VGMStream
//
//  Created by Christopher Snowhill on 9/18/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import "VGMMetadataReader.h"
#import "VGMDecoder.h"
#import "VGMInterface.h"

@implementation VGMMetadataReader

+ (NSArray *)fileTypes {
	return [VGMDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [VGMDecoder mimeTypes];
}

+ (float)priority {
	return [VGMDecoder priority];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	VGMInfoCache *sharedMyCache = [VGMInfoCache sharedCache];

	NSDictionary *metadata = [sharedMyCache getMetadataForURL:url];

	if(!metadata) {
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

		metadata = [sharedMyCache getMetadataForURL:url];
	}

	return metadata;
}

@end
