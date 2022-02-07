//
//  M3uContainer.m
//  M3u
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "M3uContainer.h"

#import "Logging.h"

@implementation M3uContainer

+ (NSArray *)fileTypes {
	return @[@"m3u", @"m3u8"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-mpegurl", @"audio/mpegurl"];
}

+ (float)priority {
	return 1.0f;
}

+ (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename {
	NSRange protocolRange = [path rangeOfString:@"://"];
	if(protocolRange.location != NSNotFound) {
		return [NSURL URLWithString:path];
	}

	NSMutableString *unixPath = [path mutableCopy];

	// Get the fragment
	NSString *fragment = @"";
	NSScanner *scanner = [NSScanner scannerWithString:unixPath];
	NSCharacterSet *characterSet = [NSCharacterSet characterSetWithCharactersInString:@"#1234567890"];
	while(![scanner isAtEnd]) {
		NSString *possibleFragment;
		[scanner scanUpToString:@"#" intoString:nil];

		if([scanner scanCharactersFromSet:characterSet intoString:&possibleFragment] && [scanner isAtEnd]) {
			fragment = possibleFragment;
			[unixPath deleteCharactersInRange:NSMakeRange([scanner scanLocation] - [possibleFragment length], [possibleFragment length])];
			break;
		}
	}
	DLog(@"Fragment: %@", fragment);

	if(![unixPath hasPrefix:@"/"]) {
		// Only relative paths would have windows backslashes.
		[unixPath replaceOccurrencesOfString:@"\\" withString:@"/" options:0 range:NSMakeRange(0, [unixPath length])];

		NSString *basePath = [[[baseFilename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

		[unixPath insertString:basePath atIndex:0];
	}

	// Append the fragment
	NSURL *url = [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString:fragment]];
	return url;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	char *filecontents = nil;

	{
		id audioSourceClass = NSClassFromString(@"AudioSource");
		id<CogSource> source = [audioSourceClass audioSourceForURL:url];

		if(![source open:url])
			return @[];

		long size = 0;
		long bytesread = 0;

		do {
			filecontents = (char *)realloc(filecontents, size + 1024);
			bytesread = [source read:(filecontents + size) amount:1024];
			size += bytesread;
		} while(bytesread == 1024);

		filecontents = (char *)realloc(filecontents, size + 1);

		filecontents[size] = '\0';
	}

	// Handle macOS Classic and Windows line endings
	{
		char *contentsscan = filecontents;
		while(*contentsscan) {
			if(*contentsscan == '\r')
				*contentsscan = '\n';
			++contentsscan;
		}
	}

	DLog(@"Trying UTF8");
	NSStringEncoding encoding = NSUTF8StringEncoding;
	NSString *contents = [NSString stringWithCString:filecontents encoding:encoding];
	if(!contents) {
		DLog(@"Trying windows GB 18030 2000");
		contents = [NSString stringWithCString:filecontents encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingGB_18030_2000)];
	}
	if(!contents) {
		DLog(@"Trying windows CP1251");
		contents = [NSString stringWithCString:filecontents encoding:NSWindowsCP1251StringEncoding];
	}
	if(!contents) {
		DLog(@"Trying latin1");
		contents = [NSString stringWithCString:filecontents encoding:NSISOLatin1StringEncoding];
	}
	free(filecontents);
	if(!contents) {
		ALog(@"Could not open file...%@ %@", url, contents);
		return @[];
	}

	NSMutableArray *entries = [NSMutableArray array];

	for(NSString *entry in [contents componentsSeparatedByString:@"\n"]) {
		NSString *_entry = [entry stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

		if([_entry hasPrefix:@"#EXT-X-MEDIA-SEQUENCE"]) // Let FFmpeg handle HLS
			return @[];

		if([_entry hasPrefix:@"#"] || [_entry isEqualToString:@""]) // Ignore extra info
			continue;

		// Need to add basePath, and convert to URL
		[entries addObject:[self urlForPath:_entry relativeTo:[url path]]];
	}

	return entries;
}

@end
