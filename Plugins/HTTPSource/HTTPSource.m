//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "HTTPSource.h"


@implementation HTTPSource

- (BOOL)buffered
{
	return NO;
}

- (BOOL)open:(NSURL *)url
{

	unsigned int port = [[url port] unsignedIntValue];
	if (!port)
		port = 80;
		
	_socket = [[Socket alloc] initWithHost:[url host] port:port];

	if (_socket) {
		NSData *request = [[NSString stringWithFormat:@"GET %@ HTTP/1.0\nHOST: %@\n\n",[url path],[url host]] dataUsingEncoding:NSUTF8StringEncoding];
		[_socket send:(void *)[request bytes] amount:[request length]];
	}
	
	return (_socket != nil);
}

- (NSDictionary *)properties
{
	return nil;
}

- (BOOL)seekable
{
	return NO;
}

- (BOOL)seek:(long)position whence:(int)whence
{
	return NO;
}

- (long)tell
{
	return byteCount;
}

- (int)read:(void *)buffer amount:(int)amount
{
	int l = [_socket receive:buffer amount:amount];
	if (l > 0)
		byteCount += l;

	return l;
}

- (void)close
{
	[_socket close];
}

+ (NSArray *)schemes
{
	return [NSArray arrayWithObject:@"http"];
}

@end
