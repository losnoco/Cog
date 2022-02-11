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
		return 0;

	if(![source seekable])
		return 0;

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	void *data = malloc(size);
	[source read:data amount:size];

	SidTune *tune = new SidTune((const uint_least8_t *)data, (uint_least32_t)size);

	if(!tune->getStatus()) {
		delete tune;
		return 0;
	}

	const SidTuneInfo *info = tune->getInfo();

	unsigned int count = info->numberOfInfoStrings();
	NSString *title = count >= 1 ? [[NSString stringWithUTF8String:info->infoString(0)] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] : @"";
	NSString *titletag = info->songs() > 1 ? @"album" : @"title";
	NSString *artist = count >= 2 ? [[NSString stringWithUTF8String:info->infoString(1)] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] : @"";

	delete tune;

	return @{titletag: title, @"artist": artist};
}

@end
