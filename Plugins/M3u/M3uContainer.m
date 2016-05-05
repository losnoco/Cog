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

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"m3u", @"m3u8", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-mpegurl", @"audio/mpegurl", nil];
}

+ (float)priority
{
    return 1.0f;
}

+ (NSURL *)urlForPath:(NSString *)path relativeTo:(NSString *)baseFilename
{
	NSRange protocolRange = [path rangeOfString:@"://"];
	if (protocolRange.location != NSNotFound) 
	{
		return [NSURL URLWithString:path];
	}

	NSMutableString *unixPath = [path mutableCopy];

	//Get the fragment
	NSString *fragment = @"";
	NSScanner *scanner = [NSScanner scannerWithString:unixPath];
	NSCharacterSet *characterSet = [NSCharacterSet characterSetWithCharactersInString:@"#1234567890"];
	while (![scanner isAtEnd]) {
		NSString *possibleFragment;
		[scanner scanUpToString:@"#" intoString:nil];

		if ([scanner scanCharactersFromSet:characterSet intoString:&possibleFragment] && [scanner isAtEnd]) 
		{
			fragment = possibleFragment;
			[unixPath deleteCharactersInRange:NSMakeRange([scanner scanLocation] - [possibleFragment length], [possibleFragment length])];
			break;
		}
	}
	DLog(@"Fragment: %@", fragment);

	if (![unixPath hasPrefix:@"/"]) {
		//Only relative paths would have windows backslashes.
		[unixPath replaceOccurrencesOfString:@"\\" withString:@"/" options:0 range:NSMakeRange(0, [unixPath length])];
		
		NSString *basePath = [[[baseFilename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

		[unixPath insertString:basePath atIndex:0];
	}
	
	//Append the fragment
	NSURL *url = [NSURL URLWithString:[[[NSURL fileURLWithPath:unixPath] absoluteString] stringByAppendingString: fragment]];
	return url;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	if (![url isFileURL]) 
		return [NSArray array];
	
	NSString *filename = [url path];
	
	NSStringEncoding encoding;
	NSError *error = nil;
	NSString *contents = [NSString stringWithContentsOfFile:filename usedEncoding:&encoding error:&error];
    if (error) {
		DLog(@"Trying UTF8");
        error = nil;
        contents = [NSString stringWithContentsOfFile:filename encoding:NSUTF8StringEncoding error:&error];
    }
    if (error) {
		DLog(@"Trying windows CP1251");
        error = nil;
        contents = [NSString stringWithContentsOfFile:filename encoding:NSWindowsCP1251StringEncoding error:&error];
	}
    if (error) {
		DLog(@"Trying latin1");
        error = nil;
        contents = [NSString stringWithContentsOfFile:filename encoding:NSISOLatin1StringEncoding error:&error];
	}
	if (error || !contents) {
		ALog(@"Could not open file...%@ %@ %@", filename, contents, error);
		return nil;
	}
	
	NSMutableArray *entries = [NSMutableArray array];
	
    for (NSString *entry in [contents componentsSeparatedByString:@"\n"])
    {
        NSString *_entry = [entry stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

		if ([_entry hasPrefix:@"#"] || [_entry isEqualToString:@""]) //Ignore extra info
			continue;
		
		//Need to add basePath, and convert to URL
		[entries addObject:[self urlForPath:_entry relativeTo:filename]];
	}
	
	return entries;
}

@end
