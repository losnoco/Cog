//
//  OMPTMetadataReader.m
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import "OMPTMetadataReader.h"
#import "OMPTDecoder.h"

#import <libOpenMPTOld/libopenmpt.hpp>

#import "Logging.H"

@implementation OMPTOldMetadataReader

+ (NSArray *)fileTypes {
	return [OMPTOldDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [OMPTOldDecoder mimeTypes];
}

+ (float)priority {
	return [OMPTOldDecoder priority];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
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

	int track_num;
	if([[url fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[url fragment] intValue];

	try {
		std::map<std::string, std::string> ctls;
		openmpt::module *mod = new openmpt::module(data, std::clog, ctls);

		NSString *title = nil;
		NSString *artist = nil;
		// NSString * comment = nil;
		NSString *date = nil;
		NSString *type = nil;

		std::vector<std::string> keys = mod->get_metadata_keys();

		for(std::vector<std::string>::iterator key = keys.begin(); key != keys.end(); ++key) {
			if(*key == "title")
				title = [NSString stringWithUTF8String:mod->get_metadata(*key).c_str()];
			else if(*key == "artist")
				artist = [NSString stringWithUTF8String:mod->get_metadata(*key).c_str()];
			/*else if ( *key == "message" )
			    comment = [NSString stringWithUTF8String: mod->get_metadata( *key ).c_str()];*/
			else if(*key == "date")
				date = [NSString stringWithUTF8String:mod->get_metadata(*key).c_str()];
			else if(*key == "type_long")
				type = [NSString stringWithUTF8String:mod->get_metadata(*key).c_str()];
		}

		delete mod;

		if(title == nil)
			title = @"";
		if(artist == nil)
			artist = @"";
		/*if (comment == nil)
		    comment = @"";*/
		if(date == nil)
			date = @"";
		if(type == nil)
			type = @"";

		return [NSDictionary dictionaryWithObjectsAndKeys:title, @"title", artist, @"artist", /*comment, @"comment",*/ date, @"year", type, @"codec", nil];
	} catch(std::exception & /*e*/) {
		return 0;
	}
}

@end
