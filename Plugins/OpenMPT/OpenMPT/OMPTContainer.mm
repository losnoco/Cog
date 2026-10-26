//
//  OMPTContainer.m
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <libOpenMPT/libopenmpt.hpp>

#import "OMPTContainer.h"
#import "OMPTDecoder.h"
#import "OMPTMetadataReader.h"

#import "Logging.h"

@implementation OMPTContainer

+ (NSArray *)fileTypes {
	return [OMPTDecoder fileTypes];
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
		return @[];

	if(![source seekable])
		return @[];

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	try {
		std::vector<char> data(static_cast<std::size_t>(size));

		[source read:data.data() amount:size];

		// Loaded as the decoder loads it, so lengths come out as its do.
		std::unique_ptr<openmpt::module> mod([OMPTDecoder newModuleWithData:data]);

		NSMutableArray *tracks = [NSMutableArray array];

		int i;
		int subsongs = mod->get_num_subsongs();

		for(i = 0; i < subsongs; ++i) {
			[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
		}

		if(trackInfo) {
			// Every subsong's length and tags from the module just loaded, as
			// the decoder and the metadata reader would find them.
			double sampleRate = [OMPTDecoder sampleRate];
			NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[tracks count]];
			i = 0;
			for(NSURL *track in tracks) {
				try {
					long length = [OMPTDecoder selectSubsong:i ofModule:mod.get() sampleRate:sampleRate];
					NSMutableDictionary *entryInfo = [[OMPTDecoder propertiesWithSampleRate:sampleRate length:length] mutableCopy];
					[entryInfo addEntriesFromDictionary:[OMPTMetadataReader metadataForModule:mod.get()]];
					info[track] = entryInfo;
				} catch(std::exception &e) {
					// Left for the readers to read as usual
					ALog(@"Exception caught while reading subsong %i with OpenMPT: %s", i, e.what());
				}
				++i;
			}
			*trackInfo = info;
		}

		return tracks;
	} catch(std::exception &e) {
		ALog(@"Exception caught while processing with OpenMPT: %s", e.what());
		return @[];
	}
}

@end
