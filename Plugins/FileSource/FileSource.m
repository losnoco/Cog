//
//  FileSource.m
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileSource.h"


@implementation FileSource

- (BOOL)open:(NSURL *)url
{
	_url = url;
	[_url retain];
	
	_fd = fopen([[url path] UTF8String], "r");
	
	return (_fd != NULL);
}

- (BOOL)seekable
{
	return YES;
}

- (BOOL)seek:(long)position whence:(int)whence
{
	return (fseek(_fd, position, whence) == 0);
}

- (long)tell
{
	return ftell(_fd);
}

- (int)read:(void *)buffer amount:(int)amount
{
	return fread(buffer, 1, amount, _fd);
}

- (void)close
{
	[_url release];
	_url = nil;
	
	fclose(_fd);
	_fd = NULL;
}

- (NSURL *)url
{
	return _url;
}

- (NSString *)mimeType
{
	return nil;
}

- (void)setURL:(NSURL *)url
{
	[url retain];
	[_url release];
	_url = url;
}


+ (NSArray *)schemes
{
	return [NSArray arrayWithObject:@"file"];
}

- (void)dealloc {
	NSLog(@"DEALLOCATING SOURCE");
	
	[super dealloc];
}

@end
