//
//  SidMetadataReader.mm
//  sidplay
//
//  Created by Christopher Snowhill on 12/8/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "SidMetadataReader.h"
#import "SidDecoder.h"

#import "Logging.H"

@implementation SidMetadataReader

+ (NSArray *)fileTypes {
	return [SidDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [SidDecoder mimeTypes];
}

+ (float)priority {
	return 0.5f;
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

	if(!size)
		return @{};
	
	void *data = malloc(size);
	if(!data)
		return @{};

	[source read:data amount:size];

	NSString *title = @"";
	NSString *titletag = @"title";
	NSString *artist = @"";

	try {
		SidTune *tune = new SidTune((const uint_least8_t *)data, (uint_least32_t)size);

		if(!tune->getStatus()) {
			delete tune;
			return 0;
		}

		const SidTuneInfo *info = tune->getInfo();

		unsigned int count = info->numberOfInfoStrings();
		title = count >= 1 ? [guess_encoding_of_string(info->infoString(0)) stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] : @"";
		titletag = info->songs() > 1 ? @"album" : @"title";
		artist = count >= 2 ? [guess_encoding_of_string(info->infoString(1)) stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] : @"";

		delete tune;
	} catch (std::exception &e) {
		ALog(@"Exception caught while reading SID tags: %s", e.what());
		return @{};
	}

	return @{titletag: title, @"artist": artist};
}

@end
