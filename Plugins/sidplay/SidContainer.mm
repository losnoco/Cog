//
//  SidContainer.mm
//  sidplay
//
//  Created by Christopher Snowhill on 12/8/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "SidContainer.h"
#import "SidDecoder.h"

#import "Logging.h"

@implementation SidContainer

+ (NSArray *)fileTypes {
	return [SidDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 0.5f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return @[];

	if(![source seekable])
		return @[];

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	if(!size)
		return @[];

	void *data = malloc(size);
	if(!data)
		return @[];
	
	[source read:data amount:size];

	unsigned int subsongs = 0;

	try {
		SidTune *tune = new SidTune((const uint_least8_t *)data, (uint_least32_t)size);

		if(!tune->getStatus()) {
			delete tune;
			return 0;
		}

		const SidTuneInfo *info = tune->getInfo();

		subsongs = info->songs();

		delete tune;
	} catch (std::exception &e) {
		ALog(@"Exception caught processing SID file for song count: %s", e.what());
		return @[];
	}

	NSMutableArray *tracks = [NSMutableArray array];

	for(unsigned int i = 1; i <= subsongs; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	return tracks;
}

@end
