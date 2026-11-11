//
//  ArchiveContainer.m
//  ArchiveSource
//
//  Created by Christoper Snowhill on 10/8/07.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "ArchiveContainer.h"

#import <File_Extractor/fex.h>

#import "Logging.h"

#import "SandboxBroker.h"

static NSString *path_pack_string(NSString *src) {
	return [NSString stringWithFormat:@"|%lu|%@|", [src length], src];
}

static NSString *escape_url_delimiters(NSString *src) {
	static NSCharacterSet *allowed;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		allowed = [[NSCharacterSet characterSetWithCharactersInString:@"%#?"] invertedSet];
	});
	return [src stringByAddingPercentEncodingWithAllowedCharacters:allowed];
}

static NSString *g_make_unpack_path(NSString *archive, NSString *file, NSString *name) {
	return [NSString stringWithFormat:@"unpack://%@%@%@", name, escape_url_delimiters(path_pack_string(archive)), escape_url_delimiters(file)];
}

@implementation ArchiveContainer

+ (NSArray *)fileTypes {
	return @[@"zip", @"rar", @"7z", @"rsn", @"vgm7z", @"gz"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/zip", @"application/x-gzip", @"application/x-rar-compressed", @"application/x-7z-compressed"];
}

+ (float)priority {
	return 1.0f;
}

+ (void)initialize {
	fex_init();
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	if(![url isFileURL]) {
		return @[];
	}

	/* The archive path gets baked into every unpack:// URL below, so it has to
	 * be a real path rather than a file reference. */
	url = CogNormalizeURL(url);

	id sandboxBrokerClass = NSClassFromString(@"SandboxBroker");
	id sandboxBroker = [sandboxBrokerClass sharedSandboxBroker];

	const void *sbHandle = [sandboxBroker beginFolderAccess:url];

	fex_t *fex;
	fex_err_t error = fex_open(&fex, [[url path] UTF8String]);
	if(error) {
		ALog(@"Archive error: %s", error);
		return @[];
	}

	NSMutableArray *files = [NSMutableArray array];

	// Folders skip playlists by default too
	NSSet *playlistTypes = [NSSet setWithArray:@[@"m3u", @"m3u8", @"pls"]];

	while(!fex_done(fex)) {
		NSString *name = guess_encoding_of_string(fex_name(fex));
		NSString *type = [[name pathExtension] lowercaseString];
		if(![playlistTypes containsObject:type] && [[NSClassFromString(@"AudioPlayer") fileTypes] containsObject:type])
			[files addObject:[NSURL URLWithDataRepresentation:[g_make_unpack_path([url path], name, @"fex") dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil]];
		fex_next(fex);
	}

	fex_close(fex);

	[sandboxBroker endFolderAccess:sbHandle];

	return files;
}

@end
