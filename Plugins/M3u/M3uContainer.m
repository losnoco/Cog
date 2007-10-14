//
//  M3uContainer.m
//  M3u
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "M3uContainer.h"


@implementation M3uContainer

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"m3u"];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-mpegurl", @"audio/mpegurl", nil];
}

+ (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename
{
	if ([path hasPrefix:@"/"]) {
		return [NSURL fileURLWithPath:path];
	}
	
	NSRange foundRange = [path rangeOfString:@"://"];
	if (foundRange.location != NSNotFound) 
	{
		return [NSURL URLWithString:path];
	}
	
	NSString *basePath = [[[baseFilename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];
	NSMutableString *unixPath = [path mutableCopy];
	
	//Only relative paths would have windows backslashes.
	[unixPath replaceOccurrencesOfString:@"\\" withString:@"/" options:0 range:NSMakeRange(0, [unixPath length])];
	
	return [NSURL fileURLWithPath:[basePath stringByAppendingString:[unixPath autorelease]]];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	if (![url isFileURL]) 
		return [NSArray array];
	
	NSString *filename = [url path];
	
	NSError *error = nil;
	NSString *contents = [NSString stringWithContentsOfFile:filename encoding:NSUTF8StringEncoding error:&error];
	if (error || !contents) {
		NSLog(@"Could not open file...%@ %@", contents, error);
		return NO;
	}
	
	NSString *entry;
	NSEnumerator *e = [[contents componentsSeparatedByString:@"\n"] objectEnumerator];
	NSMutableArray *entries = [NSMutableArray array];
	
	while (entry = [[e nextObject] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]])
	{
		if ([entry hasPrefix:@"#"] || [entry isEqualToString:@""]) //Ignore extra info
			continue;
		
		//Need to add basePath, and convert to URL
		[entries addObject:[M3uContainer urlForPath:entry relativeTo:filename]];		
	}
	
	return entries;
}

@end
