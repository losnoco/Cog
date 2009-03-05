//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "HTTPSource.h"
#include <JNetLib/jnetlib.h>

@implementation HTTPSource

- (BOOL)open:(NSURL *)url
{
	_url = [url copy];
	
	JNL::open_socketlib();
	
	_get = new JNL_HTTPGet();
	
	NSString *userAgent = [NSString stringWithFormat:@"User-Agent:Cog %@", [[NSBundle mainBundle] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey]];
	_get->addheader([userAgent UTF8String]);
	_get->addheader("Connection:close");
	_get->addheader("Accept:*/*");
	
	_get->connect([[url absoluteString] UTF8String]);
	for(;;) {
        int status = _get->get_status();
		
        if (status < 0 || status > 1) {
			break;
		}
		
        if (_get->run() < 0) {
			return 0;
        }
	}
	
	int st = _get->run();
	if (st < 0) {
		return NO;
	}

	const char *mimeType = _get->getheader("content-type");
	if (NULL != mimeType) {
		_mimeType = [[NSString alloc] initWithUTF8String:mimeType];
	}

	return YES;
}

- (NSString *)mimeType
{
	NSLog(@"Returning mimetype! %@", _mimeType);
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

- (int)read:(void *)buffer amount:(int)amount
{
	int totalRead = 0;

	while (totalRead < amount) {
		int status = _get->run();
		int amountRead = _get->get_bytes((char *)((uint8_t *)buffer) + totalRead, amount - totalRead);
		
		totalRead += amountRead;

		if (status && 0 == amountRead) break;
	}

	_byteCount += totalRead;

	return totalRead;
}

- (void)close
{
	if (NULL != _get) {
		delete _get;
		_get = NULL;
	}
	
	[_url release];
	_url = nil;
	
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
	return _url;
}

+ (NSArray *)schemes
{
	return [NSArray arrayWithObject:@"http"];
}

@end
