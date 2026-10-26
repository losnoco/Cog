//
//  HVLContainer.m
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "HVLContainer.h"
#import "HVLDecoder.h"
#import "HVLMetadataReader.h"

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
	return [self urlsForContainerURL:url trackInfo:NULL];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)trackInfo {
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

	// Loaded at the rate the decoder plays at, so lengths come out as its do.
	double sampleRate = [HVLDecoder sampleRate];

	struct hvl_tune *tune = hvl_LoadTune(data, (uint32_t)size, (int)sampleRate, 2);
	free(data);
	if(!tune)
		return nil;

	int subsongNr = tune->ht_SubsongNr;

	NSMutableArray *tracks = [NSMutableArray array];

	for(int i = 0; i <= subsongNr; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	if(trackInfo) {
		// Every subsong's length and tags from the tune just loaded, as the
		// decoder and the metadata reader would find them.
		NSDictionary *metadata = [HVLMetadataReader metadataForTune:tune];
		long framesFade = [HVLDecoder framesFadeForSampleRate:sampleRate];
		NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[tracks count]];
		int i = 0;
		for(NSURL *track in tracks) {
			long framesLength = [HVLDecoder framesLengthOfTune:tune subsong:i sampleRate:sampleRate];
			NSMutableDictionary *entryInfo = [[HVLDecoder propertiesWithSampleRate:sampleRate totalFrames:framesLength + framesFade] mutableCopy];
			[entryInfo addEntriesFromDictionary:metadata];
			info[track] = entryInfo;
			++i;
		}
		*trackInfo = info;
	}

	hvl_FreeTune(tune);

	return tracks;
}

@end
