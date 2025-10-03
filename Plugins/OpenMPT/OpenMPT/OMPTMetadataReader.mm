//
//  OMPTMetadataReader.m
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import "OMPTMetadataReader.h"
#import "OMPTDecoder.h"

#import <libOpenMPT/libopenmpt.hpp>

#import "Logging.H"

@implementation OMPTMetadataReader

+ (NSArray *)fileTypes {
	return [OMPTDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [OMPTDecoder mimeTypes];
}

+ (float)priority {
	return 1.0f;
}

static void setDictionary(NSMutableDictionary *dict, NSString *tag, NSString *value) {
	NSString *realKey = [tag stringByReplacingOccurrencesOfString:@"." withString:@"․"];
	NSMutableArray *array = [dict valueForKey:realKey];
	if(!array) {
		array = [NSMutableArray new];
		[dict setObject:array forKey:realKey];
	}
	[array addObject:value];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return @{};

	if(![source seekable])
		return @{};

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	try {
		std::vector<char> data(static_cast<std::size_t>(size));

		[source read:data.data() amount:size];

		int track_num;
		if([[url fragment] length] == 0)
			track_num = 0;
		else
			track_num = [[url fragment] intValue];

		std::map<std::string, std::string> ctls;
		openmpt::module *mod = new openmpt::module(data, std::clog, ctls);

		NSMutableDictionary *dict = [NSMutableDictionary new];

		std::vector<std::string> keys = mod->get_metadata_keys();

		for(std::vector<std::string>::iterator key = keys.begin(); key != keys.end(); ++key) {
			@autoreleasepool {
				NSString *tag = guess_encoding_of_string((*key).c_str());
				NSString *value = guess_encoding_of_string(mod->get_metadata(*key).c_str());
				if(*key == "type")
					continue;
				else if(*key == "type_long") {
					setDictionary(dict, @"codec", value);
				} else {
					setDictionary(dict, tag, value);
				}
			}
		}

		delete mod;

		return dict;
	} catch(std::exception &e) {
		ALog(@"Exception caught while reading metadata with OpenMPT: %s", e.what());
		return @{};
	}
}

@end
