//
//  PlsContainer.m
//  Pls
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "PlsContainer.h"

#import "Logging.h"

@implementation PlsContainer

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"pls"];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-scpls", @"application/pls", nil];
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
    char * filecontents = nil;
    
    {
        id audioSourceClass = NSClassFromString(@"AudioSource");
        id<CogSource> source = [audioSourceClass audioSourceForURL:url];
        
        if (![source open:url])
            return [NSArray array];
        
        long size = 0;
        long bytesread = 0;

        do {
            filecontents = (char *) realloc(filecontents, size + 1024);
            bytesread = [source read:(filecontents + size) amount:1024];
            size += bytesread;
        } while (bytesread == 1024);
        
        filecontents = (char *) realloc(filecontents, size + 1);
            
        filecontents[size] = '\0';
    }
    
    // Handle macOS Classic and Windows line endings
    {
        char * contentsscan = filecontents;
        while (*contentsscan) {
            if (*contentsscan == '\r')
                *contentsscan = '\n';
            ++contentsscan;
        }
    }
    
    DLog(@"Trying UTF8");
	NSStringEncoding encoding = NSUTF8StringEncoding;
    NSString *contents = [NSString stringWithCString:filecontents encoding:encoding];
    if (!contents) {
		DLog(@"Trying windows GB 18030 2000");
		contents = [NSString stringWithCString:filecontents encoding:CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingGB_18030_2000)];
	}
    if (!contents) {
		DLog(@"Trying windows CP1251");
        contents = [NSString stringWithCString:filecontents encoding:NSWindowsCP1251StringEncoding];
	}
    if (!contents) {
		DLog(@"Trying latin1");
        contents = [NSString stringWithCString:filecontents encoding:NSISOLatin1StringEncoding];
	}
    free(filecontents);
	if (!contents) {
		ALog(@"Could not open file...%@ %@", url, contents);
		return [NSArray array];
	}
	
	NSMutableArray *entries = [NSMutableArray array];
	
    for (NSString *entry in [contents componentsSeparatedByString:@"\n"])
    {
        NSString *_entry = [entry stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

		NSScanner *scanner = [[NSScanner alloc] initWithString:_entry];
		NSString *lhs = nil;
		NSString *rhs = nil;

		if (![scanner scanUpToString:@"=" intoString:&lhs]	|| // get LHS
			![scanner scanString:@"=" intoString:nil]		|| // skip the =
			![scanner scanUpToString:@"" intoString:&rhs]	|| // get RHS
			[lhs rangeOfString:@"File" options:NSCaseInsensitiveSearch|NSAnchoredSearch].location == NSNotFound) // We only want file entries
		{
			continue;
		}
		
		//need to add basepath if its a file, and convert to URL
		[entries addObject:[self urlForPath:rhs relativeTo:[url path]]];
	}
	
	return entries;
}	

@end
