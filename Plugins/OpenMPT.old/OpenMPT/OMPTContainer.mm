//
//  OMPTContainer.m
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import <libOpenMPTOld/libopenmpt.hpp>

#import "OMPTContainer.h"
#import "OMPTDecoder.h"

#import "Logging.h"

@implementation OMPTOldContainer

+ (NSArray *)fileTypes {
	return [OMPTOldDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return [OMPTOldDecoder priority];
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

	std::vector<char> data(static_cast<std::size_t>(size));

	[source read:data.data() amount:size];

	try {
		std::map<std::string, std::string> ctls;
		openmpt::module *mod = new openmpt::module(data, std::clog, ctls);

		NSMutableArray *tracks = [NSMutableArray array];

		int i;
		int subsongs = mod->get_num_subsongs();

		delete mod;

		for(i = 0; i < subsongs; ++i) {
			[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
		}

		return tracks;
	} catch(std::exception & /*e*/) {
		return 0;
	}
}

@end
