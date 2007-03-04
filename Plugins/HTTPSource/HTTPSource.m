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
		pastHeader = NO;
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
	if (!pastHeader) {
		const int delimeter_size = 4; //\r\n\r\n
		
		int l = [_socket receive:buffer amount:amount];
		uint8_t *f;
		while(NULL == (f = (uint8_t *)strnstr((const char *)buffer, "\r\n\r\n", l))) {
			//Need to check for boundary conditions
			memmove(buffer, (uint8_t *)buffer + (l - delimeter_size), delimeter_size);
			l = [_socket receive:((uint8_t *)buffer + delimeter_size) amount:(amount - delimeter_size)];
		}
		
		pastHeader = YES;
			
		uint8_t *bufferOffset = f + delimeter_size;
		uint8_t *bufferEnd = (uint8_t *)buffer + l;
		int amountRemaining = bufferEnd - bufferOffset;
		
		/*
		//For testing only
		FILE *testFout = fopen("header.raw", "w");
		fwrite(buffer, 1, bufferOffset - (uint8_t *)buffer, testFout);
		fclose(testFout);

		testFout = fopen("test.raw", "w");
		fwrite(bufferOffset, 1, amountRemaining, testFout);
		fclose(testFout);
		*/
		
		memmove(buffer,bufferOffset, amountRemaining);
		
		return amountRemaining + [self read:((uint8_t *)buffer + amountRemaining) amount:(amount - amountRemaining)];
	}
	else {
		int l = [_socket receive:buffer amount:amount];

		/*
		//FOR TESTING ONLY
		FILE *testFout = fopen("test.raw", "a");
		fwrite(buffer, 1, l, testFout);
		fclose(testFout);
		*/
		if (l > 0)
			byteCount += l;

		return l;
	}
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
