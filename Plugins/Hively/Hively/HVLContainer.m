//
//  HVLContainer.m
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "HVLContainer.h"
#import "HVLDecoder.h"

@implementation HVLContainer

+ (NSArray *)fileTypes {
	return [HVLDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 1.0f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return 0;

	if(![source seekable])
		return 0;

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	if(size > UINT_MAX)
		return nil;

	void *data = malloc(size);
	[source read:data amount:size];

	struct hvl_tune *tune = hvl_LoadTune(data, (uint32_t)size, 44100, 2);
	free(data);
	if(!tune)
		return nil;

	int subsongNr = tune->ht_SubsongNr;

	hvl_FreeTune(tune);

	NSMutableArray *tracks = [NSMutableArray array];

	for(int i = 0; i <= subsongNr; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	return tracks;
}

@end
