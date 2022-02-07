//
//  DumbContainer.m
//  Dumb
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Dumb/dumb.h>

#import "DumbContainer.h"
#import "DumbDecoder.h"

#import "Logging.h"

@interface DumbCallbackData : NSObject {
	@public
	NSString *baseUrl;
	@public
	NSMutableArray *tracks;
}
@end

@implementation DumbCallbackData
@end

@implementation DumbContainer

+ (NSArray *)fileTypes {
	return [DumbDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 1.0f;
}

int scanCallback(void *data, int startOrder, long length) {
	NSObject *_cbData = (__bridge NSObject *)(data);
	DumbCallbackData *cbData = (id)_cbData;

	[cbData->tracks addObject:[NSURL URLWithString:[cbData->baseUrl stringByAppendingFormat:@"#%i", startOrder]]];

	return 0;
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

	void *data = malloc(size);
	[source read:data amount:size];

	DUMBFILE *df = dumbfile_open_memory_and_free(data, size);
	if(!df) {
		ALog(@"Open failed for file: %@", [url absoluteString]);
		return NO;
	}

	NSMutableArray *tracks = [NSMutableArray array];

	int i;
	int subsongs = dumb_get_psm_subsong_count(df);
	if(subsongs) {
		dumbfile_close(df);

		for(i = 0; i < subsongs; ++i) {
			[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
		}
	} else {
		dumbfile_seek(df, 0, SEEK_SET);

		DUH *duh;
		NSString *ext = [[url pathExtension] lowercaseString];
		duh = dumb_read_any_quick(df, [ext isEqualToString:@"mod"] ? 0 : 1, 0);

		dumbfile_close(df);

		if(duh) {
			DumbCallbackData *data = [[DumbCallbackData alloc] init];
			data->baseUrl = [url absoluteString];
			data->tracks = tracks;

			dumb_it_scan_for_playable_orders(duh_get_it_sigdata(duh), scanCallback, (__bridge void *)data);
		}
	}

	return tracks;
}

@end
