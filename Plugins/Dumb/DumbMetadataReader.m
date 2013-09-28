//
//  DumbMetadataReader.m
//  Dumb
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "DumbMetadataReader.h"
#import "DumbDecoder.h"

#import <Dumb/dumb.h>

@implementation DumbMetadataReader

+ (NSArray *)fileTypes
{
	return [DumbDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [DumbDecoder mimeTypes];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	if (![url isFileURL])
		return nil;

	dumb_register_stdfiles();

	DUH *duh;
	NSString *ext = [[[url path] pathExtension] lowercaseString];
    duh = dumb_load_any_quick([[url path] UTF8String], [ext isEqualToString:@"mod"] ? 0 : 1, 0);

	if (!duh)
	{
		NSLog(@"Failed to create duh");
		return nil;
	}

	//Some titles are all spaces?!
	NSString *title = [[NSString stringWithUTF8String: duh_get_tag(duh, "TITLE")] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
	unload_duh(duh);

	if (title == nil) {
		title = @"";
	}
	
	return [NSDictionary dictionaryWithObject:title forKey:@"title"];
}

@end
