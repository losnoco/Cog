//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "HTTPSource.h"
#import "HTTPConnection.h"

#import "Logging.h"

@implementation HTTPSource

- (BOOL)open:(NSURL *)url
{
	_connection = [[HTTPConnection alloc] initWithURL:url];
	
	// Note: The User-Agent CANNOT contain the string "Mozilla" or Shoutcast/Icecast will serve up HTML
	NSString *userAgent = [NSString stringWithFormat:@"Cog %@", [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey]];
	[_connection setValue:userAgent forRequestHeader:@"User-Agent"];
	[_connection setValue:@"close" forRequestHeader:@"Connection"];
	[_connection setValue:@"*/*" forRequestHeader:@"Accept"];

	BOOL success = [_connection connect];
	if (NO == success) {
		return NO;
	}
	
	_mimeType = [[_connection valueForResponseHeader:@"Content-type"] copy];

	return YES;
}

- (NSString *)mimeType
{
	DLog(@"Returning mimetype! %@", _mimeType);
	return _mimeType;
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
	return _byteCount;
}

- (long)read:(void *)buffer amount:(long)amount
{
	long totalRead = 0;

	while (totalRead < amount) {
		NSInteger amountReceived = [_connection receiveData:((uint8_t *)buffer) + totalRead amount:amount - totalRead];
		if (amountReceived <= 0) {
			break;
		}

		totalRead += amountReceived;
	}
	
	_byteCount += totalRead;

	return totalRead;
}

- (void)close
{
	[_connection close];
	[_connection release];
	_connection = nil;
	
	[_mimeType release];
	_mimeType = nil;
}


- (void)dealloc
{
	[self close];
	
	[super dealloc];
}

- (NSURL *)url
{
	return [_connection URL];
}

+ (NSArray *)schemes
{
	return [NSArray arrayWithObject:@"http"];
}

@end
