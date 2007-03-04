//
//  FileSource.m
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FileSource.h"


@implementation FileSource

- (BOOL)buffered
{
	return NO;
}

- (BOOL)open:(NSURL *)url
{
	[self setURL:url];
	
	_fd = fopen([[url path] UTF8String], "r");
	
	return (_fd != NULL);
}

- (NSDictionary *)properties
{
	return nil;
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
	fclose(_fd);
}

- (NSURL *)url
{
	return _url;
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

@end
