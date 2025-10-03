//
//  VGMContainer.m
//  VGMStream
//
//  Created by Christopher Snowhill on 9/1/17.
//  Copyright 2017 __LoSnoCo__. All rights reserved.
//

#import "VGMContainer.h"
#import "VGMDecoder.h"
#import "VGMInterface.h"

#import "Logging.h"

@implementation VGMContainer

+ (void)initialize {
	register_log_callback();
}

+ (NSArray *)fileTypes {
	return [VGMDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return [VGMDecoder priority];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	NSString *path = [[url absoluteString] stringByRemovingPercentEncoding];

	if([url fragment]) {
		// input url might be a TXTP with fragment parameters, in which case
		// we need to parse that anyway
		NSString *frag = [url fragment];
		NSUInteger len = [frag length];
		if(len < 5 || ![[frag substringFromIndex:len - 5] isEqualToString:@".txtp"]) {
			// input url already has fragment defined - no need to expand further
			return [NSMutableArray arrayWithObject:url];
		}

		path = [path stringByAppendingFormat:@"%%23%@", frag];
		url = [NSURL fileURLWithPath:[path stringByRemovingPercentEncoding]];
	}

	libstreamfile_t* sf = open_vfs([path UTF8String]);
	if(!sf) return @[];

	libvgmstream_config_t vcfg = {0};

	libvgmstream_t* infostream = libvgmstream_create(sf, 0, &vcfg);
	if(!infostream) {
		libstreamfile_close(sf);
		return @[];
	}

	VGMInfoCache *sharedMyCache = [VGMInfoCache sharedCache];

	NSMutableArray *tracks = [NSMutableArray array];
	NSURL *trackurl;

	int i;
	int subsongs = infostream->format->subsong_count;
	if(subsongs == 0)
		subsongs = 1;
	if(infostream->format->subsong_index > 0)
		subsongs = 1;

	{
		trackurl = [NSURL URLWithString:[[url absoluteString] stringByAppendingString:@"#1"]];
		[sharedMyCache stuffURL:trackurl stream:infostream];
		[tracks addObject:trackurl];
	}

	for(i = 2; i <= subsongs; ++i) {
		libvgmstream_close_stream(infostream);

		int ret = libvgmstream_open_stream(infostream, sf, i);

		if(ret != 0) {
			libvgmstream_free(infostream);
			libstreamfile_close(sf);
			return @[];
		}

		trackurl = [NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]];
		[sharedMyCache stuffURL:trackurl stream:infostream];
		[tracks addObject:trackurl];
	}

	libvgmstream_free(infostream);
	libstreamfile_close(sf);

	return tracks;
}

@end
